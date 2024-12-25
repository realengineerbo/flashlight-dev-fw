#include <avr/io.h>

#include <math.h>
#include <stdbool.h>

#include "brightness.h"
extern "C" {
#include "control.h"
#include "dac.h"
}

struct BrightnessGroup {
	bool hdr;
	uint8_t dac_vref;
	uint8_t dac_value_min;
	uint8_t dac_value_step_count;
	brightness_t brightness_min;
	brightness_t brightness_max;
};

// Prioritise efficiency: Switch to HDR as soon as possible, 652 possible brightness settings
// Prioritise resolution: 1095 possible brightness settings
#define PRIORITISE_EFFICIENCY 1

static const BrightnessGroup BRIGHTNESS_GROUPS[] = {
#if PRIORITISE_EFFICIENCY
	{ .hdr = false, .dac_vref = VREF_DAC0REFSEL_0V55_gc, .dac_value_min = 0, .dac_value_step_count = 100, .brightness_min = 0U, .brightness_max = 3705461U },
	{ .hdr = false, .dac_vref = VREF_DAC0REFSEL_1V1_gc, .dac_value_min = 50, .dac_value_step_count = 0, .brightness_min = 3705461U, .brightness_max = 3705461U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_0V55_gc, .dac_value_min = 1, .dac_value_step_count = 254, .brightness_min = 3705461U, .brightness_max = 944892804U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_1V1_gc, .dac_value_min = 128, .dac_value_step_count = 127, .brightness_min = 948598266U, .brightness_max = 1889785609U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_1V5_gc, .dac_value_min = 187, .dac_value_step_count = 68, .brightness_min = 1889785609U, .brightness_max = 2576980377U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_2V5_gc, .dac_value_min = 153, .dac_value_step_count = 102, .brightness_min = 2576980377U, .brightness_max = 4294967295U },
#else
	{ .hdr = false, .dac_vref = VREF_DAC0REFSEL_0V55_gc, .dac_value_min = 0, .dac_value_step_count = 255, .brightness_min = 0U, .brightness_max = 9448928U },
	{ .hdr = false, .dac_vref = VREF_DAC0REFSEL_1V1_gc, .dac_value_min = 128, .dac_value_step_count = 127, .brightness_min = 9485982U, .brightness_max = 18897856U },
	{ .hdr = false, .dac_vref = VREF_DAC0REFSEL_1V5_gc, .dac_value_min = 187, .dac_value_step_count = 68, .brightness_min = 18897856U, .brightness_max = 25769803U },
	{ .hdr = false, .dac_vref = VREF_DAC0REFSEL_2V5_gc, .dac_value_min = 153, .dac_value_step_count = 102, .brightness_min = 25769803U, .brightness_max = 42949672U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_0V55_gc, .dac_value_min = 12, .dac_value_step_count = 243, .brightness_min = 44465543U, .brightness_max = 944892804U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_1V1_gc, .dac_value_min = 128, .dac_value_step_count = 127, .brightness_min = 948598266U, .brightness_max = 1889785609U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_1V5_gc, .dac_value_min = 187, .dac_value_step_count = 68, .brightness_min = 1889785609U, .brightness_max = 2576980377U },
	{ .hdr = true, .dac_vref = VREF_DAC0REFSEL_2V5_gc, .dac_value_min = 153, .dac_value_step_count = 102, .brightness_min = 2576980377U, .brightness_max = 4294967295U },
#endif
};

static const uint8_t BRIGHTNESS_GROUPS_SIZE = sizeof(BRIGHTNESS_GROUPS) / sizeof(BrightnessGroup);

static int8_t binary_search(brightness_t brightness) {
	uint8_t left = 0;
	uint8_t right = BRIGHTNESS_GROUPS_SIZE - 1;
	while (left < right) {
		uint8_t mid = left + (right - left) / 2;
		if (brightness < BRIGHTNESS_GROUPS[mid].brightness_min) {
			right = mid - 1;
		} else if (brightness > BRIGHTNESS_GROUPS[mid].brightness_max) {
			left = mid + 1;
		} else {
			return mid;
		}
		if (left == right) {
			return left;
		}
	}
	// Fell between the gaps, choose lower
	if (left < right) {
		return left;
	} else {
		return right;
	}
	// This shouldn't be possible
	return -1;
}

static brightness_t brightness_prev = 0;

void set_brightness(brightness_t brightness) {
	if (get_uvlo()) {
		brightness = 0;
	}
	if (brightness != 0) {
		if (get_boost_state() != ENABLED) {
			enable_boost();
		}
		if (brightness == brightness_prev) {
			return;
		}
	}
	// Binary search for appropriate group
	int8_t index = binary_search(brightness);
	if (index < 0) {
		index = 0;
		brightness = 0;
	}
	const BrightnessGroup& bg = BRIGHTNESS_GROUPS[index];
	// Make sure brightness is within range
	if (brightness < bg.brightness_min) {
		brightness = bg.brightness_min;
	} else if (brightness > bg.brightness_max) {
		brightness = bg.brightness_max;
	}
	// Calculate nearest DAC value
	uint8_t dac_value = bg.dac_value_min;
	if (bg.dac_value_step_count != 0) {
		dac_value = bg.dac_value_min + roundf((float)bg.dac_value_step_count * (brightness - bg.brightness_min)) / (bg.brightness_max - bg.brightness_min);
	}
	// Apply changes
	if (bg.dac_vref != DAC0_get_vref()) {
		DAC0_set_vref(bg.dac_vref);
	}
	if (dac_value != DAC0_get_data()) {
		DAC0_set_data(dac_value);
	}
	if (bg.hdr) {
		if (get_hdr_state() != ENABLED) {
			enable_hdr();
		}
	} else {
		if (get_hdr_state() != DISABLED) {
			disable_hdr();
		}
	}
	if (brightness == 0) {
		disable_boost();
	}
	brightness_prev = brightness;
}