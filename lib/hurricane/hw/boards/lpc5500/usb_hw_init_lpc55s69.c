/**
 * @file usb_hw_init_lpc55s69.c
 * @brief LPC55S69-specific USB hardware initialization for device (USB0) and host (USB1) controllers.
 */

 #include <stdio.h>
 #include <stdatomic.h>
 #include "fsl_device_registers.h"
 #include "fsl_power.h"
 #include "fsl_clock.h"
 #include "fsl_reset.h"
 #include "usb.h"
 #include "usb_phy.h"
 #include "clock_config.h"
 #include "PERI_USBPHY.h"
 
 #ifndef BOARD_USB_PHY_D_CAL
 #define BOARD_USB_PHY_D_CAL      (0x0CU)
 #endif
 #ifndef BOARD_USB_PHY_TXCAL45DP
 #define BOARD_USB_PHY_TXCAL45DP  (0x06U)
 #endif
 #ifndef BOARD_USB_PHY_TXCAL45DM
 #define BOARD_USB_PHY_TXCAL45DM  BOARD_USB_PHY_TXCAL45DP
 #endif
 #ifndef BOARD_XTAL_FREQ
 #define BOARD_XTAL_FREQ         (24000000U)
 #endif
 
 #define USB_IRQ_PRIORITY        (3U)
 
 extern usb_device_handle device_handle;
 extern usb_host_handle   host_handle;
 
 /* Forward-declare ISR functions if not included elsewhere */
 extern void USB_DeviceLpcIp3511IsrFunction(void *deviceHandle);
 extern void USB_HostEhciIsrFunction(void *hostHandle);
 
 /**
  * @brief Initialize USB0 (IP3511 FS) for device mode.
  */
 void usb_device_hw_init(void)
 {
     /* Power and reset */
     POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY);
     RESET_PeripheralReset(kUSB0D_RST_SHIFT_RSTn);
     RESET_PeripheralReset(kUSB0HSL_RST_SHIFT_RSTn);
     RESET_PeripheralReset(kUSB0HMR_RST_SHIFT_RSTn);
 
     /* Clock setup: FRO (96MHz) / 2 -> 48MHz */
     CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 2U, true);
     CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
 
     /* PHY configuration */
     usb_phy_config_struct_t phyCfg;
     phyCfg.D_CAL     = BOARD_USB_PHY_D_CAL;
     phyCfg.TXCAL45DP = BOARD_USB_PHY_TXCAL45DP;
     phyCfg.TXCAL45DM = BOARD_USB_PHY_TXCAL45DM;
     USB_EhciPhyInit(kUSB_ControllerLpcIp3511Fs0, BOARD_XTAL_FREQ, &phyCfg);
 
     printf("[LPC55S69] USB0 initialized: FS IP3511 (48MHz)\n");
 }
 
 /**
  * @brief Initialize USB1 (EHCI HS) for host mode.
  */
 void usb_host_hw_init(void)
 {
     /* Power, clocks, and reset */
     POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY);
     CLOCK_EnableClock(kCLOCK_Usb1Clk);
     CLOCK_EnableClock(kCLOCK_Usbh1);
     RESET_PeripheralReset(kUSB1H_RST_SHIFT_RSTn);
     RESET_PeripheralReset(kUSB1D_RST_SHIFT_RSTn);
     RESET_PeripheralReset(kUSB1_RST_SHIFT_RSTn);
 
     /* Enable PHY PLL and host clocks */
     CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_UsbPhySrcExt, BOARD_XTAL_FREQ);
     CLOCK_EnableUsbhs0HostClock(kCLOCK_UsbSrcUnused, 0U);
 
     /* PHY configuration */
     usb_phy_config_struct_t phyCfg;
     phyCfg.D_CAL     = BOARD_USB_PHY_D_CAL;
     phyCfg.TXCAL45DP = BOARD_USB_PHY_TXCAL45DP;
     phyCfg.TXCAL45DM = BOARD_USB_PHY_TXCAL45DM;
     USB_EhciPhyInit(kUSB_ControllerEhci1, BOARD_XTAL_FREQ, &phyCfg);
 
     printf("[LPC55S69] USB1 initialized: HS EHCI (480MHz)\n");
 }
 
 /**
  * @brief Enable USB0 interrupt.
  */
 void USB_DeviceIsrEnable(void)
 {
     NVIC_SetPriority(USB0_IRQn, USB_IRQ_PRIORITY);
     EnableIRQ(USB0_IRQn);
 }
 
 /**
  * @brief Enable USB1 interrupt.
  */
 void USB_HostIsrEnable(void)
 {
     NVIC_SetPriority(USB1_IRQn, USB_IRQ_PRIORITY);
     EnableIRQ(USB1_IRQn);
 }
 
 void USB0_IRQHandler(void)
 {
     USB_DeviceLpcIp3511IsrFunction(device_handle);
 }
 
 void USB1_IRQHandler(void)
 {
     USB_HostEhciIsrFunction(host_handle);
 }
 
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
  */
 void hurricane_hw_init(void)
 {
     printf("[LPC55S69] Initializing USB controllers...\n");
     usb_device_hw_init();
     usb_host_hw_init();
     printf("[LPC55S69] USB controllers initialized.\n");
 }
 
 /**
  * @brief Poll USB controllers (called periodically).
  */
 void hurricane_hw_poll(void)
 {
     hurricane_hw_sync_controllers();
 }
 