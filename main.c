#include "Delay.H"
#include "usb_cdc.h"
#include "usbpd_phy.h"

#include "ch554_platform.h"

extern uint8_t USBPD_PortStatus;

void main(void) {
	CDC_InitBaud();
    CH554_Init();
	
    USBPD_DFP_Init();
    USBPD_PortStatus = USBPD_STATUS_WAITING;

    while(1) {
    	CDC_USB_Poll();
    	CDC_UART_Poll();
    	USBPD_DFP_CC_Poll();
    };
}

/*
 * According to SDCC specification, interrupt handlers MUST be placed within the file which contains
 * the void main(void) function, otherwise SDCC won't be able to recognize it. It's not a bug but a feature.
 * If you know how to fix this, please let me know.
 */
void USBInterruptEntry(void) interrupt INT_NO_USB {
	USBInterrupt();
}

void GPIOInterruptEntry(void) interrupt INT_NO_GPIO {
	USBPD_Rx_InterruptHandler();
}

void Timer0InterruptEntry(void) interrupt INT_NO_TMR0 {
	USBPD_DFP_CC_Assert();
}
