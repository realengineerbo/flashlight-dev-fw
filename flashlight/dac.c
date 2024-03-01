#include <avr/io.h>

#include "dac.h"

/*
DAC: PA6 (output)
*/

void DAC0_init() {
	PORTA.DIRSET = PIN6_bm;
	DAC0_set_vref(VREF_DAC0REFSEL_0V55_gc);
	DAC0_set_data(0);
	DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

uint8_t DAC0_get_vref() {
	return (VREF.CTRLA & VREF_DAC0REFSEL_gm);
}

void DAC0_set_vref(uint8_t vref) {
	VREF.CTRLA &= ~VREF_DAC0REFSEL_gm;
	VREF.CTRLA |= vref;
}

uint8_t DAC0_get_data() {
	return DAC0.DATA;
}

void DAC0_set_data(uint8_t data) {
	DAC0.DATA = data;
}
