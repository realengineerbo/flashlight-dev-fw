#ifndef ADC_H_
#define ADC_H_

#include <stdbool.h>

bool ADC0_is_converting();
void get_internal_temperature(void (*cb)(float));
void get_ntc_temperature(void (*cb)(float));

bool ADC1_is_converting();
void get_battery_level(void (*cb)(float));
void get_off_time(void (*cb)(float));

#endif /* ADC_H_ */