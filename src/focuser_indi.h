#ifndef _FOCUSER_INDI_H_
#define _FOCUSER_INDI_H_

#include "libindiclient/indi.h"
#include "common_indi.h"

enum {
	FOCUSER_CALLBACK_READY = 0,
	FOCUSER_CALLBACK_DONE,
	FOCUSER_CALLBACK_MAX
};

struct focuser_t {
	//This must be fist in the structure
	COMMON_INDI_VARS

	struct indi_prop_t *motion_prop;
	struct indi_prop_t *position_prop;
	struct indi_prop_t *target_prop;
};

struct focuser_t *focuser_find(void *window);

#endif
