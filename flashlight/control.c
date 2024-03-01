#include "f_cpu.h"

#include <avr/io.h>
#include <util/delay.h>

#include "control.h"

/*
nINV: PA1 (output, pulls OPAMP- down, then input/high-Z)
INV: PA2 (output, drives N-FET gate)
HDR: PA3 (output, drives N-FET gate)
nHDR: PA4 (output, hacked to drive N-FET gate)
EN: PA7 (output, enables op-amp and MP3432)
*/

#define nINV_PORT PORTA
#define nINV_PIN PIN1_bm
#define INV_PORT PORTA
#define INV_PIN PIN2_bm

#define HDR_PORT PORTA
#define HDR_PIN PIN3_bm
#define nHDR_PORT PORTA
#define nHDR_PIN PIN4_bm

#define EN_PORT PORTA
#define EN_PIN PIN7_bm

#define USE_STARTUP_FLASH_FIX 1
#define FLASH_FIX_PRE_DELAY_MS 1
#define FLASH_FIX_POST_DELAY_MS 10

static bool uvlo = false;
static state_t boost_state = INVALID;
static state_t hdr_state = INVALID;

bool get_uvlo() {
	return uvlo;
}

void set_uvlo() {
	uvlo = true;
}

void reset_uvlo() {
	uvlo = false;
}

void enable_inv() {
	INV_PORT.DIRSET = INV_PIN;
	INV_PORT.OUTSET = INV_PIN;
	nINV_PORT.DIRSET = nINV_PIN;
	nINV_PORT.OUTCLR = nINV_PIN;
}

void disable_inv() {
	INV_PORT.DIRCLR = INV_PIN;
	nINV_PORT.DIRCLR = nINV_PIN;
}

state_t get_hdr_state() {
	return hdr_state;
}

void enable_hdr() {
	if (hdr_state == ENABLED) {
		return;
	}
	HDR_PORT.DIRSET = HDR_PIN;
	HDR_PORT.OUTSET = HDR_PIN;
	// Hardware hacked, nHDR drives N-FET with pull-down
	nHDR_PORT.DIRCLR = nHDR_PIN;
	hdr_state = ENABLED;
}

void disable_hdr() {
	if (hdr_state == DISABLED) {
		return;
	}
	// Hardware hacked, gate pull-up removed, drive required
	HDR_PORT.DIRSET = HDR_PIN;
	HDR_PORT.OUTCLR = HDR_PIN;
	// Hardware hacked, nHDR drives N-FET
	nHDR_PORT.DIRSET = nHDR_PIN;
	nHDR_PORT.OUTSET = nHDR_PIN;
	hdr_state = DISABLED;
}

state_t get_boost_state() {
	return boost_state;
}

void enable_boost() {
	if (uvlo || boost_state == ENABLED) {
		return;
	}
#if USE_STARTUP_FLASH_FIX
	enable_inv();
	_delay_ms(FLASH_FIX_PRE_DELAY_MS);
#endif
	// Enable MP3432 and op-amp
	EN_PORT.DIRSET = EN_PIN;
	EN_PORT.OUTSET = EN_PIN;
#if USE_STARTUP_FLASH_FIX
	_delay_ms(FLASH_FIX_POST_DELAY_MS);
	disable_inv();
#endif
	boost_state = ENABLED;
}

void disable_boost() {
	if (boost_state == DISABLED) {
		return;
	}
	// Disable MP3432 and op-amp
	EN_PORT.DIRCLR = EN_PIN;
	EN_PORT.OUTCLR = EN_PIN;
	boost_state = DISABLED;
}