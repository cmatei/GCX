/*******************************************************************************
  Copyright(c) 2009 Geoffrey Hausheer. All rights reserved.

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

  Contact Information: gcx@phracturedblue.com <Geoffrey Hausheer>
*******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "gcx.h"
#include "params.h"

#include "libindiclient/indi.h"
#include "tele_indi.h"

#include <glib-object.h>

static void tele_check_state(struct tele_t *tele)
{
	if (tele->ready)
		return;

	if (tele->is_connected &&
		((tele->move_ns_prop && tele->move_ew_prop) ||
		(tele->timed_guide_ns_prop && tele->timed_guide_ew_prop)))
	{
		tele->ready = 1;
		d4_printf("Telescope is ready\n");
		INDI_exec_callbacks(INDI_COMMON (tele), TELE_CALLBACK_READY);
	}
}

static void tele_new_coords_cb(struct indi_prop_t *iprop, void *data)
{
	struct tele_t *tele = data;

	if (iprop->state != INDI_STATE_IDLE && iprop->state != INDI_STATE_OK)
		return;
	tele->right_ascension = 15.0 * indi_prop_get_number(iprop, "RA");
	tele->declination = indi_prop_get_number(iprop, "DEC");
}

static void tele_move_cb(struct indi_prop_t *iprop, void *data)
{
	struct tele_t *tele = data;

	if (tele->move_ns_prop &&
            (tele->move_ns_prop->state != INDI_STATE_IDLE ||
	     tele->move_ew_prop->state != INDI_STATE_IDLE))
	{
		return;
	}
	if (tele->timed_guide_ns_prop &&
            (tele->timed_guide_ns_prop->state != INDI_STATE_IDLE ||
	     tele->timed_guide_ew_prop->state != INDI_STATE_IDLE))
	{
		return;
	}
	// If we get here than the scope state is consistent
	if (indi_prop_get_switch(tele->move_ns_prop, "MOTION_NORTH") ||
	    indi_prop_get_switch(tele->move_ns_prop, "MOTION_SOUTH") ||
	    indi_prop_get_switch(tele->move_ew_prop, "MOTION_EAST") ||
	    indi_prop_get_switch(tele->move_ew_prop, "MOTION_WEST"))
	{
		return;
	}
	INDI_exec_callbacks(INDI_COMMON (tele), TELE_CALLBACK_STOP);
}

//Stop all telescope motion
void tele_cancel_movement(struct tele_t *tele)
{
	indi_prop_set_switch(tele->abort_prop, "ABORT", TRUE);
	indi_send(tele->abort_prop, NULL);
}

//Stop NS motion
static void tele_cancel_movement_ns(struct tele_t *tele)
{
	indi_prop_set_switch(tele->move_ns_prop, "MOTION_NORTH", FALSE);
	indi_prop_set_switch(tele->move_ns_prop, "MOTION_SOUTH", FALSE);
	indi_send(tele->move_ns_prop, NULL);
}

//Stop EW motion
static void tele_cancel_movement_ew(struct tele_t *tele)
{
	indi_prop_set_switch(tele->move_ew_prop, "MOTION_EAST", FALSE);
	indi_prop_set_switch(tele->move_ew_prop, "MOTION_WEST", FALSE);
	indi_send(tele->move_ew_prop, NULL);
}

static void tele_connect(struct indi_prop_t *iprop, void *callback_data)
{
	struct tele_t *tele = (struct tele_t *)callback_data;

	if (strcmp(iprop->name, "EQUATORIAL_EOD_COORD") == 0) {
		tele->coord_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)tele_new_coords_cb, tele);
	}
	else if (strcmp(iprop->name, "ON_COORD_SET") == 0) {
		tele->coord_set_type_prop = iprop;
	}
	else if (strcmp(iprop->name, "TELESCOPE_ABORT_MOTION") == 0) {
		tele->abort_prop = iprop;
	}
	else if (strcmp(iprop->name, "TELESCOPE_MOTION_NS") == 0) {
		tele->move_ns_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, tele);
	}
	else if (strcmp(iprop->name, "TELESCOPE_MOTION_WE") == 0) {
		tele->move_ew_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, tele);
	}
	else if (strcmp(iprop->name, "TELESCOPE_TIMED_GUIDE_NS") == 0) {
		tele->timed_guide_ns_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, tele);
	}
	else if (strcmp(iprop->name, "TELESCOPE_TIMED_GUIDE_WE") == 0) {
		tele->timed_guide_ew_prop = iprop;
		indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, tele);
	}
	else if (strcmp(iprop->name, "Slew rate") == 0) {
		tele->speed_prop = iprop;
	}
	else {
		//INDI_try_dev_connect(iprop, INDI_COMMON (tele), P_STR(INDI_SCOPE_PORT));
		INDI_try_dev_connect(iprop, INDI_COMMON (tele), NULL);
	}

	tele_check_state(tele);
}

void tele_set_speed(struct tele_t *tele, int type)
{
	struct indi_elem_t *elem;
	if (! tele->speed_prop) {
		err_printf("Telescope doesn't support changing move speed\n");
		return;
	}
	switch (type) {
	case TELE_MOVE_CENTERING:
		elem = indi_prop_set_switch(tele->speed_prop, "Centering", TRUE);
		break;
	case TELE_MOVE_GUIDE:
		elem = indi_prop_set_switch(tele->speed_prop, "Guide", TRUE);
		break;
	default:
		err_printf("Unknown speed: %d\n", type);
		return;
	}
	if (! elem) {
		err_printf("Couldn't locate speed control\n");
		return;
	}
	indi_send(tele->speed_prop, elem);
}

void tele_timed_move(struct tele_t *tele, int dx_msec, int dy_msec, int type)
{
	struct indi_elem_t *elem;
	d3_printf("Timed move: %d %d [%d]\n", dx_msec, dy_msec, type);
	if(! tele->ready) {
		err_printf("Telescope isn't connected.  Aborting timed move\n");
		return;
	}
	if (type == TELE_MOVE_GUIDE && tele->timed_guide_ns_prop) {
		// We can submit a timed guide, and let the mount take care of it
		if (dx_msec != 0) {
			if (dx_msec > 0) {
				elem = indi_prop_set_number(tele->timed_guide_ew_prop, "TIMED_GUIDE_E", dx_msec / 1000.0);
			} else {
				elem = indi_prop_set_number(tele->timed_guide_ew_prop, "TIMED_GUIDE_W", -dx_msec / 1000.0);
			}
			indi_send(tele->timed_guide_ew_prop, elem);
		}
		if (dy_msec != 0) {
			if (dy_msec > 0) {
				elem = indi_prop_set_number(tele->timed_guide_ns_prop, "TIMED_GUIDE_N", dy_msec / 1000.0);
			} else {
				elem = indi_prop_set_number(tele->timed_guide_ns_prop, "TIMED_GUIDE_S", -dy_msec / 1000.0);
			}
			indi_send(tele->timed_guide_ns_prop, elem);
		}
		return;
	}
	// Use timer to do move
	tele_set_speed(tele, type);
	if (dx_msec != 0) {
		if (dx_msec > 0) {
			elem = indi_prop_set_switch(tele->move_ew_prop, "MOTION_EAST", TRUE);
		} else {
			elem = indi_prop_set_switch(tele->move_ew_prop, "MOTION_WEST", TRUE);
			dx_msec = -dx_msec;
		}
		indi_send(tele->move_ew_prop, elem);
		g_timeout_add_full(G_PRIORITY_HIGH, dx_msec, (GSourceFunc)tele_cancel_movement_ew, tele, NULL);
	}
	if (dy_msec != 0) {
		if (dy_msec > 0) {
			elem = indi_prop_set_switch(tele->move_ns_prop, "MOTION_NORTH", TRUE);
		} else {
			elem = indi_prop_set_switch(tele->move_ns_prop, "MOTION_SOUTH", TRUE);
			dy_msec = -dy_msec;
		}
		indi_send(tele->move_ns_prop, elem);
		g_timeout_add_full(G_PRIORITY_HIGH, dy_msec, (GSourceFunc)tele_cancel_movement_ns, tele, NULL);
	}
}

void tele_guide_move(struct tele_t *tele, int dx_msec, int dy_msec)
{
	tele_timed_move(tele, dx_msec, dy_msec, TELE_MOVE_GUIDE);
}

void tele_center_move(struct tele_t *tele, float dra, float ddec)
{
	float ra, dec;
	ra = tele_get_ra(tele) + dra;
	dec = tele_get_dec(tele) + ddec;
	tele_set_speed(tele, TELE_MOVE_CENTERING);
	tele_set_coords(tele, TELE_COORDS_SLEW, ra, dec, 0.0);
}

double tele_get_ra(struct tele_t *tele)
{
	if (! tele->coord_prop)
		return 0.0;
	return 15.0 * indi_prop_get_number(tele->coord_prop, "RA");
}

double tele_get_dec(struct tele_t *tele)
{
	if (! tele->coord_prop)
		return 0.0;
	return indi_prop_get_number(tele->coord_prop, "DEC");
}

void tele_abort(struct tele_t *tele)
{
	struct indi_elem_t *elem;
	if (! tele) {
		err_printf("No telescope found\n");
		return;
	}
	if (! tele->ready) {
		err_printf("Telescope isn't ready\n");
		return;
	}
	if (! tele->abort_prop) {
		d4_printf("Telescope doesn't support ABORT\n");
		return;
	}
	if ((elem = indi_prop_set_switch(tele->abort_prop, "ABORT", 1)))
		indi_send(tele->abort_prop, elem);
}

int tele_set_coords(struct tele_t *tele, int type, double ra, double dec, double equinox)
{
	struct indi_elem_t *elem = NULL;

	if (ra < 0 || ra >= 360 || dec < -90 || dec > 90) {
		err_printf("Bad RA/DEC\n");
		return -1;
	}
	if (! tele) {
		err_printf("No telescope found\n");
		return -1;
	}
	if (! tele->ready) {
		err_printf("Telescope isn't ready\n");
		return -1;
	}
	if (! tele->coord_prop) {
		d4_printf("Telescope doesn't support coordinates\n");
		return -1;
	}
	if (! tele->coord_set_type_prop && type != TELE_COORDS_SLEW) {
		d4_printf("Telescope doesn't support sync\n");
		return -1;
	}
	if (type >= TELE_COORDS_MAX) {
		err_printf("Unknown type specified to coord-sync: %d\n", type);
		return -1;
	}
	if (tele->coord_set_type_prop) {
		switch (type) {
		case TELE_COORDS_SYNC:
			elem = indi_prop_set_switch(tele->coord_set_type_prop, "SYNC", 1);
			break;
		case TELE_COORDS_SLEW:
			elem = indi_prop_set_switch(tele->coord_set_type_prop, "SLEW", 1);
			break;
		}
		if (elem) {
			if (tele->abort_prop)
				indi_send(tele->abort_prop, elem);
		} else {
			err_printf("Telescope failed to change mode\n");
			return -1;
		}
	}
	indi_prop_set_number(tele->coord_prop, "RA", ra / 15.0);
	indi_prop_set_number(tele->coord_prop, "DEC", dec);
	indi_send(tele->coord_prop, NULL);
	return 0;
}

void tele_set_ready_callback(void * window, void *func, void *data)
{
	struct tele_t *tele;
	tele = (struct tele_t *)g_object_get_data(G_OBJECT(window), "telescope");
	if (! tele) {
		err_printf("Telescope wasn't found\n");
		return;
	}
	INDI_set_callback(INDI_COMMON (tele), TELE_CALLBACK_READY, func, data);

}

struct tele_t *tele_find(void *window)
{
	struct tele_t *tele;
	struct indi_t *indi;
	tele = (struct tele_t *)g_object_get_data(G_OBJECT(window), "telescope");
	if (tele) {
		if (tele->ready) {
			d4_printf("Found telescope\n");
			return tele;
		}
		return NULL;
	}
	if (! (indi = INDI_get_indi(window)))
		return NULL;
	tele = g_new0(struct tele_t, 1);
	INDI_common_init(INDI_COMMON (tele), "telescope", tele_check_state, TELE_CALLBACK_MAX);
	indi_device_add_cb(indi, P_STR(INDI_SCOPE_NAME), (IndiDevCB)tele_connect, tele);
	g_object_set_data(G_OBJECT(window), "telescope", tele);
	if (tele->ready)
		return tele;
	return NULL;
}
