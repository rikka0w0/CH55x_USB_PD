#include "ch554_platform.h"
#include "usbpd_phy.h"
#include "usb_cdc.h"
#include "delay.h"

// CC1 - P1.4
// CC2 - P1.5


xdatabuf(0xD1, USBPD_RawDatBuf, 512);
uint8_t USBPD_ConnectionStatus;
uint8_t USBPD_RawDatBuf_Ptr;

void USBPD_Rx_Begin(void) {
	// Config interrupt on P1.5 and P1.4 (CC lines), sensitive to falling edge
	//GPIO_IE = bIE_IO_EDGE | bIE_P1_5_LO | bIE_P1_4_LO;
	GPIO_IE = bIE_IO_EDGE | bIE_P1_5_LO;

	// Adjust priority
	IP_EX |= bIP_GPIO;

	// Enable GPIO interrupt
	IE_GPIO = 1;
}

void USBPD_Rx_InterruptHandler(void) {
	static uint16_t last_cnt;
	static uint16_t this_cnt;
	static uint16_t this_cnt2;
	static uint16_t this_cnt3;
	static uint8_t last_half_1;
	
	GPIO_IE &= ~bIE_IO_EDGE;	// Set GPIO to level-sensitive interrupt mode to clear the interrupt
	IE_GPIO = 0;	// Disable GPIO interrupt
	
	USBPD_ConnectionStatus = USBPD_CO_CC2;//P1.5 debug
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
	CMP_CHAN = 0; // P1.4 AIN1
	//CMP_CHAN = 1;	// Connect P3.2 (AIN3) to inverting input of the comparator


	// Configure Timer0
	// ftim0 = fsys = 24MHz
	T2MOD |= bTMR_CLK | bT0_CLK;
	TMOD &=~ bT0_CT;
	// Ignore INT0
	TMOD &=~ (bT0_GATE | bT0_M1);
	TMOD |= bT0_M0;
	// When maximum packet time reached, timer1 overflows it generates a interrupt
	TH0 = 0x6B;	// Max 429bit (4b5b), 3.03 to 3.7us per 4b5b bit (~1.6ms)
	TL0 = 0x27;	// 3.7us * 429 * 24Mhz = 38096 cycles, TH0 TL0 = 0xFFFF-0x94D0 = 0x6B2F
	TF0 = 0;	// Clear overflow flag

	TR0 = 1;	// Enable Timer0

	// Decision rule: (3.03+3.07)/2/2*1.5=2.52375uS, ~60 cycles
	//USBPD_RawDatBuf_Ptr = 0;
	//last_half_1 = 0;

	//for (uint8_t i=0; i<16;i++) {
		CMP_IF = 0;
		while(!CMP_IF);
		CMP_IF = 0;
		TR0 = 0;
		last_cnt = (TH0<<8)|TL0;
		TR0 = 1;

		while(!CMP_IF);
		CMP_IF = 0;
		TR0 = 0;
		this_cnt = (TH0<<8)|TL0;
		TR0 = 1;

		while(!CMP_IF);
		CMP_IF = 0;
		TR0 = 0;
		this_cnt2 = (TH0<<8)|TL0;
		TR0 = 1;

		while(!CMP_IF);
		CMP_IF = 0;
		TR0 = 0;
		this_cnt3 = (TH0<<8)|TL0;
		TR0 = 1;
	//}

	//for (uint8_t i=0; i<16;i++) {
	//	CDC_Hex(USBPD_RawDatBuf[i]);
	//}

	// Disable ADC
	ADC_CFG &= ~bCMP_EN;
	last_half_1 = TF0;

	CDC_Puts("QAQ\r\n");
	CDC_Hex((last_cnt>>8)&0xFF);
	CDC_Hex(last_cnt&0xFF);
	CDC_Hex((this_cnt>>8)&0xFF);
	CDC_Hex(this_cnt&0xFF);
	CDC_Hex((this_cnt2>>8)&0xFF);
	CDC_Hex(this_cnt2&0xFF);
	CDC_Hex((this_cnt3>>8)&0xFF);
	CDC_Hex(this_cnt3&0xFF);
	CDC_Hex(last_half_1);

	/*
	 * 		TR0 = 0;
		this_cnt = (TH0 << 8) | TL0;
		TR0 = 1;
		if (this_cnt-last_cnt > 60) {	// 4b5b 0
			USBPD_RawDatBuf[USBPD_RawDatBuf_Ptr++] = 0;
			last_half_1 = 0;
		} else if (last_half_1){	// part of 4b5b 1
			USBPD_RawDatBuf[USBPD_RawDatBuf_Ptr++] = 1;
			last_half_1 = 0;
		} else {
			last_half_1 = 1;
		}
		last_cnt=this_cnt;
	 */
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
	P1_DIR_PU &=~ 0x30;	// CCx_MCU to open-collector

	// 2. Set Rp_DIS to 0 before config Rp_DIS to push-pull
	// Rp_DIS = 0;
	// Pn_MOD_OC = 0;

	// 3. Enable CC connection to MCU
	// CC_EN = 0;
}
