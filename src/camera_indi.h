#ifndef _CAMERA_INDI_H_
#define _CAMERA_INDI_H_

#include "libindiclient/indi.h"

#include <glib.h>

#include "common_indi.h"

enum CAMERAS {
	CAMERA_MAIN,
	CAMERA_GUIDE,
};

enum CAMERA_CALLBACKS {
	CAMERA_CALLBACK_READY = 0,
	CAMERA_CALLBACK_EXPOSE,
	CAMERA_CALLBACK_TEMPERATURE,
	CAMERA_CALLBACK_MAX
};

struct camera_t {
	//This must be fist in the structure
	COMMON_INDI_VARS
	const char *portname;
	int has_blob;

	const unsigned char *image;
	int image_size;
	const char *image_format;

	struct indi_prop_t *expose_prop;
	struct indi_prop_t *abort_prop;
	struct indi_prop_t *frame_prop;
	struct indi_prop_t *frame_type_prop;
	struct indi_prop_t *binning_prop;
	struct indi_prop_t *temp_prop;
};

void camera_get_binning(struct camera_t *camera, int *x, int *y);
void camera_set_binning(struct camera_t *camera, int x, int y);

void camera_get_size(struct camera_t *camera, const char *param, int *value, int *min, int *max);
void camera_set_size(struct camera_t *camera, int width, int height, int x_offset, int y_offset);

void camera_get_temperature(struct camera_t *camera, float *value, float *min, float *max);
void camera_set_temperature(struct camera_t *camera, float value);

void camera_get_exposure_settings(struct camera_t *camera, float *value, float *min, float *max);

void camera_expose(struct camera_t *camera, double time);
void camera_abort_exposure(struct camera_t *camera);

void camera_set_ready_callback(void *window, int type, void *func, void *data);

struct camera_t *camera_find(void *window, int type);

#endif
