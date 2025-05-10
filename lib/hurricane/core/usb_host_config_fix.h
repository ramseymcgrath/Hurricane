/**
 * @file usb_host_config_fix.h
 * @brief Additional USB Host configuration macros
 * 
 * This file provides additional USB Host configuration macros that are 
 * required by the NXP SDK but not included in the auto-generated config file.
 */

#ifndef USB_HOST_CONFIG_FIX_H
#define USB_HOST_CONFIG_FIX_H

/* Define the missing USB host configuration macros */
#define USB_HOST_CONFIG_MAX_TRANSFERS                     (8U)
#define USB_HOST_CONFIG_MAX_HOST                          (1U)
#define USB_HOST_CONFIG_ENUMERATION_MAX_STALL_RETRIES     (3U)
#define USB_HOST_CONFIG_ENUMERATION_MAX_RETRIES           (3U)
#define USB_HOST_CONFIG_MAX_NAK                           (10U)
#define USB_HOST_CONFIG_CONFIGURATION_MAX_INTERFACE       USB_HOST_CONFIG_MAX_INTERFACES

#endif /* USB_HOST_CONFIG_FIX_H */