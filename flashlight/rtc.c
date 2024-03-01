#include <avr/interrupt.h>
#include <avr/io.h>

#include "rtc.h"

void RTC_init() {
	// Select clock
	RTC.CLKSEL &= ~RTC_CLKSEL_gm;
	RTC.CLKSEL |= RTC_CLKSEL_INT32K_gc;
	// Enable interrupt
	RTC.PITINTCTRL |= RTC_PI_bm;
	// Set period and enable
	RTC.PITCTRLA |= RTC_PERIOD_CYC4096_gc | RTC_PITEN_bm;
}

static volatile uint32_t counter = 0;

ISR(RTC_PIT_vect) {
	RTC.PITINTFLAGS = 1;
	counter++;
}

uint32_t get_counter() {
	return counter;
}