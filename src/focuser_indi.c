/*******************************************************************************
  Copyright(c) 2013 Matei Conovici. All rights reserved.

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
#include "focuser_indi.h"

#include <glib-object.h>


static void focuser_check_state(struct focuser_t *focuser)
{
	if (focuser->is_connected &&
	    focuser->motion_prop && focuser->position_prop && focuser->target_prop) {
		focuser->ready = 1;
		d4_printf("Focuser is ready\n");
		INDI_exec_callbacks(INDI_COMMON (focuser), FOCUSER_CALLBACK_READY);
	}
}

static void focuser_connect(struct indi_prop_t *iprop, void *callback_data)
{
	struct focuser_t *focuser = (struct focuser_t *) callback_data;

#if 0
	if (strcmp(iprop->name, "FILTER_SLOT") == 0) {
		d4_printf("Found FILTER_SLOT for filter wheel %s\n", iprop->idev->name);
		fwheel->filter_slot_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)fwheel_slot_update, fwheel);
	}
	else
		INDI_try_dev_connect(iprop, INDI_COMMON (fwheel), P_STR(INDI_FWHEEL_PORT));
#endif

	focuser_check_state(focuser);
}

#if 0


static void fwheel_slot_update(struct indi_prop_t *iprop, struct fwheel_t *fwheel)
{
	if(! fwheel->ready)
		return;
	if(iprop->state == INDI_STATE_IDLE || iprop->state == INDI_STATE_OK)
		INDI_exec_callbacks(INDI_COMMON (fwheel), FWHEEL_CALLBACK_DONE);
}


char **fwheel_get_filters(struct fwheel_t *fwheel)
{
	char **fnames;

	fnames = calloc(1,1024);
	printf("fwheel_get_filters is not yet defined\n");
	return fnames;
}

void fwheel_set_ready_callback(void * window, void *func, void *data)
{
	struct fwheel_t *fwheel;
	fwheel = (struct fwheel_t *)g_object_get_data(G_OBJECT(window), "fwheel");
	if (! fwheel) {
		err_printf("Filter wheel wasn't found\n");
		return;
	}
	INDI_set_callback(INDI_COMMON (fwheel), FWHEEL_CALLBACK_READY, func, data);

}

#endif

struct focuser_t *focuser_find(void *window)
{
	struct focuser_t *focuser;
	struct indi_t *indi;

	focuser = (struct focuser_t *) g_object_get_data (G_OBJECT(window), "focuser");
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
	indi_device_add_cb(indi, P_STR(INDI_FOCUSER_NAME), (IndiDevCB) focuser_connect, focuser);
	g_object_set_data(G_OBJECT(window), "focuser", focuser);
	if (focuser->ready)
		return focuser;

	return NULL;
}
