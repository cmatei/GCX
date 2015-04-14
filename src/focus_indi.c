/*******************************************************************************
  Copyright(c) 2014 Matei Convnoici. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Contact Information: mconovici@gmail.com <Matei Conovici>
*******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "gcx.h"
#include "params.h"

#include "libindiclient/indi.h"
#include "focus_indi.h"

#include <glib-object.h>

static void focuser_check_state(struct focuser_t *focuser)
{
	if (focuser->position_prop && focuser->is_connected) {
		focuser->ready = 1;
		d4_printf("Focuser is ready\n");
		INDI_exec_callbacks(INDI_COMMON (focuser), FOCUSER_CALLBACK_READY);
	}
}

static void focuser_position_update(struct indi_prop_t *iprop, struct focuser_t *focuser)
{
	if (!focuser->ready)
		return;

	if (iprop->state == INDI_STATE_IDLE || iprop->state == INDI_STATE_OK)
		INDI_exec_callbacks(INDI_COMMON (focuser), FOCUSER_CALLBACK_DONE);
}

static void focuser_temp_change_cb(struct indi_prop_t *iprop, void *data)
{
	struct focuser_t *focuser = data;

	if (!focuser->ready)
		return;

	INDI_exec_callbacks(INDI_COMMON (focuser), FOCUSER_CALLBACK_TEMPERATURE);
}

static void focuser_connect(struct indi_prop_t *iprop, void *callback_data)
{
	struct focuser_t *focuser = (struct focuser_t *)callback_data;

	if (!strcmp(iprop->name, "ABS_FOCUS_POSITION")) {
		d4_printf("Found ABS_FOCUS_POSITION for focuser %s\n", iprop->idev->name);
		focuser->position_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)focuser_position_update, focuser);
	} else if (!strcmp(iprop->name, "FOCUS_TEMPERATURE")) {
		d4_printf("Found FOCUS_TEMPERATURE for focuser %s\n", iprop->idev->name);
		focuser->temperature_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)focuser_temp_change_cb, focuser);
	}else {
		//INDI_try_dev_connect(iprop, INDI_COMMON (focuser), P_STR(INDI_FOCUSER_PORT));
		INDI_try_dev_connect(iprop, INDI_COMMON (focuser), NULL);
	}

	focuser_check_state(focuser);
}

void focuser_get_absolute_position(struct focuser_t *focuser, unsigned int *position)
{
	struct indi_elem_t *elem = NULL;

	*position = 0;

	if (!focuser->ready) {
		err_printf("Focuser isn't ready. Can't get position.\n");
		return;
	}

	if (!focuser->position_prop) {
		err_printf("Focuser doesn't support absolute positioning.\n");
		return;
	}

	if ((elem = indi_find_elem(focuser->position_prop, "FOCUS_ABSOLUTE_POSITION")) != NULL)
		*position = elem->value.num.value;
}

void focuser_set_absolute_position(struct focuser_t *focuser, unsigned int position)
{
	int changed = 0;

	if (!focuser->ready) {
		err_printf("Focuser isn't ready. Can't get position.\n");
		return;
	}

	if (!focuser->position_prop) {
		err_printf("Focuser doesn't support absolute positioning.\n");
		return;
	}

	changed |= INDI_update_elem_if_changed(focuser->position_prop, "FOCUS_ABSOLUTE_POSITION",  position);
	if (changed)
		indi_send(focuser->position_prop, NULL);
}

void focuser_set_ready_callback(void * window, void *func, void *data)
{
	struct focuser_t *focuser;

	focuser = (struct focuser_t *) g_object_get_data(G_OBJECT(window), "focuser");
	if (!focuser) {
		err_printf("Filter wheel wasn't found\n");
		return;
	}

	INDI_set_callback(INDI_COMMON (focuser), FOCUSER_CALLBACK_READY, func, data);
}

struct focuser_t *focuser_find(void *window)
{
	struct focuser_t *focuser;
	struct indi_t *indi;
	char *device = P_STR(INDI_MAIN_CAMERA_FOCUSER);

	focuser = (struct focuser_t *) g_object_get_data(G_OBJECT(window), "focuser");
	if (focuser) {
		if (focuser->ready) {
			d4_printf("Found focuser\n");
			return focuser;
		}

		return NULL;
	}

	indi = INDI_get_indi(window);
	if (!indi)
		return NULL;

	focuser = g_new0(struct focuser_t, 1);
	INDI_common_init(INDI_COMMON (focuser), "focuser", focuser_check_state, FOCUSER_CALLBACK_MAX);
	indi_device_add_cb(indi, device, (IndiDevCB)focuser_connect, focuser);

	g_object_set_data(G_OBJECT(window), "focuser", focuser);
	if (focuser->ready)
		return focuser;

	return NULL;
}
