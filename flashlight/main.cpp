#include "f_cpu.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>

#include "adc.h"
#include "brightness.h"
extern "C" {
#include "control.h"
#include "dac.h"
#include "eeprom.h"
#include "rtc.h"
#include "usart.h"
};

#define CLICK_GRACE_PERIOD_SECONDS 1
#define UVLO_VOLTS 3

/*
BAT_EN: PB0 (output)
OTC: PC1 (ADC input, then output)
*/

#define BAT_EN_PORT PORTB
#define BAT_EN_PIN PIN0_bm

#define OTC_PORT PORTC
#define OTC_PIN PIN1_bm

// LED is external, for debugging
#define LED_PORT PORTB
#define LED_PIN PIN5_bm

// ====================
// ===== Off-Time =====
// ====================

static void off_time_handler(float off_time) {
	// Set pin to output to charge off-time capacitor
	OTC_PORT.OUTSET = OTC_PIN;
	OTC_PORT.DIRSET = OTC_PIN;
	// printf("off-time: %.2fs\r\n", (double)off_time);

	uint8_t click_counter = load_click_counter();
	if (off_time <= CLICK_GRACE_PERIOD_SECONDS) {
		save_click_counter(click_counter + 1);
	} else if (click_counter != 0) {
		// Intentionally only reset click counter here
		// Leaving flashlight on does not count against grace period
		save_click_counter(0);
	}
}

static void check_off_time() {
	// Set pin to input
	OTC_PORT.DIRCLR = OTC_PIN;
	get_off_time(off_time_handler);
}

// ===============================
// ===== Temperature Sensing =====
// ===============================

static float kelvin_to_celsius(float kelvin) {
	return kelvin - 273.15f;
}

static bool use_internal_temperature = false;

static void internal_temperature_handler(float temperature) {
	use_internal_temperature = false;
	printf("internal temp.: %.0f C\r\n", (double)kelvin_to_celsius(temperature));
}

static void ntc_temperature_handler(float temperature) {
	use_internal_temperature = true;
	printf("NTC temp.: %.2f C\r\n", (double)kelvin_to_celsius(temperature));
}

static void check_temperatures() {
	if (use_internal_temperature) {
		get_internal_temperature(internal_temperature_handler);
	} else {
		get_ntc_temperature(ntc_temperature_handler);
	}
}

// =========================
// ===== Battery Level =====
// =========================

static void battery_level_handler(float battery_level) {
	BAT_EN_PORT.DIRCLR = BAT_EN_PIN;
	BAT_EN_PORT.OUTCLR = BAT_EN_PIN;
	// printf("battery: %.2f V\r\n", (double)battery_level);
	if (battery_level < UVLO_VOLTS) {
		set_uvlo();
		set_brightness(0);
	} else if (get_uvlo()) {
		reset_uvlo();
	}
}

static void check_battery_level() {
	BAT_EN_PORT.DIRSET = BAT_EN_PIN;
	BAT_EN_PORT.OUTSET = BAT_EN_PIN;
	get_battery_level(battery_level_handler);
}

// ================================
// ===== Brightness Ramp Loop =====
// ================================

static void update_brightness() {
	static const float MAX_TIME = 22;
	static const uint16_t STEP_COUNT = 4096;
	static const float TIME_STEP = MAX_TIME / STEP_COUNT;
	static float t = 0;
	static int8_t direction = 1;
	float brightness_float = BRIGHTNESS_MAX * exp(t - MAX_TIME);
	brightness_t brightness = 0;
	if (brightness_float <= 0) {
		brightness = 0;
	} else if (brightness_float >= BRIGHTNESS_MAX) {
		brightness = BRIGHTNESS_MAX;
	} else {
		brightness = (brightness_t)brightness_float;
	}
	set_brightness(brightness);
	t += direction * TIME_STEP;
	if (t >= 1.1f * MAX_TIME) {
		direction = -1;
	} else if (t <= 0) {
		t = 0;
		direction = 1;
	}
}

typedef enum {
	MODE_ULTRA_LOW = 0,
	MODE_LOW,
	MODE_HIGH,
	MODE_ULTRA_HIGH,
	MODE_RAMP_LOOP,
	MODE_MAX,
} mode_t;

int main() {
	// Enable global interrupts
	sei();

	// Check off-time
	check_off_time();
	while (ADC1_is_converting());

	// Initialise
	disable_boost();
	disable_hdr();
	USART0_init();
	RTC_init();
	DAC0_init();

	// Enable external LED
	LED_PORT.DIRSET = LED_PIN;

	uint8_t click_counter = load_click_counter();
	mode_t mode = static_cast<mode_t>(click_counter % MODE_MAX);
	printf("mode: %u\r\n", mode);

	uint32_t blink_counter_prev = 0;
	uint32_t check_counter_prev = 0;

	while (true) {
		static const uint8_t COUNTER_FREQ_HZ = 8;
		uint32_t counter = get_counter();

		// Check temperatures and battery level
		static const uint8_t UPDATE_FREQ_HZ = 8;
		static const uint8_t UPDATE_COUNTER_PERIOD = COUNTER_FREQ_HZ / UPDATE_FREQ_HZ;
		if (counter - check_counter_prev >= UPDATE_COUNTER_PERIOD) {
			check_counter_prev = counter;
			check_temperatures();
			check_battery_level();
		}

		switch (mode) {
		case MODE_ULTRA_LOW:
		case MODE_MAX:
			set_brightness(1);
			break;
		case MODE_LOW:
			set_brightness(4e5);
			break;
		case MODE_HIGH:
			set_brightness(15e6);
			break;
		case MODE_ULTRA_HIGH:
			set_brightness(BRIGHTNESS_MAX);
			break;
		case MODE_RAMP_LOOP:
			update_brightness();
			break;
		}

		// Toggle LED at 2 Hz if normal, 1 Hz if UVLO
		static const uint8_t BLINK_FREQ_HZ = 2;
		static const uint8_t BLINK_COUNTER_PERIOD = COUNTER_FREQ_HZ / BLINK_FREQ_HZ;
		if (counter - blink_counter_prev >= (get_uvlo() ? (2 * BLINK_COUNTER_PERIOD) : BLINK_COUNTER_PERIOD)) {
			blink_counter_prev = counter;
			LED_PORT.OUTTGL = LED_PIN;
		}
	}

	return 0;
}

