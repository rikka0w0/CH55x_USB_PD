#ifndef __USB_CDC_H
#define __USB_CDC_H

#include "ch554_platform.h"

// CDC bRequests:
// bmRequestType = 0xA1
#define SERIAL_STATE 0x20
#define GET_LINE_CODING 0X21			// This request allows the host to find out the currently configured line coding.
// bmRequestType = 21
#define SET_LINE_CODING 0X20			// Configures DTE rate, stop-bits, parity, and number-of-character
#define SET_CONTROL_LINE_STATE 0X22			// This request generates RS-232/V.24 style control signals.
#define SEND_BREAK 0x23

// CDC Rx state machine
#define CDC_STATE_IDLE 0

extern uint32_t CDC_Baud;


#define CDC_PUTCHARBUF_LEN  16

void CDC_InitBaud(void);
void CDC_SetBaud(void);
void CDC_USB_Poll(void);
void CDC_UART_Poll(void);

void CDC_PutChar(uint8_t tdata);
void CDC_Puts(char *str);
void CDC_Hex(uint8_t val);
void CDC_HexDump(uint8_t* addr, uint8_t len);

#endif
