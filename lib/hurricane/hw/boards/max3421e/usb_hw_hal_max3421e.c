#ifdef MAX3421E_ENABLED

#include "hw/hurricane_hw_hal.h"
#include "max3421e_registers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef PLATFORM_ESP32
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Configuration for ESP32 SPI and GPIO pins
#define MAX3421E_SPI_HOST       SPI2_HOST
#define MAX3421E_SPI_MOSI       GPIO_NUM_23
#define MAX3421E_SPI_MISO       GPIO_NUM_19
#define MAX3421E_SPI_CLK        GPIO_NUM_18
#define MAX3421E_SPI_CS         GPIO_NUM_5
#define MAX3421E_INT_PIN        GPIO_NUM_4
#define MAX3421E_RST_PIN        GPIO_NUM_22

static const char* TAG = "max3421e";
static spi_device_handle_t spi_handle;
static uint8_t current_device_address = 0;
static bool device_connected = false;

// Forward declarations for internal functions
static uint8_t max3421e_read_register(uint8_t reg);
static void max3421e_write_register(uint8_t reg, uint8_t data);
static void max3421e_write_bytes(uint8_t reg, const uint8_t* data, uint8_t length);
static void max3421e_read_bytes(uint8_t reg, uint8_t* data, uint8_t length);
static int max3421e_wait_for_interrupt(uint8_t irq_mask, uint16_t timeout_ms);
static void max3421e_reset(void);
static uint8_t max3421e_get_connection_speed(void);
static uint8_t max3421e_get_status(void);
static uint8_t max3421e_get_result(void);
static void max3421e_handle_irqs(void);

// SPI communication functions
static uint8_t max3421e_read_register(uint8_t reg) {
    uint8_t cmd = reg | MAX3421E_DIR_IN;
    uint8_t result = 0;
    
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .rxlength = 8,
        .rx_buffer = &result,
        .flags = SPI_TRANS_USE_RXDATA
    };
    
    if (spi_device_polling_transmit(spi_handle, &t) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read register 0x%02X", reg);
        return 0;
    }
    
    return result;
}

static void max3421e_write_register(uint8_t reg, uint8_t data) {
    uint8_t cmd[2] = { reg | MAX3421E_DIR_OUT, data };
    
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = cmd,
        .flags = 0
    };
    
    if (spi_device_polling_transmit(spi_handle, &t) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write register 0x%02X", reg);
    }
}

static void max3421e_write_bytes(uint8_t reg, const uint8_t* data, uint8_t length) {
    uint8_t* buffer = malloc(length + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer for SPI write");
        return;
    }
    
    buffer[0] = reg | MAX3421E_DIR_OUT;
    memcpy(&buffer[1], data, length);
    
    spi_transaction_t t = {
        .length = 8 * (length + 1),
        .tx_buffer = buffer,
        .flags = 0
    };
    
    if (spi_device_polling_transmit(spi_handle, &t) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write multiple bytes to register 0x%02X", reg);
    }
    
    free(buffer);
}

static void max3421e_read_bytes(uint8_t reg, uint8_t* data, uint8_t length) {
    uint8_t cmd = reg | MAX3421E_DIR_IN;
    
    spi_transaction_t t_cmd = {
        .length = 8,
        .tx_buffer = &cmd,
        .flags = 0
    };
    
    if (spi_device_polling_transmit(spi_handle, &t_cmd) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send read command to register 0x%02X", reg);
        return;
    }
    
    spi_transaction_t t_data = {
        .length = 8 * length,
        .rx_buffer = data,
        .flags = 0
    };
    
    if (spi_device_polling_transmit(spi_handle, &t_data) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read multiple bytes from register 0x%02X", reg);
    }
}

// Initialize the MAX3421E host controller
void hurricane_hw_init(void) {
    ESP_LOGI(TAG, "Initializing MAX3421E USB host controller");

    // Configure GPIO pins
    gpio_config_t gpio_conf = {
        .pin_bit_mask = (1ULL << MAX3421E_INT_PIN) | (1ULL << MAX3421E_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpio_conf);
    
    // Reset MAX3421E using reset pin
    gpio_set_level(MAX3421E_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(MAX3421E_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Configure SPI bus
    spi_bus_config_t bus_config = {
        .mosi_io_num = MAX3421E_SPI_MOSI,
        .miso_io_num = MAX3421E_SPI_MISO,
        .sclk_io_num = MAX3421E_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(MAX3421E_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));
    
    // Attach the MAX3421E to the SPI bus
    spi_device_interface_config_t dev_config = {
        .mode = 0,
        .clock_speed_hz = 10*1000*1000,  // 10 MHz
        .spics_io_num = MAX3421E_SPI_CS,
        .queue_size = 3,
        .flags = 0,
    };
    
    ESP_ERROR_CHECK(spi_bus_add_device(MAX3421E_SPI_HOST, &dev_config, &spi_handle));
    
    // Reset the MAX3421E
    max3421e_reset();
    
    // Configure the MAX3421E for host mode
    uint8_t mode_reg = max3421e_read_register(MAX3421E_REG_MODE);
    mode_reg |= MAX3421E_MODE_HOST;      // Host mode
    mode_reg |= MAX3421E_MODE_SOFKAENAB; // Enable SOF generation
    max3421e_write_register(MAX3421E_REG_MODE, mode_reg);
    
    // Configure pin control
    max3421e_write_register(MAX3421E_REG_PINCTL, MAX3421E_PINCTL_FDUPSPI | MAX3421E_PINCTL_INTLEVEL);
    
    // Enable interrupts
    max3421e_write_register(MAX3421E_REG_HIEN, 
                           MAX3421E_HIEN_CONDETIE |  // Connect detect
                           MAX3421E_HIEN_RCVDAVIE |  // Receive data available
                           MAX3421E_HIEN_HXFRDNIE);  // Transfer done
    
    // Enable USB interrupts
    max3421e_write_register(MAX3421E_REG_USBIEN, 
                           MAX3421E_USBIEN_OSCOKIE |  // Oscillator OK
                           MAX3421E_USBIEN_VBUSIE);   // VBUS change
    
    ESP_LOGI(TAG, "MAX3421E initialized successfully");
    
    // Start with a clean interrupt state
    max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);
    max3421e_write_register(MAX3421E_REG_USBIRQ, 0xFF);
    
    device_connected = false;
    current_device_address = 0;
}

// Poll the MAX3421E for events
void hurricane_hw_poll(void) {
    // Check for and handle interrupts
    max3421e_handle_irqs();
    
    // Check for device connection if none connected
    if (!device_connected) {
        uint8_t jk_state = max3421e_get_status();
        if (jk_state & MAX3421E_HRSL_JSTATUS) {
            ESP_LOGI(TAG, "J state detected: Full-speed device connected");
            device_connected = true;
            
            // Issue bus reset
            max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_BUSRST);
            vTaskDelay(pdMS_TO_TICKS(50));  // Wait for bus reset to complete
            max3421e_write_register(MAX3421E_REG_HCTL, 0);
            vTaskDelay(pdMS_TO_TICKS(20));  // Recovery time
            
            // Set default address 0 for enumeration
            current_device_address = 0;
            max3421e_write_register(MAX3421E_REG_PERADDR, current_device_address);
        } else if (jk_state & MAX3421E_HRSL_KSTATUS) {
            ESP_LOGI(TAG, "K state detected: Low-speed device connected");
            device_connected = true;
            
            // Configure for low-speed
            uint8_t mode = max3421e_read_register(MAX3421E_REG_MODE);
            mode |= MAX3421E_MODE_SPEED;  // Enable low-speed mode
            max3421e_write_register(MAX3421E_REG_MODE, mode);
            
            // Issue bus reset
            max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_BUSRST);
            vTaskDelay(pdMS_TO_TICKS(50));  // Wait for bus reset to complete
            max3421e_write_register(MAX3421E_REG_HCTL, 0);
            vTaskDelay(pdMS_TO_TICKS(20));  // Recovery time
            
            // Set default address 0 for enumeration
            current_device_address = 0;
            max3421e_write_register(MAX3421E_REG_PERADDR, current_device_address);
        }
    }
}

// Check if a device is connected
int hurricane_hw_device_connected(void) {
    return device_connected ? 1 : 0;
}

// Reset the USB bus
void hurricane_hw_reset_bus(void) {
    ESP_LOGI(TAG, "Resetting USB bus");
    
    // Issue bus reset
    max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_BUSRST);
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for bus reset to complete
    max3421e_write_register(MAX3421E_REG_HCTL, 0);
    vTaskDelay(pdMS_TO_TICKS(20));  // Recovery time
    
    // Reset device tracking
    device_connected = false;
    current_device_address = 0;
}

// Perform a USB control transfer
int hurricane_hw_control_transfer(const hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    if (!device_connected) {
        ESP_LOGE(TAG, "No device connected for control transfer");
        return -1;
    }
    
    ESP_LOGI(TAG, "Control transfer: bRequest=0x%02X, wValue=0x%04X, wIndex=0x%04X, wLength=%u",
             setup->bRequest, setup->wValue, setup->wIndex, setup->wLength);
    
    // Set device address for the transaction
    max3421e_write_register(MAX3421E_REG_PERADDR, current_device_address);
    
    // Load setup data into SUDFIFO
    uint8_t setup_data[8];
    setup_data[0] = setup->bmRequestType;
    setup_data[1] = setup->bRequest;
    setup_data[2] = setup->wValue & 0xFF;
    setup_data[3] = (setup->wValue >> 8) & 0xFF;
    setup_data[4] = setup->wIndex & 0xFF;
    setup_data[5] = (setup->wIndex >> 8) & 0xFF;
    setup_data[6] = setup->wLength & 0xFF;
    setup_data[7] = (setup->wLength >> 8) & 0xFF;
    
    max3421e_write_bytes(MAX3421E_REG_SUDFIFO, setup_data, 8);
    
    // Clear any pending interrupts
    max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);
    
    // Trigger SETUP transfer
    max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_SETUP);
    
    // Wait for transfer completion
    if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 500) != 0) {
        ESP_LOGE(TAG, "Timeout waiting for SETUP stage completion");
        return -1;
    }
    
    // Check result
    uint8_t result = max3421e_get_result();
    if (result != MAX3421E_RESULT_SUCCESS) {
        ESP_LOGE(TAG, "SETUP transfer failed: 0x%02X", result);
        return -1;
    }
    
    // Handle data stage if present
    if (setup->wLength > 0) {
        // Determine direction
        bool is_device_to_host = (setup->bmRequestType & 0x80) != 0;
        
        if (is_device_to_host) {
            // IN transfer (device to host)
            max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_RCVTOG1);  // Set toggle for IN data stage
            max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);  // Clear interrupts
            max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_IN);  // Start IN transfer
            
            if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 500) != 0) {
                ESP_LOGE(TAG, "Timeout waiting for IN data stage");
                return -1;
            }
            
            result = max3421e_get_result();
            if (result != MAX3421E_RESULT_SUCCESS) {
                ESP_LOGE(TAG, "IN data stage failed: 0x%02X", result);
                return -1;
            }
            
            // Read the received data
            uint8_t recv_bytes = max3421e_read_register(MAX3421E_REG_RCVBC);
            if (recv_bytes > 0) {
                if (recv_bytes > length) {
                    recv_bytes = length;
                }
                
                max3421e_read_bytes(MAX3421E_REG_RCVFIFO, buffer, recv_bytes);
                
                // Send zero-length OUT status stage
                max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_SNDTOG1);  // Set toggle for OUT status
                max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);  // Clear interrupts
                max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_OUT);  // Start OUT transfer
                
                if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 500) != 0) {
                    ESP_LOGE(TAG, "Timeout waiting for OUT status stage");
                    return -1;
                }
                
                return recv_bytes;  // Return number of bytes received
            }
        } else {
            // OUT transfer (host to device)
            // Load data to send
            if (length > 0 && buffer != NULL) {
                // Write data to SNDFIFO
                max3421e_write_bytes(MAX3421E_REG_SNDFIFO, buffer, length);
                max3421e_write_register(MAX3421E_REG_SNDBC, length);
                
                // Start OUT data transfer
                max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_SNDTOG1);  // Set toggle for OUT data
                max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);  // Clear interrupts
                max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_OUT);  // Start OUT transfer
                
                if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 500) != 0) {
                    ESP_LOGE(TAG, "Timeout waiting for OUT data stage");
                    return -1;
                }
                
                result = max3421e_get_result();
                if (result != MAX3421E_RESULT_SUCCESS) {
                    ESP_LOGE(TAG, "OUT data stage failed: 0x%02X", result);
                    return -1;
                }
                
                // IN status stage
                max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_RCVTOG1);  // Set toggle for IN status
                max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);  // Clear interrupts
                max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_IN);  // Start IN transfer
                
                if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 500) != 0) {
                    ESP_LOGE(TAG, "Timeout waiting for IN status stage");
                    return -1;
                }
                
                return length;  // Return number of bytes sent
            }
        }
    } else {
        // No data stage, just a status stage
        // If bmRequestType & 0x80, device sends a zero-length OUT packet
        // Otherwise, device sends a zero-length IN packet
        bool is_device_to_host = (setup->bmRequestType & 0x80) != 0;
        
        if (is_device_to_host) {
            // Status stage is OUT
            max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_SNDTOG1);  // Set toggle for OUT status
            max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);  // Clear interrupts
            max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_OUT);  // Start OUT transfer
        } else {
            // Status stage is IN
            max3421e_write_register(MAX3421E_REG_HCTL, MAX3421E_HCTL_RCVTOG1);  // Set toggle for IN status
            max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);  // Clear interrupts
            max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_IN);  // Start IN transfer
        }
        
        if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 500) != 0) {
            ESP_LOGE(TAG, "Timeout waiting for status stage");
            return -1;
        }
    }
    
    return 0;
}

// Set the device address after enumeration
int hurricane_hw_set_address(uint8_t address) {
    ESP_LOGI(TAG, "Setting device address to %d", address);
    current_device_address = address;
    max3421e_write_register(MAX3421E_REG_PERADDR, current_device_address);
    return 0;
}

// Perform an interrupt IN transfer (for HID devices)
int hurricane_hw_interrupt_in_transfer(uint8_t endpoint, void* buffer, uint16_t length) {
    if (!device_connected) {
        ESP_LOGE(TAG, "No device connected for interrupt transfer");
        return -1;
    }
    
    // Set device address
    max3421e_write_register(MAX3421E_REG_PERADDR, current_device_address);
    
    // Clear any pending interrupts
    max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);
    
    // Toggle receive data 
    uint8_t hctl = 0;
    static bool toggle = false;
    if (toggle) {
        hctl |= MAX3421E_HCTL_RCVTOG1;
    } else {
        hctl |= MAX3421E_HCTL_RCVTOG0;
    }
    
    max3421e_write_register(MAX3421E_REG_HCTL, hctl);
    
    // Start the IN transfer - endpoint goes in the lower 4 bits
    max3421e_write_register(MAX3421E_REG_HXFR, MAX3421E_HXFR_IN | (endpoint & MAX3421E_HXFR_EP_MASK));
    
    // Wait for transfer completion with short timeout (non-blocking behavior)
    if (max3421e_wait_for_interrupt(MAX3421E_HIRQ_HXFRDNIRQ, 5) != 0) {
        // No data available, which is fine for polling interrupt endpoints
        return 0;
    }
    
    // Check result
    uint8_t result = max3421e_get_result();
    if (result == MAX3421E_RESULT_SUCCESS) {
        // Get received data
        uint8_t recv_bytes = max3421e_read_register(MAX3421E_REG_RCVBC);
        if (recv_bytes > 0) {
            // Limit to buffer size
            if (recv_bytes > length) {
                recv_bytes = length;
            }
            
            // Read data from RCVFIFO
            max3421e_read_bytes(MAX3421E_REG_RCVFIFO, buffer, recv_bytes);
            
            // Toggle for next transfer
            toggle = !toggle;
            
            return recv_bytes;
        }
    } else if (result == MAX3421E_RESULT_NAK) {
        // NAK is normal for interrupt endpoints when no data is available
        return 0;
    } else {
        ESP_LOGW(TAG, "Interrupt transfer failed: 0x%02X", result);
        return -1;
    }
    
    return 0;
}

// Internal helper functions

static void max3421e_reset(void) {
    // Software reset
    max3421e_write_register(MAX3421E_REG_USBCTL, MAX3421E_USBCTL_CHIPRES);
    vTaskDelay(pdMS_TO_TICKS(10));
    max3421e_write_register(MAX3421E_REG_USBCTL, 0);
    
    // Wait for oscillator to stabilize
    ESP_LOGI(TAG, "Waiting for MAX3421E oscillator...");
    uint16_t timeout = 500;  // 500ms timeout
    while (timeout > 0) {
        uint8_t usbirq = max3421e_read_register(MAX3421E_REG_USBIRQ);
        if (usbirq & MAX3421E_USBIRQ_OSCOKIRQ) {
            ESP_LOGI(TAG, "MAX3421E oscillator stabilized");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        timeout--;
    }
    
    if (timeout == 0) {
        ESP_LOGW(TAG, "Timeout waiting for oscillator to stabilize");
    }
    
    // Clear interrupt flags
    max3421e_write_register(MAX3421E_REG_USBIRQ, 0xFF);
    max3421e_write_register(MAX3421E_REG_HIRQ, 0xFF);
}

static uint8_t max3421e_get_status(void) {
    return max3421e_read_register(MAX3421E_REG_HRSL);
}

static uint8_t max3421e_get_result(void) {
    uint8_t hrsl = max3421e_read_register(MAX3421E_REG_HRSL);
    return hrsl & MAX3421E_HRSL_RESULT_MASK;
}

static uint8_t max3421e_get_connection_speed(void) {
    uint8_t mode = max3421e_read_register(MAX3421E_REG_MODE);
    return (mode & MAX3421E_MODE_SPEED) ? 0 : 1;  // 0 = Low-speed, 1 = Full-speed
}

static int max3421e_wait_for_interrupt(uint8_t irq_mask, uint16_t timeout_ms) {
    while (timeout_ms > 0) {
        uint8_t irq = max3421e_read_register(MAX3421E_REG_HIRQ);
        if (irq & irq_mask) {
            return 0;  // Success
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        timeout_ms--;
    }
    return -1;  // Timeout
}

static void max3421e_handle_irqs(void) {
    // Read interrupt flags
    uint8_t usbirq = max3421e_read_register(MAX3421E_REG_USBIRQ);
    uint8_t hirq = max3421e_read_register(MAX3421E_REG_HIRQ);
    
    // Clear the flags we're handling
    max3421e_write_register(MAX3421E_REG_USBIRQ, usbirq);
    max3421e_write_register(MAX3421E_REG_HIRQ, hirq);
    
    // Handle USB interrupts
    if (usbirq & MAX3421E_USBIRQ_VBUSIRQ) {
        ESP_LOGI(TAG, "VBUS change detected");
    }
    
    if (usbirq & MAX3421E_USBIRQ_NOVBUSIRQ) {
        ESP_LOGI(TAG, "VBUS removed");
        device_connected = false;
    }
    
    // Handle host interrupts
    if (hirq & MAX3421E_HIRQ_CONDETIRQ) {
        ESP_LOGI(TAG, "Device connection/disconnection event");
        
        // Check connection status
        uint8_t jk_state = max3421e_get_status();
        
        if ((jk_state & (MAX3421E_HRSL_JSTATUS | MAX3421E_HRSL_KSTATUS)) == 0) {
            ESP_LOGI(TAG, "Device disconnected");
            device_connected = false;
        }
    }
}

#else
// Stub implementation for non-ESP32 platforms
void hurricane_hw_init(void) {
    printf("[MAX3421E] Hardware initialization stub\n");
}

void hurricane_hw_poll(void) {
    // No-op
}

int hurricane_hw_device_connected(void) {
    return 0;
}

void hurricane_hw_reset_bus(void) {
    printf("[MAX3421E] Bus reset stub\n");
}

int hurricane_hw_control_transfer(const hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    printf("[MAX3421E] Control transfer stub\n");
    return -1;
}

int hurricane_hw_interrupt_in_transfer(uint8_t endpoint, void* buffer, uint16_t length) {
    printf("[MAX3421E] Interrupt transfer stub\n");
    return -1;
}
#endif // PLATFORM_ESP32

#endif // MAX3421E_ENABLED