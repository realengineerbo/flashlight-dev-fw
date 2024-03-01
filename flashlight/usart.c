#include <avr/io.h>
#include <stdio.h>

#include "usart.h"

/*
USART: PB2 (default USART0 TX)
*/

static void USART0_sendChar(char c) {
	while (!(USART0.STATUS & USART_DREIF_bm));
	USART0.TXDATAL = c;
}

static int USART0_printChar(char c, FILE *stream) {
	USART0_sendChar(c);
	return 0;
}

static FILE USART_stream = FDEV_SETUP_STREAM(USART0_printChar, NULL, _FDEV_SETUP_WRITE);

void USART0_init() {
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(3333333 * 64 / (16 * (float)BAUD_RATE)) + 0.5)
	// Set TX pin as output and idle high
	PORTB.DIRSET = PIN2_bm;
	PORTB.OUTSET = PIN2_bm;
	// Select alternative communication pins for USART0
	// PORTMUX.CTRLB |= PORTMUX_USART0_ALTERNATE_gc;

	// Set baud rate
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);
	// Enable transmitter
	USART0.CTRLB |= USART_TXEN_bm;

	// Redirect
	stdout = &USART_stream;
}