#ifndef CONTROL_H_
#define CONTROL_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	INVALID,
	ENABLED,
	DISABLED,
} state_t;

bool get_uvlo();
void set_uvlo();
void reset_uvlo();

void enable_inv();
void disable_inv();

state_t get_hdr_state();
void enable_hdr();
void disable_hdr();

state_t get_boost_state();
void enable_boost();
void disable_boost();

#endif /* CONTROL_H_ */