#ifndef DAC_H_
#define DAC_H_

void DAC0_init();
uint8_t DAC0_get_vref();
void DAC0_set_vref(uint8_t vref);
uint8_t DAC0_get_data();
void DAC0_set_data(uint8_t data);

#endif /* DAC_H_ */