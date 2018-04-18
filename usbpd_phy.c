#include "ch554_platform.h"
#include "usbpd_phy.h"
#include "usb_cdc.h"
#include "delay.h"

// CC1 - P1.4
// CC2 - P1.5
SBIT(USBPD_CC_PIN, GPIO1, 4);
SBIT(LED, GPIO1, 7);

xdatabuf(0xD1, USBPD_RawDatBuf, 512);
uint8_t* USBPD_RawDatBuf_End;
uint8_t USBPD_ConnectionStatus;
uint8_t USBPD_PortStatus;

void USBPD_Rx_Begin(void) {
	// Config interrupt on P1.4, sensitive to falling edge
	GPIO_IE = bIE_IO_EDGE | bIE_P1_4_LO;
	// Adjust priority
	IP_EX |= bIP_GPIO;

	// Configure Timer0
	ET0 = 0;	// Disable Timer0 interrupt
	TR0 = 0;	// Disable Timer0
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

	// Enable GPIO interrupt
	IE_GPIO = 1;

	CDC_PutChar(USBPD_ConnectionStatus);
}

void USBPD_Rx_InterruptHandler(void) {
	GPIO_IE &= ~(bIE_IO_EDGE | bIE_P1_4_LO);
	IE_GPIO = 0;

	TR0 = 1;	// Enable Timer0

	__asm
	mov	dpl,#_USBPD_RawDatBuf
	mov	dph,#(_USBPD_RawDatBuf >> 8)
	__endasm;

	for (uint8_t i=0; i<60; i++) {
		while (USBPD_CC_PIN && !TF0);
		__asm
		clr	_LED
		mov a, _TL0
		movx @dptr, a
		inc dptr
		setb _LED
		__endasm;

		while (!USBPD_CC_PIN && !TF0);
		__asm
		clr	_LED
		mov a, _TL0
		movx @dptr, a
		inc dptr
		setb _LED
		__endasm;

		while (USBPD_CC_PIN && !TF0);
		__asm
		clr	_LED
		mov a, _TL0
		movx @dptr, a
		inc dptr
		setb _LED
		__endasm;

		while (!USBPD_CC_PIN && !TF0);
		__asm
		clr	_LED
		mov a, _TL0
		movx @dptr, a
		inc dptr
		setb _LED
		__endasm;
	}


	__asm
		mov	_USBPD_RawDatBuf_End,_DPL
		mov (_USBPD_RawDatBuf_End+1), _DPH
	__endasm;

	TR0 = 0;


	USBPD_PortStatus = USBPD_STATUS_RECEIVED;
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
	P1_DIR_PU &=~ 0x20;	// CCx_MCU to open-collector

	// 2. Set Rp_DIS to 0 before config Rp_DIS to push-pull
	// Rp_DIS = 0;
	// Pn_MOD_OC = 0;

	// 3. Enable CC connection to MCU
	// CC_EN = 0;
}

void USBPD_DFP_CC_Poll() {
	uint8_t len;
	uint8_t last;
	uint8_t cur;
	if (USBPD_PortStatus == USBPD_STATUS_WAITING) {
		USBPD_DFP_CC_Detect();
		if (USBPD_ConnectionStatus & USBPD_CO_CC2) {
			mDelayuS(50);
			USBPD_DFP_CC_Detect();
			if (USBPD_ConnectionStatus & USBPD_CO_CC2) {
				// Configure Timer0
				// ftim0 = fsys/12 = 2MHz
				T2MOD &=~ bT0_CLK;
				TMOD &=~ bT0_CT;
				// Ignore INT0, Mode 1 (16-bit)
				TMOD &=~ (bT0_GATE | bT0_M1);
				TMOD |= bT0_M0;
				// When maximum packet time reached, timer1 overflows it generates a interrupt
				TH0 = 0x0;	// 0x0000 -> 0xFFFF
				TL0 = 0x0;	// 32.7675 mS
				TF0 = 0;
				ET0 = 1;

				USBPD_PortStatus = USBPD_STATUS_ASSERTING;
				TR0 = 1;	// Enable Timer0
			}
		}
	} else if (USBPD_PortStatus == USBPD_STATUS_RECEIVED) {
		USBPD_PortStatus = USBPD_STATUS_IDLE;

		len = USBPD_RawDatBuf_End - USBPD_RawDatBuf;
		CDC_PutChar(len);

		last = USBPD_RawDatBuf[0];
		for (uint8_t i=1; i<len; i++) {
			cur = USBPD_RawDatBuf[i];
			if (cur > last) {
				last = cur - last;
			} else {
				last = 255 - last;
				last += cur;
				last ++;
			}

			// Decision rule: (3.03+3.07)/2/2*1.5=2.52375uS, ~60 cycles
			if (last < 60) {
				i++;
				// Assume next is another narrow pulse
				// Two narrow pulse -> An change in the middle of a frame-> 1
				last = USBPD_RawDatBuf[i];
				CDC_PutChar(1);
			} else {
				last = cur;
				CDC_PutChar(0);
			}


		}
	}
}

void USBPD_DFP_CC_Assert() {
	ET0 = 0;	// Disable Timer0 interrupt
	TR0 = 0;	// Disable Timer0
	TF0 = 0;	// Clear overflow flag

	if (USBPD_PortStatus == USBPD_STATUS_ASSERTING) {
		USBPD_DFP_CC_Detect();
		if (USBPD_ConnectionStatus & USBPD_CO_CC2) {
			mDelayuS(50);
			USBPD_DFP_CC_Detect();
			if (USBPD_ConnectionStatus & USBPD_CO_CC2) {
				USBPD_PortStatus = USBPD_STATUS_CONNECTED;
				USBPD_Rx_Begin();
				return;
			}
		}

		USBPD_PortStatus = USBPD_STATUS_WAITING;
	}
}

/*
 * 		__asm
		mov a, _TL0
		mov r6, a	// cur = TL0

		clr	c
		subb a,r5	// > last
		mov	r5,a	// last = cur - last
		jnc	90001$		// If cur>last Jump to 90001$
		mov a, #255
		clr c
		subb a, r5	// last = 255 - last
		inc a		// last ++

		90001$: // a = abs(cur - last)
		clr c
		subb a, #60	// CY = 1 -> abs()<60
		clr a
		rlc a
		movx @dptr, a
		inc dptr

		mov a, r6
		mov r5, a

		setb _LED
		__endasm;
 */
