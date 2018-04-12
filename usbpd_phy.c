#include "ch554_platform.h"
#include "usbpd_phy.h"
#include "usb_cdc.h"
#include "delay.h"

// CC1 - P1.4
// CC2 - P1.5



uint8_t USBPD_ConnectionStatus;

void USBPD_Rx_Begin(void) {
	// Config interrupt on P1.5 and P1.4 (CC lines), sensitive to falling edge
	GPIO_IE = bIE_IO_EDGE | bIE_P1_5_LO | bIE_P1_4_LO;

	// Adjust priority
	IP_EX |= bIP_GPIO;

	// Enable GPIO interrupt
	IE_GPIO = 1;
}

void USBPD_Rx_InterruptHandler(void) {
	// Disable GPIO interrupt
	IE_GPIO = 0;

	// Configure Comparator
	ADC_CFG |= bCMP_EN;	// Enable Comparator
	// Connect CC to non-inverting input of the comparator
	if (USBPD_ConnectionStatus & USBPD_CO_CC1) {
		ADC_CHAN1 = 0;
		ADC_CHAN0 = 1;
	} else {
		ADC_CHAN1 = 1;
		ADC_CHAN0 = 0;
	}
	CMP_CHAN = 1;	// Connect P3.2 (AIN3) to inverting input of the comparator
	CMP_IF = 0;

	// Configure Timer0
	// ftim0 = fsys
	T2MOD |= bTMR_CLK | bT0_CLK;
	T2MOD &=~ bT0_CT;
	// Ignore INT0
	TMOD &=~ (bT0_GATE | bT0_M1);
	TMOD |= bT0_M0;
	// When maximum packet time reached, timer1 overflows it generates a interrupt
	TH0 = 233;
	TL0 = 233;
	TR0 = 1;
	ET0 = 1;


	CDC_Puts("QAQ");
}

/**
 * Detects cable orientation, Current capacity of UFP
 */
void USBPD_DFP_CC_Detect(void) {
	uint8_t ccv;

	// Enable ADC
	ADC_CFG |= bADC_EN | bADC_CLK;

	// P1.4
	ADC_CHAN1 = 0;
	ADC_CHAN0 = 1;
	mDelayuS(2);
	ADC_START = 1;
	while (ADC_START);
	ccv = ADC_DATA;

	// P1.5
	ADC_CHAN1 = 1;
	ADC_CHAN0 = 0;
	mDelayuS(2);
	ADC_START = 1;
	while (ADC_START);

	USBPD_ConnectionStatus = USBPD_CO_NC | USBPD_PWR_DEFAULT;
	if (ccv >= USBPD_CCV_DEFAULT_MIN) {		// CC1
		USBPD_ConnectionStatus |= USBPD_CO_CC1;
	}
	if (ADC_DATA >= USBPD_CCV_DEFAULT_MIN) {// CC2
		USBPD_ConnectionStatus |= USBPD_CO_CC2;
		ccv = ADC_DATA;
	}

	// ccv represents the voltage of active CC line
	if((ccv >= USBPD_CCV_3A0_MIN) && (ccv <= USBPD_CCV_3A0_MAX)) {
		USBPD_ConnectionStatus |= USBPD_PWR_3A0;
	} else if((ccv >= USBPD_CCV_1A5_MIN) && (ccv <= USBPD_CCV_1A5_MAX)) {
		USBPD_ConnectionStatus |= USBPD_PWR_1A5;
    }

	// Disable ADC
	ADC_CFG &= ~bADC_EN;
}

void USBPD_DFP_Init(void) {
	// All GPIOs are high impedance after power up

	// 1. Config CCx_MCU and CC_EN to open-collector mode, no pull-up
	// Pn_DIR_PU = 0

	// 2. Set Rp_DIS to 0 before config Rp_DIS to push-pull
	// Rp_DIS = 0;
	// Pn_MOD_OC = 0;

	// 3. Enable CC connection to MCU
	// CC_EN = 0;
}
