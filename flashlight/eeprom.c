#include <avr/eeprom.h>
#include <avr/io.h>

#include "eeprom.h"

#define CLICK_COUNTER ((uint8_t *)0)

uint8_t load_click_counter() {
	return eeprom_read_byte(CLICK_COUNTER);
}

void save_click_counter(uint8_t value) {
	eeprom_write_byte(CLICK_COUNTER, value);
}