#ifndef __USBPD_PHY_H
#define __USBPD_PHY_H

/*
 * Maximum size of a Power Delivery packet (in bits on the wire) :
 *    16-bit header + 0..7 32-bit data objects  (+ 4b5b encoding)
 * 64-bit preamble + SOP (4x 5b) + message in 4b5b + 32-bit CRC + EOP (1x 5b)
 * = 64 + 4*5 + 16 * 5/4 + 7 * 32 * 5/4 + 32 * 5/4 + 5
 */
#define USBPD_BIT_LEN 429
#define USBPD_MAX_RAW_SIZE (USBPD_BIT_LEN*2)


// USBPD_ConnectionStatus
// Cable orientation
#define USDPD_CO_MASK 	0x03
#define USBPD_CO_NC 	0x00
#define USBPD_CO_CC1 	0x01		// P1.4
#define USBPD_CO_CC2 	0x02		// P1.5
#define USBPD_CO_ERR	0x03		// If USBPD_CO == 0x03, USBPD_PWR is meaning less
// Host current capacity
#define USBPD_PWR_MASK 		0x0C
#define USBPD_PWR_DEFAULT	0x00
#define USBPD_PWR_1A5		0x04
#define USBPD_PWR_3A0		0x08
#define USBPD_PWR_UNDEF		0x0C
// Port Status
#define USBPD_STATUS_MASK		0xF0
#define USBPD_STATUS_IDLE		0x00
#define USBPD_STATUS_WAITING	0x10
#define USBPD_STATUS_ASSERTING	0x20
#define USBPD_STATUS_CONNECTED	0x30
#define USBPD_STATUS_RECEIVED	0x40


// UFP CC line threshold value
//	ICapaticy 	CC_Min		CC_Max
//	Default		0.25V		0.61V
//	1.5A		0.70V		1.16V
//	3.0A		1.31V		2.04V
#define  USBPD_CCV_DEFAULT_MIN  13
#define  USBPD_CCV_DEFAULT_MAX  31
#define  USBPD_CCV_1A5_MIN  36
#define  USBPD_CCV_1A5_MAX  59
#define  USBPD_CCV_3A0_MIN  67
#define  USBPD_CCV_3A0_MAX  104


void USBPD_Rx_Begin(void);
void USBPD_Rx_InterruptHandler(void);
void USBPD_DFP_CC_Detect(void);
void USBPD_DFP_Init(void);
void USBPD_DFP_CC_Poll(void);
void USBPD_DFP_CC_Assert(void);

#endif /* __USBPD_PHY_H */
