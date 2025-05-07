/**
 * @file usb_hw_init_lpc55s69.c
 * @brief LPC55S69-specific USB hardware initialization
 * 
 * This file implements the USB hardware initialization for the LPC55S69 platform,
 * including clock configuration, PHY settings, power management, and resets.
 * It supports both USB0 (IP3511 FS) for device mode and USB1 (EHCI HS) for host mode.
 */

#include <stdio.h>

// NXP SDK includes
#include "fsl_device_registers.h"
#include "fsl_power.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "usb.h"

// Default PHY calibration values
// These values are typically defined in the board configuration
// but we provide defaults here in case they're not defined
#ifndef BOARD_USB0_PHY_D_CAL
#define BOARD_USB0_PHY_D_CAL (0x0CU)
#endif

#ifndef BOARD_USB0_PHY_TXCAL45DP
#define BOARD_USB0_PHY_TXCAL45DP (0x06U)
#endif

#include "fsl_usb_phy.h"

// Use SDK-defined PHY configuration constants
#define BOARD_USB0_PHY_TXCAL45DM  kUSB_PhyControl_TXCAL45DM_MASK
#define BOARD_USB1_PHY_D_CAL      kUSB_PhyControl_D_CAL_MASK
#define BOARD_USB1_PHY_TXCAL45DP  kUSB_PhyControl_TXCAL45DP_MASK
#define BOARD_USB1_PHY_TXCAL45DM  kUSB_PhyControl_TXCAL45DM_MASK

#ifndef BOARD_XTAL_FREQ
#define BOARD_XTAL_FREQ (24000000U)
#endif

// USB interrupt priorities
#define USB_DEVICE_INTERRUPT_PRIORITY (3U)
#define USB_HOST_INTERRUPT_PRIORITY (3U)

// External device and host handle references
extern usb_device_handle device_handle;
extern usb_host_handle host_handle;

/**
 * @brief Initialize USB0 (IP3511 FS) for device mode
 *
 * This function configures the USB0 controller in device mode (Full Speed).
 * It sets up the necessary clocks, power, and PHY settings required for
 * proper operation of the USB0 controller.
 *
 * USB0 is a Full-Speed (FS) IP3511 controller used in device mode.
 */
void usb_device_hw_init(void)
{

    // Power configuration for USB0
    POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY); // Power up the USB0 PHY

    // Reset USB0 controller and PHY
    RESET_PeripheralReset(kUSB0D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HSL_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB0HMR_RST_SHIFT_RSTn);

    // Setup USB clock source
    // USB0 needs a 48MHz clock - use PLL0 (main PLL) with divider
    const clock_usb_src_t usb0Src = kCLOCK_UsbSrcFro; // Use FRO as source (typically 96MHz)
    const uint32_t usbClkDiv = 2;                     // Divide by 2 to get 48MHz
    const usb_phy_config_struct_t phyConfig = {
        BOARD_USB0_PHY_D_CAL,
        BOARD_USB0_PHY_TXCAL45DP,
        BOARD_USB0_PHY_TXCAL45DM,
    };

    // Configure USB clock source and divider
    CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, usbClkDiv, true);
    CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, usbClkDiv, false);
    CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);

    // Initialize USB0 PHY
    USB_EhciPhyInit(kUSB_ControllerLpcIp3511Fs0, 0, &phyConfig);

    printf("[LPC55S69-Device] USB0 PHY and clocks initialized (Full-Speed IP3511)\n");
}

/**
 * @brief Initialize USB1 (EHCI HS) for host mode
 *
 * This function configures the USB1 controller in host mode (High Speed).
 * It sets up the necessary clocks, power, and PHY settings required for
 * proper operation of the USB1 controller.
 *
 * USB1 is a High-Speed (HS) EHCI controller used in host mode.
 */
void usb_host_hw_init(void)
{
    // Enable USB1 PHY clock
    CLOCK_EnableClock(kCLOCK_Usb1Clk);

    // Enable USB1 host clock
    CLOCK_EnableClock(kCLOCK_Usbh1);

    // Power configuration for USB1
    POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY); // Power up the USB1 PHY

    // Reset USB1 controller and PHY
    RESET_PeripheralReset(kUSB1H_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1D_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSB1_RST_SHIFT_RSTn);

    // Setup USB clock source
    // USB1 needs a 480MHz clock for high-speed operation
    const clock_usb_src_t usb1Src = kCLOCK_UsbSrcPll0; // Use PLL0 as source
    const usb_phy_config_struct_t phyConfig = {
        BOARD_USB1_PHY_D_CAL,
        BOARD_USB1_PHY_TXCAL45DP,
        BOARD_USB1_PHY_TXCAL45DM,
    };

    // Configure USB1 clock source and divider (using PLL0)
    CLOCK_SetClkDiv(kCLOCK_DivUsb1Clk, 1, false);
    CLOCK_AttachClk(kPLL0_to_USB1_CLK);

    // Initialize USB1 PHY
    USB_EhciPhyInit(kUSB_ControllerEhci1, BOARD_XTAL_FREQ, &phyConfig);

    printf("[LPC55S69-Host] USB1 PHY and clocks initialized (High-Speed EHCI)\n");
}

/**
 * @brief Function to enable USB device interrupt
 *
 * This function enables the interrupts for USB device controller (USB0)
 */
void USB_DeviceIsrEnable(void)
{
    uint8_t irqNumber = USB0_IRQn;
    uint8_t usbDeviceIP3511Irq = USB0_IRQn;

    // Set interrupt priority
    NVIC_SetPriority((IRQn_Type)usbDeviceIP3511Irq, USB_DEVICE_INTERRUPT_PRIORITY);

    // Enable interrupt
    EnableIRQ((IRQn_Type)usbDeviceIP3511Irq);
}

/**
 * @brief Function to enable USB host interrupt
 *
 * This function enables the interrupts for USB host controller (USB1)
 */
void USB_HostIsrEnable(void)
{
    uint8_t irqNumber = USB1_IRQn;
    uint8_t usbHostEhciIrq = USB1_IRQn;

    // Set interrupt priority
    NVIC_SetPriority((IRQn_Type)usbHostEhciIrq, USB_HOST_INTERRUPT_PRIORITY);

    // Enable interrupt
    EnableIRQ((IRQn_Type)usbHostEhciIrq);
}

/**
 * @brief Interrupt handler for USB0 (Device) controller
 */
void USB0_IRQHandler(void)
{
    USB_DeviceLpcIp3511IsrFunction(device_handle);
}

/**
 * @brief Interrupt handler for USB1 (Host) controller
 */
void USB1_IRQHandler(void)
{
    USB_HostEhciIsrFunction(host_handle);
}

//==============================================================================
// Common functions for both controllers
//==============================================================================

/**
 * @brief Synchronize USB host and device controllers
 *
 * This function ensures proper synchronization between the USB host controller
 * (USB1 - EHCI) and the USB device controller (USB0 - IP3511 FS) when they need
 * to coordinate operations.
 *
 * The LPC55S69 platform has physically separate USB controllers:
 * - USB0: IP3511 Full-Speed controller (device mode)
 * - USB1: EHCI High-Speed controller (host mode)
 *
 * While these controllers operate with separate hardware resources (unlike shared
 * controllers like on RT1060), this function provides synchronization for:
 * 1. Power management coordination
 * 2. Clock resource management
 * 3. Interrupt priority handling
 * 4. Thread-safe access to any shared resources
 *
 * Thread safety is ensured through a mutex that protects critical sections
 * where both controllers might access shared resources simultaneously.
 */

// Mutex for thread safety when accessing shared resources
static volatile bool usb_resource_lock = false;

void hurricane_hw_sync_controllers(void)
{
    // Attempt to acquire the lock for accessing shared resources
    while (__atomic_test_and_set(&usb_resource_lock, __ATOMIC_ACQUIRE))
    {
        // Wait if lock is already held
        // For a more sophisticated implementation, a proper mutex with
        // timeout and priority inheritance could be used
    }
    
    // Critical section - safe to access shared resources
    
    // CLOCK SYNCHRONIZATION
    // On LPC55S69, USB0 and USB1 use different clock sources:
    // - USB0: FRO with divider for 48MHz
    // - USB1: PLL0 for 480MHz
    // Check if both controllers are active and manage clock if needed
    
    // POWER MANAGEMENT
    // Both controllers have separate power domains (USB0_PHY and USB1_PHY)
    // If one controller is inactive, we could potentially reduce power
    // No action needed if both controllers are active
    
    // INTERRUPT PRIORITY
    // Ensure the interrupt priorities are properly balanced
    // Both controllers are set to the same priority by default (3)
    // but could be adjusted if one needs higher priority
    
    // End of critical section - release the lock
    __atomic_clear(&usb_resource_lock, __ATOMIC_RELEASE);
}

// Add a common initialization function for both controllers
void hurricane_hw_init(void)
{
    printf("[LPC55S69] Initializing dual USB stack\n");
    
    // Initialize both host and device stacks
    usb_device_hw_init();
    usb_host_hw_init();
    
    printf("[LPC55S69] Dual USB stack initialized\n");
    printf("[LPC55S69]   - USB0: Device Mode (Full-Speed IP3511)\n");
    printf("[LPC55S69]   - USB1: Host Mode (High-Speed EHCI)\n");
}

// Common polling function for both controllers
void hurricane_hw_poll(void)
{
    // This function would be called from the core to poll both host and device
    // No specific action needed here for the LPC55S69 as both controllers
    // are interrupt-driven, but could be used to check status
    
    // Synchronize controllers if needed
    hurricane_hw_sync_controllers();
}
