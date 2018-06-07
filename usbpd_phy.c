#include "ch554_platform.h"
#include "usbpd_phy.h"
#include "usb_cdc.h"
#include "crc32.h"
#include "delay.h"

#define TABLE_5b4b_ERR 0x90
#define TABLE_5b4b_SYNC1 0xA0
#define TABLE_5b4b_SYNC2 0xB0
#define TABLE_5b4b_SYNC3 0xC0
#define TABLE_5b4b_RST1 0xD0
#define TABLE_5b4b_RST2 0xE0
#define TABLE_5b4b_EOP 0xF0
code const uint8_t table_5b4b[] = {
	TABLE_5b4b_ERR, TABLE_5b4b_ERR,
	TABLE_5b4b_ERR,	TABLE_5b4b_ERR,
	TABLE_5b4b_ERR, TABLE_5b4b_ERR,
	TABLE_5b4b_SYNC3, TABLE_5b4b_RST1,
	TABLE_5b4b_ERR, 1,
	4, 5,
	TABLE_5b4b_ERR, TABLE_5b4b_EOP,
	6, 7,

	TABLE_5b4b_ERR, TABLE_5b4b_SYNC2,
	8, 9,
	2, 3,
	0x0A, 0x0B,
	TABLE_5b4b_SYNC1, TABLE_5b4b_RST2,
	0x0C, 0x0D,
	0x0E, 0x0F,
	0, TABLE_5b4b_ERR
};

// CC1 - P1.4
// CC2 - P1.5
SBIT(USBPD_CC_PIN, GPIO1, 4);
SBIT(LED, GPIO1, 7);

xdatabuf(0xD1, PD_RawDatBuf, PD_MAX_RAW_SIZE);
uint8_t* PD_RawDatBuf_End;
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
	F1 = 0;
	__asm
	mov	dpl,#_PD_RawDatBuf
	mov	dph,#(_PD_RawDatBuf >> 8)
	__endasm;

	for (uint8_t i=0; i<PD_MAX_RAW_SIZE/4; i++) {
		while (USBPD_CC_PIN && !TF0);
		__asm
		mov a, _TL0
		movx @dptr, a
		inc dptr
		__endasm;

		while (!USBPD_CC_PIN && !TF0);
		__asm
		mov a, _TL0
		movx @dptr, a
		inc dptr
		__endasm;

		while (USBPD_CC_PIN && !TF0);
		__asm
		mov a, _TL0
		movx @dptr, a
		inc dptr
		__endasm;

		while (!USBPD_CC_PIN && !TF0);
		__asm
		mov a, _TL0
		movx @dptr, a
		inc dptr
		__endasm;
	}


	__asm
		mov	_PD_RawDatBuf_End,_DPL
		mov (_PD_RawDatBuf_End+1), _DPH
	__endasm;

	TR0 = 0;
	F1 = TF0;

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

void PD_Rx_Decode() {
	uint8_t* header_offset;
	uint8_t* rx_ptr = PD_RawDatBuf;
	uint8_t byte, nibble;

	uint8_t cur;
	uint32_t all = 0;
	uint8_t last = *PD_RawDatBuf;

	byte = PD_RX_ERR_UNKNOWN;
	header_offset = PD_RawDatBuf + 1;
	for (; header_offset < PD_RawDatBuf_End; header_offset++) {
		cur = *header_offset;

		// last = abs (cur-last)
		if (cur > last) {
			last = cur - last;
		}
		else {
			last = 255 - last;
			last += cur;
			last++;
		}

		all >>= 1;

		if (last < 60)
			all |= 0x80000000;

		last = cur;
		if (all == 0xC7E3C78D) { // SOP, Sync2 Sync1 Sync1 Sync1 101
			byte = PD_RX_SOP;
			break;
		}
		if (all == 0xF33F3F3F) {
			byte = PD_RX_ERR_HARD_RESET;
			break;
		}
		if (all == 0x3c7fe0ff) {
			byte = PD_RX_ERR_CABLE_RESET;
			break;
		}
	}

	*rx_ptr = byte;
	if (byte != PD_RX_SOP)
		return;
	LED=1;
	// Decoding
	*rx_ptr = PD_RX_ERR_UNKNOWN;
	rx_ptr++;
	header_offset++;
	while (header_offset < PD_RawDatBuf_End) {
		nibble = 0;
		for (uint8_t i = 0; i < 5; i++) {
			cur = *header_offset;
			if (cur > last) {
				last = cur - last;
			}
			else {
				last = 255 - last;
				last += cur;
				last++;
			}

			if (last < 60) {
				header_offset++;
				last = *header_offset;
				nibble |= 1 << i;
			}
			else {
				last = cur;
			}
			header_offset++;
		}

		nibble = table_5b4b[nibble];
		if (nibble > 15) {
			if (nibble == TABLE_5b4b_EOP)
				*PD_RawDatBuf = rx_ptr - PD_RawDatBuf - 1;
			return;
		}

		byte = 0;
		for (uint8_t i = 0; i < 5; i++) {
			cur = *header_offset;
			if (cur > last) {
				last = cur - last;
			}
			else {
				last = 255 - last;
				last += cur;
				last++;
			}

			if (last < 60) {
				header_offset++;
				last = *header_offset;
				byte |= 1 << i;
			}
			else {
				last = cur;
			}
			header_offset++;
		}

		byte = table_5b4b[byte];
		if (nibble > 15)
			return;

		*rx_ptr = byte << 4 | nibble;
		rx_ptr++;
	}
}

void USBPD_DFP_CC_Poll() {
	uint8_t len;
	uint32_t crc;
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
		IE=0;
		LED=0;
		PD_Rx_Decode();
		len = *PD_RawDatBuf;
		crc = crc32_fast(PD_RawDatBuf + 1, len - 4);
		IE=1;
		for (uint8_t i=0; i<=len; i++) {
			CDC_Hex(PD_RawDatBuf[i]);
		}
		if (
				U32B0(crc) == PD_RawDatBuf[len-3] &&
				U32B1(crc) == PD_RawDatBuf[len-2] &&
				U32B2(crc) == PD_RawDatBuf[len-1] &&
				U32B3(crc) == PD_RawDatBuf[len])
			CDC_Puts("GoodCRC\n");
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
