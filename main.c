#include "Delay.H"
#include "usb_cdc.h"

#include "ch554_platform.h"

extern void USBPD_Rx_Begin(void);
extern void USBPD_Rx_InterruptHandler(void);

void main(void) {
	CDC_InitBaud();
    CH554_Init();
	
    USBPD_Rx_Begin();

    while(1) {
    	CDC_USB_Poll();
    	CDC_UART_Poll();
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
