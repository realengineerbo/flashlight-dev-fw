#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

uint8_t load_click_counter();
void save_click_counter(uint8_t value);

#endif /* EEPROM_H_ */