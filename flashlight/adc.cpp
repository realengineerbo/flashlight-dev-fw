#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stddef.h>

#include "adc.h"

/*
OTC: PC1 (ADC1, AIN7 for off-time capacitor. ADC on startup, output high after)
BAT: PC0 (ADC1, AIN6 for battery level)
NTC: PB4 (ADC0, AIN9 for NTC temperature, ranges from 5% to 95% of VCC)
*/

bool ADC0_is_converting() {
	return ADC0.COMMAND;
}

static void ADC0_init_common() {
	// Enable init delay
	ADC0.CTRLD |= ADC_INITDLY_DLY16_gc;
	// Set sample accumulation
	ADC0.CTRLB = ADC_SAMPNUM_ACC4_gc;
	// Set sample capacitance and prescaler
	ADC0.CTRLC |= ADC_SAMPCAP_bm | ADC_PRESC_DIV16_gc;
	// Enable ADC
	ADC0.CTRLA |= ADC_ENABLE_bm;
}

static void ADC0_init_internal_temperature() {
	// Set VREF to 1.1V
	VREF.CTRLA &= ~VREF_ADC0REFSEL_gm;
	VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
	// Use internal reference
	ADC0.CTRLC &= ~ADC_REFSEL_gm;
	ADC0.CTRLC |= ADC_REFSEL_INTREF_gc;
	// Measure internal temperature
	ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;

	ADC0_init_common();
}

static void ADC0_init_ntc_temperature() {
	// Use VDD as reference
	ADC0.CTRLC &= ~ADC_REFSEL_gm;
	ADC0.CTRLC |= ADC_REFSEL_VDDREF_gc;
	// Measure AIN9 for NTC temperature
	ADC0.MUXPOS = ADC_MUXPOS_AIN9_gc;

	ADC0_init_common();
}

static void ADC0_start_conversion() {
	// Enable interrupt
	ADC0.INTCTRL |= ADC_RESRDY_bm;
	// Start conversion
	ADC0.COMMAND |= ADC_STCONV_bm;
}

static void (*ADC0_cb)(uint16_t) = NULL;

ISR(ADC0_RESRDY_vect) {
	ADC0.INTFLAGS = ADC_RESRDY_bm;

	// Divide by 4 due to sample accumulation
	uint16_t adc0_res = ADC0.RES >> 2;

	if (ADC0_cb) {
		ADC0_cb(adc0_res);
		ADC0_cb = NULL;
	}
}

static void (*internal_temperature_cb)(float) = NULL;

static void internal_temperature_handler(uint16_t lsb) {
	uint8_t sigrow_gain = SIGROW.TEMPSENSE0; // Read unsigned value from signature row
	int8_t sigrow_offset = SIGROW.TEMPSENSE1; // Read signed value from signature row
	uint32_t temp = lsb - sigrow_offset;
	temp *= sigrow_gain; // Result might overflow 16 bit variable (10bit+8bit)
	temp += 0x80; // Add 1/2 to get correct rounding on division below
	temp >>= 8; // Divide result to get Kelvin

	if (internal_temperature_cb) {
		internal_temperature_cb((float)temp);
		internal_temperature_cb = NULL;
	}
}

void get_internal_temperature(void (*cb)(float)) {
	ADC0_init_internal_temperature();
	ADC0_cb = internal_temperature_handler;
	internal_temperature_cb = cb;
	ADC0_start_conversion();
}

static void (*ntc_temperature_cb)(float) = NULL;

static void ntc_temperature_handler(uint16_t lsb) {
	static const float R0 = 10e3; // NTC resistance at T0: 10kOhms
	static const float R1 = 10e3; // Potential divider resistor: 10kOhms
	static const float T0 = 298.15;
	static const float B = 3428; // 3428K: 25-80degC, 3434K:25-85degC, 3455K:25-100degC
	static const float C = logf(R0) / B - 1.0f / T0;

	// C = ln(R_0)/B - 1/T_0
	// temp_sense_volts = temp_sense_lsb*VCC/1023
	// temp_sense_volts = VCC*R1/(R + R1)
	// temp_sense_lsb/1023 = R1/(R + R1)
	// R = 1023*R1/temp_sense_lsb - R1
	// T = 1/(ln(R)/B - C)
	float r = 1023 * R1 / lsb - R1;
	float temp = 1 / (logf(r) / B - C);

	if (ntc_temperature_cb) {
		ntc_temperature_cb(temp);
		ntc_temperature_cb = NULL;
	}
}

void get_ntc_temperature(void (*cb)(float)) {
	ADC0_init_ntc_temperature();
	ADC0_cb = ntc_temperature_handler;
	ntc_temperature_cb = cb;
	ADC0_start_conversion();
}

bool ADC1_is_converting() {
	return ADC1.COMMAND;
}

static void ADC1_init_common() {
	// Enable init delay
	ADC1.CTRLD |= ADC_INITDLY_DLY16_gc;
	// Set sample accumulation
	ADC1.CTRLB = ADC_SAMPNUM_ACC4_gc;
	// Set sample capacitance and prescaler
	ADC1.CTRLC |= ADC_SAMPCAP_bm | ADC_PRESC_DIV16_gc;
	// Enable ADC
	ADC1.CTRLA |= ADC_ENABLE_bm;
}

static void ADC1_init_battery_level() {
	// Set VREF to 1.5V
	VREF.CTRLC &= ~VREF_ADC1REFSEL_gm;
	VREF.CTRLC |= VREF_ADC1REFSEL_1V5_gc;
	// Use internal reference
	ADC1.CTRLC &= ~ADC_REFSEL_gm;
	ADC1.CTRLC |= ADC_REFSEL_INTREF_gc;
	// Measure AIN6 for battery level
	ADC1.MUXPOS = ADC_MUXPOS_AIN6_gc;

	ADC1_init_common();
}

static void ADC1_init_off_time_capacitor() {
	// Set VREF to 2.5V
	VREF.CTRLC &= ~VREF_ADC1REFSEL_gm;
	VREF.CTRLC |= VREF_ADC1REFSEL_2V5_gc;
	// Use internal reference
	ADC1.CTRLC &= ~ADC_REFSEL_gm;
	ADC1.CTRLC |= ADC_REFSEL_INTREF_gc;
	// Measure AIN7 for off-time capacitor
	ADC1.MUXPOS = ADC_MUXPOS_AIN7_gc;

	ADC1_init_common();
}

static void ADC1_start_conversion() {
	// Enable interrupt
	ADC1.INTCTRL |= ADC_RESRDY_bm;
	// Start conversion
	ADC1.COMMAND |= ADC_STCONV_bm;
}

static void (*ADC1_cb)(uint16_t) = NULL;

ISR(ADC1_RESRDY_vect) {
	ADC1.INTFLAGS = ADC_RESRDY_bm;

	// Divide by 4 due to sample accumulation
	uint16_t adc1_res = ADC1.RES >> 2;

	if (ADC1_cb) {
		ADC1_cb(adc1_res);
		ADC1_cb = NULL;
	}
}

static float ADC1_get_vref() {
	uint8_t adc1refsel = VREF.CTRLC & VREF_ADC1REFSEL_gm;
	switch (adc1refsel) {
	case VREF_ADC1REFSEL_0V55_gc:
		return 0.55f;
	case VREF_ADC1REFSEL_1V1_gc:
		return 1.1f;
	case VREF_ADC1REFSEL_2V5_gc:
		return 2.5f;
	case VREF_ADC1REFSEL_4V34_gc:
		return 4.34f;
	case VREF_ADC1REFSEL_1V5_gc:
		return 1.5f;
	default:
		return 0;
	}
}

static void (*battery_level_cb)(float) = NULL;

static void battery_level_handler(uint16_t lsb) {
	if (battery_level_cb) {
		// Gain from to potential divider
		static const float GAIN = 3.0f;
		battery_level_cb(GAIN * ADC1_get_vref() * lsb / 1023);
		battery_level_cb = NULL;
	}
}

void get_battery_level(void (*cb)(float)) {
	ADC1_init_battery_level();
	ADC1_cb = battery_level_handler;
	battery_level_cb = cb;
	ADC1_start_conversion();
}

static void (*off_time_cb)(float) = NULL;

static void off_time_handler(uint16_t lsb) {
	static const float R = 750e3; // 750kOhms
	static const float C = 4.7e-6; // 4.7uF
	static const float RC = R * C;

	// Vc/Vs = exp(-t/RC)
	// t = -RC * ln(Vc/Vs)
	// When VCC falls, OTC is clamped by ESD protection diodes
	// leaving diode's forward voltage ~0.35V (empirical value)
	static const float vs = 0.35f;
	float vc = ADC1_get_vref() * lsb / 1023;
	float off_time = -RC * logf(vc / vs);

	// If OTC is not clamped, use VDD as reference
	// and calculate off_time the ratiometric way
	// float off_time = -RC * logf(lsb / 1023.0f);

	if (off_time_cb) {
		off_time_cb(off_time);
		off_time_cb = NULL;
	}
}

void get_off_time(void (*cb)(float)) {
	ADC1_init_off_time_capacitor();
	ADC1_cb = off_time_handler;
	off_time_cb = cb;
	ADC1_start_conversion();
}