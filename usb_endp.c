#include "usb_endp.h"
#include "usb_cdc.h"

#include "ch554_platform.h"

uint8_t USB_EP_HALT_SET(uint8_t ep) {
	switch (ep) {
	case 0x82:
		UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
		return 0;
	case 0x02:
		UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
		return 0;
	case 0x83:
		UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
		return 0;
	case 0x03:
		UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;
		return 0;	
	case 0x81:
		UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;
		return 0;
	default:
		return 0xFF;
	}
}

uint8_t USB_EP_HALT_CLEAR(uint8_t ep) {
	switch (ep)
	{
	case 0x82:
		UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
		return 0;
	case 0x02:
		UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
		return 0;
	case 0x83:
		UEP3_CTRL = UEP3_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
		return 0;
	case 0x03:
		UEP3_CTRL = UEP3_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
		return 0;	
	case 0x81:
		UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
		return 0;
	default:
		return 0xFF;
	}
}
