/**
 * @file usb_hw_init_lpc55s69.c
 * @brief LPC55S69-specific USB hardware initialization
 * 
 * This file implements the USB hardware initialization for the LPC55S69 platform,
 * including clock configuration, PHY settings, power management, and resets.
 * It supports both USB0 (IP3511 FS) for device mode and USB1 (EHCI HS) for host mode.
 */

#include <stdio.h>
#include <stdatomic.h>

// NXP SDK includes
#include "fsl_device_registers.h"
#include "fsl_power.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "usb.h"
#include "usb_phy.h"
#include "clock_config.h"
#include "PERI_USBPHY.h"

// USB PHY calibration values for the LPC55S69
#ifndef BOARD_USB_PHY_D_CAL
#define BOARD_USB_PHY_D_CAL (0x0CU)  // Default calibration value
#endif

#ifndef BOARD_USB_PHY_TXCAL45DP
#define BOARD_USB_PHY_TXCAL45DP (0x06U)  // Default DP calibration
#endif

#ifndef BOARD_USB_PHY_TXCAL45DM
#define BOARD_USB_PHY_TXCAL45DM (0x06U)  // Default DM calibration
#endif

// XTAL/clock configuration
#ifndef BOARD_XTAL_FREQ
#define BOARD_XTAL_FREQ (24000000U)  // 24MHz crystal
#endif

// USB interrupt priorities
#define USB_DEVICE_INTERRUPT_PRIORITY (3U)
#define USB_HOST_INTERRUPT_PRIORITY (3U)

// External device and host handle references
extern usb_device_handle device_handle;
extern usb_host_handle host_handle;

// Forward-declare ISR functions
extern void USB_DeviceLpcIp3511IsrFunction(void *deviceHandle);
extern void USB_HostEhciIsrFunction(void *hostHandle);

/**
 * @brief Initialize USB0 (LPC IP3511 FS) for device mode
 */
void usb_device_hw_init(void)
{
    // Turn on USB0 PHY
    POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY);

    // Reset the USB0 device controller
    RESET_PeripheralReset(kUSB0D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HSL_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HMR_RST_SHIFT_RSTn);

    // Configure USB0 clock: FRO (96MHz) / 2 = 48MHz
    CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 2U, true);
    CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);

    // Configure USB0 PHY
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
    
    // Enable USB0 host clock to access the registers
    CLOCK_EnableClock(kCLOCK_Usbhsl0);
    
    // Configure device mode for USB0
    *((uint32_t *)(USBFSH_BASE + 0x5C)) |= USBFSH_PORTMODE_DEV_ENABLE_MASK;
    
    // Disable USB0 host clock after configuration
    CLOCK_DisableClock(kCLOCK_Usbhsl0);

    // Initialize USB0 PHY
    USB_EhciPhyInit(kUSB_ControllerLpcIp3511Fs0, BOARD_XTAL_FREQ, &phyConfig);

    printf("[LPC55S69] USB0 initialized: FS IP3511 (48MHz) for device mode\n");
}

/**
 * @brief Initialize USB1 (EHCI HS) for host mode
 */
void usb_host_hw_init(void)
{
    // Turn on USB1 PHY
    POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY);
    
    // Enable and reset USB1 controller
    CLOCK_EnableClock(kCLOCK_Usb1Clk);
    CLOCK_EnableClock(kCLOCK_Usbh1);
    RESET_PeripheralReset(kUSB1H_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1RAM_RST_SHIFT_RSTn);

    // For USB1 host mode with HS capability
    CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_UsbPhySrcExt, BOARD_XTAL_FREQ);
    CLOCK_EnableUsbhs0HostClock(kCLOCK_UsbSrcUnused, 0U);

    // Configure USB1 PHY
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
    
    // For HS controller, enable host mode
    CLOCK_EnableClock(kCLOCK_Usbh1);
    *((uint32_t *)(USBHSH_BASE + 0x50)) = USBHSH_PORTMODE_SW_PDCOM_MASK;
    *((uint32_t *)(USBHSH_BASE + 0x50)) &= ~USBHSH_PORTMODE_DEV_ENABLE_MASK;
    CLOCK_DisableClock(kCLOCK_Usbh1);

    // Initialize USB1 PHY
    USB_EhciPhyInit(kUSB_ControllerEhci1, BOARD_XTAL_FREQ, &phyConfig);

    printf("[LPC55S69] USB1 initialized: HS EHCI (480MHz) for host mode\n");
}

/**
 * @brief Enable USB0 device controller interrupt
 */
void USB_DeviceIsrEnable(void)
{
    uint8_t irqNumber = USB0_IRQn;
    
    // Set priority and enable interrupt
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    EnableIRQ((IRQn_Type)irqNumber);
}

/**
 * @brief Enable USB1 host controller interrupt
 */
void USB_HostIsrEnable(void)
{
    uint8_t irqNumber = USB1_IRQn;
    
    // Set priority and enable interrupt
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
    EnableIRQ((IRQn_Type)irqNumber);
}

/**
 * @brief USB0 (Device) IRQ Handler
 */
void USB0_IRQHandler(void)
{
    USB_DeviceLpcIp3511IsrFunction(device_handle);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F */
    __DSB();
}

/**
 * @brief USB1 (Host) IRQ Handler
 */
void USB1_IRQHandler(void)
{
    USB_HostEhciIsrFunction(host_handle);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F */
    __DSB();
}

/* Resource synchronization lock for dual USB operations */
static atomic_flag usb_resource_lock = ATOMIC_FLAG_INIT;

/**
 * @brief Synchronize shared USB resources (clocks/power).
 */
void hurricane_hw_sync_controllers(void)
{
    while (atomic_flag_test_and_set(&usb_resource_lock)) {}
    /* No shared resources to manage on LPC55S69 */
    atomic_flag_clear(&usb_resource_lock);
}

/**
 * @brief Initialize both USB controllers.
 * 
 * This is the main entry point called by the Hurricane library.
 */
void hurricane_hw_init(void)
{
    printf("[LPC55S69] Initializing USB controllers...\n");
    
    // Clear any pending IRQs
    NVIC_ClearPendingIRQ(USB0_IRQn);
    NVIC_ClearPendingIRQ(USB0_NEEDCLK_IRQn);
    NVIC_ClearPendingIRQ(USB1_IRQn);
    NVIC_ClearPendingIRQ(USB1_NEEDCLK_IRQn);
    
    // Power up both PHYs
    POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY);
    POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY);
    
    // Reset all USB controllers to ensure clean state
    RESET_PeripheralReset(kUSB0D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HSL_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HMR_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1H_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1RAM_RST_SHIFT_RSTn);
    
    // Initialize the controllers
    usb_device_hw_init();  // USB0 for device mode
    usb_host_hw_init();    // USB1 for host mode
    
    printf("[LPC55S69] USB controllers initialized successfully.\n");
}

/**
 * @brief Poll USB controllers (called periodically).
 */
void hurricane_hw_poll(void)
{
    hurricane_hw_sync_controllers();
    
    // Any additional polling for USB operation can be placed here
    // Most operations are interrupt-driven, so minimal work needed here
}
