#ifndef _FOCUS_INDI_H_
#define _FOCUS_INDI_H_

#include "libindiclient/indi.h"
#include "common_indi.h"

enum {
	FOCUSER_CALLBACK_READY = 0,
	FOCUSER_CALLBACK_TEMPERATURE,
	FOCUSER_CALLBACK_DONE,
	FOCUSER_CALLBACK_MAX
};

struct focuser_t {
	//This must be fist in the structure
	COMMON_INDI_VARS

	struct indi_prop_t *position_prop;
	struct indi_prop_t *temperature_prop;
};


void focuser_get_absolute_position(struct focuser_t *focuser, unsigned int *position);
void focuser_set_absolute_position(struct focuser_t *focuser, unsigned int position);

void focuser_set_ready_callback(void * window, void *func, void *data);
struct focuser_t *focuser_find(void *window);

#endif
