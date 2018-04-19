#ifndef	__CH554_CONF_H__
#define __CH554_CONF_H__

#define  UART0_BUAD    57600

//#define CLOCK_FREQ_32
//#define CLOCK_FREQ_24
//#define CLOCK_FREQ_16
//#define CLOCK_FREQ_12
//#define CLOCK_FREQ_6
//#define CLOCK_FREQ_3
//#define CLOCK_FREQ_P750
//#define CLOCK_FREQ_P1875

void CH554_Init(void);

// Interrupt Handlers
void USBInterrupt(void);

// XRAM Allocation
#define EP2_RX_ADDR 		0x0000
#define EP0_ADDR 			0x0012
#define LINECODING_ADDR 	0x001C
#define EP2_TX_ADDR 		EP2_RX_ADDR+64

#define EP2_SIZE 16
#define LINECODING_SIZE 7

extern_xdatabuf(EP2_RX_ADDR, EP2_RX_BUF);
extern_xdatabuf(EP0_ADDR, Ep0Buffer);
extern_xdatabuf(LINECODING_ADDR, LineCoding);
extern_xdatabuf(EP2_TX_ADDR, EP2_TX_BUF);

#endif
