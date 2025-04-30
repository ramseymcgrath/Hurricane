#pragma once

#include <stdint.h>

// MAX3421E Register Definitions
// R = Read, W = Write, RW = Read/Write, RC = Read with Clear

// MAX3421E Command Byte Format
#define MAX3421E_DIR_OUT        0x00
#define MAX3421E_DIR_IN         0x80
#define MAX3421E_ACKSTAT        0x40

// SPI Interface Registers
#define MAX3421E_REG_RCVFIFO    0x08    // R   Receive FIFO
#define MAX3421E_REG_SNDFIFO    0x10    // W   Send FIFO
#define MAX3421E_REG_SUDFIFO    0x20    // RW  Setup Data FIFO
#define MAX3421E_REG_RCVBC      0x30    // R   Receive Byte Count
#define MAX3421E_REG_SNDBC      0x38    // W   Send Byte Count

// Peripheral/Host Shared Registers
#define MAX3421E_REG_USBIRQ     0x68    // RC  USB Interrupt Request
#define MAX3421E_REG_USBIEN     0x70    // RW  USB Interrupt Enable
#define MAX3421E_REG_USBCTL     0x78    // RW  USB Control
#define MAX3421E_REG_CPUCTL     0x80    // RW  CPU Control
#define MAX3421E_REG_PINCTL     0x88    // RW  Pin Control

// Host Mode Registers
#define MAX3421E_REG_REVISION   0x90    // R   Revision
#define MAX3421E_REG_FNADDR     0x98    // RW  Function Address
#define MAX3421E_REG_IOPINS1    0xA0    // RW  I/O Pins 1
#define MAX3421E_REG_IOPINS2    0xA8    // RW  I/O Pins 2
#define MAX3421E_REG_GPINIRQ    0xB0    // RC  GPIO Interrupt Request
#define MAX3421E_REG_GPINIEN    0xB8    // RW  GPIO Interrupt Enable
#define MAX3421E_REG_GPINPOL    0xC0    // RW  GPIO Interrupt Polarity
#define MAX3421E_REG_HIRQ       0xC8    // RC  Host Interrupt Request
#define MAX3421E_REG_HIEN       0xD0    // RW  Host Interrupt Enable
#define MAX3421E_REG_MODE       0xD8    // RW  Host Mode
#define MAX3421E_REG_PERADDR    0xE0    // RW  Peripheral Address
#define MAX3421E_REG_HCTL       0xE8    // RW  Host Control
#define MAX3421E_REG_HXFR       0xF0    // RW  Host Transfer
#define MAX3421E_REG_HRSL       0xF8    // R   Host Result

// USBIRQ Register Bits
#define MAX3421E_USBIRQ_VBUSIRQ     0x40    // VBUS Change
#define MAX3421E_USBIRQ_NOVBUSIRQ   0x20    // No VBUS
#define MAX3421E_USBIRQ_SUSPIRQ     0x10    // Suspend
#define MAX3421E_USBIRQ_URESIRQ     0x08    // USB Reset
#define MAX3421E_USBIRQ_HSPIRQ      0x04    // High Speed
#define MAX3421E_USBIRQ_SOFIRQ      0x02    // Start of Frame
#define MAX3421E_USBIRQ_OSCOKIRQ    0x01    // Oscillator OK

// USBIEN Register Bits (same as USBIRQ bits)
#define MAX3421E_USBIEN_VBUSIE      0x40
#define MAX3421E_USBIEN_NOVBUSIE    0x20
#define MAX3421E_USBIEN_SUSPIE      0x10
#define MAX3421E_USBIEN_URESIE      0x08
#define MAX3421E_USBIEN_HSPIE       0x04
#define MAX3421E_USBIEN_SOFIE       0x02
#define MAX3421E_USBIEN_OSCOKIE     0x01

// USBCTL Register Bits
#define MAX3421E_USBCTL_CHIPRES     0x20    // Chip Reset
#define MAX3421E_USBCTL_PWRDOWN     0x10    // Power Down
#define MAX3421E_USBCTL_VBGATE      0x08    // VBUS Gate
#define MAX3421E_USBCTL_CONNECT     0x04    // Connect
#define MAX3421E_USBCTL_SIGRWU      0x02    // Signal Remote Wakeup

// PINCTL Register Bits
#define MAX3421E_PINCTL_FDUPSPI     0x10    // Full-duplex SPI
#define MAX3421E_PINCTL_INTLEVEL    0x08    // INT pin level (active low)
#define MAX3421E_PINCTL_POSINT      0x04    // INT edge (active rising edge)
#define MAX3421E_PINCTL_GPXA        0x02    // GPX pin drive (active low)
#define MAX3421E_PINCTL_GPXB        0x01    // GPX pin function (active bus activity)

// HIRQ Register Bits
#define MAX3421E_HIRQ_HXFRDNIRQ     0x80    // Host Transfer Done
#define MAX3421E_HIRQ_FRAMEIRQ      0x40    // Frame Generate
#define MAX3421E_HIRQ_CONDETIRQ     0x20    // Connect Detect
#define MAX3421E_HIRQ_SUSDNIRQ      0x10    // Suspend Operation Done
#define MAX3421E_HIRQ_SNDBAVIRQ     0x08    // SNDFIFO Buffer Available  
#define MAX3421E_HIRQ_RCVDAVIRQ     0x04    // RCVFIFO Data Available
#define MAX3421E_HIRQ_RWUIRQ        0x02    // Remote Wakeup
#define MAX3421E_HIRQ_BUSIRQ        0x01    // BUS Event

// HIEN Register Bits (same as HIRQ bits)
#define MAX3421E_HIEN_HXFRDNIE      0x80
#define MAX3421E_HIEN_FRAMEIE       0x40
#define MAX3421E_HIEN_CONDETIE      0x20
#define MAX3421E_HIEN_SUSDNIE       0x10
#define MAX3421E_HIEN_SNDBAVIE      0x08
#define MAX3421E_HIEN_RCVDAVIE      0x04
#define MAX3421E_HIEN_RWUIE         0x02
#define MAX3421E_HIEN_BUSIE         0x01

// MODE Register Bits
#define MAX3421E_MODE_HOST          0x01    // Host mode (1) or peripheral mode (0)
#define MAX3421E_MODE_SPEED         0x02    // Full speed (0) or low speed (1)
#define MAX3421E_MODE_SOFKAENAB     0x04    // Enable SOF generation when set
#define MAX3421E_MODE_HUBPRE        0x08    // Hub pre-scaler
#define MAX3421E_MODE_SEPIRQ        0x10    // Separate IRQ pin for host or peripheral mode

// HCTL Register Bits
#define MAX3421E_HCTL_BUSRST        0x01    // Bus Reset
#define MAX3421E_HCTL_FRMRST        0x02    // Frame Counter Reset
#define MAX3421E_HCTL_SAMPLEBUS     0x04    // Sample Bus
#define MAX3421E_HCTL_SIGRSM        0x08    // Signal Resume
#define MAX3421E_HCTL_RCVTOG0       0x10    // Receive Toggle 0
#define MAX3421E_HCTL_RCVTOG1       0x20    // Receive Toggle 1
#define MAX3421E_HCTL_SNDTOG0       0x40    // Send Toggle 0
#define MAX3421E_HCTL_SNDTOG1       0x80    // Send Toggle 1

// HXFR Register Bits
#define MAX3421E_HXFR_SETUP         0x00    // Setup transfer
#define MAX3421E_HXFR_IN            0x10    // IN transfer
#define MAX3421E_HXFR_OUT           0x20    // OUT transfer
#define MAX3421E_HXFR_ISO           0x40    // Isochronous transfer
#define MAX3421E_HXFR_BULK          0x80    // Bulk transfer
#define MAX3421E_HXFR_HS            0xC0    // Handshake
#define MAX3421E_HXFR_EP_MASK       0x0F    // Endpoint mask

// HRSL Register Bits
#define MAX3421E_HRSL_RCVTOGRD      0x10    // Receive toggle
#define MAX3421E_HRSL_SNDTOGRD      0x20    // Send toggle
#define MAX3421E_HRSL_KSTATUS       0x40    // K status
#define MAX3421E_HRSL_JSTATUS       0x80    // J status
#define MAX3421E_HRSL_RESULT_MASK   0x0F    // Result code mask

// HRSL Result Codes
#define MAX3421E_RESULT_SUCCESS     0x00    // Successful transfer
#define MAX3421E_RESULT_BUSY        0x01    // SIE busy
#define MAX3421E_RESULT_BADREQ      0x02    // Bad requested value
#define MAX3421E_RESULT_UNDEF       0x03    // Undefined result
#define MAX3421E_RESULT_NAK         0x04    // Endpoint NAKed
#define MAX3421E_RESULT_STALL       0x05    // Endpoint stalled
#define MAX3421E_RESULT_TOGERR      0x06    // Toggle error
#define MAX3421E_RESULT_WRONGPID    0x07    // Wrong PID
#define MAX3421E_RESULT_BADBC       0x08    // Bad byte count
#define MAX3421E_RESULT_PIDERR      0x09    // PID error
#define MAX3421E_RESULT_PKTERR      0x0A    // Packet error
#define MAX3421E_RESULT_CRCERR      0x0B    // CRC error
#define MAX3421E_RESULT_KERR        0x0C    // K error
#define MAX3421E_RESULT_JERR        0x0D    // J error
#define MAX3421E_RESULT_TIMEOUT     0x0E    // Timeout
#define MAX3421E_RESULT_BABBLE      0x0F    // Babble detected