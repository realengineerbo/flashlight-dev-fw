#ifndef BRIGHTNESS_H_
#define BRIGHTNESS_H_

#include <stdint.h>

typedef uint32_t brightness_t;
#define BRIGHTNESS_MAX ((brightness_t)0xFFFFFFFF)

void set_brightness(brightness_t brightness);

#endif /* BRIGHTNESS_H_ */