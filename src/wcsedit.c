/*******************************************************************************
  Copyright(c) 2000 - 2003 Radu Corlan. All rights reserved.

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

  Contact Information: radu@corlan.net
*******************************************************************************/

/* wcs edit dialog */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <glob.h>
#include <math.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "obsdata.h"
#include "sourcesdraw.h"
#include "params.h"
#include "interface.h"
#include "wcs.h"
#include "misc.h"

#define INV_DBL -10000.0

/* Pertti's code begin */
static void wcs_north_cb( GtkWidget *widget, gpointer data );
static void wcs_south_cb( GtkWidget *widget, gpointer data );
static void wcs_east_cb( GtkWidget *widget, gpointer data );
static void wcs_west_cb( GtkWidget *widget, gpointer data );
static void wcs_NS_cb(int dir, GtkWidget *widget, gpointer data );
static void wcs_EW_cb(int dir, GtkWidget *widget, gpointer data );
static void wcs_rot_inc_cb( GtkWidget *widget, gpointer data );
static void wcs_rot_dec_cb( GtkWidget *widget, gpointer data );
static void wcs_rot_cb(int dir, GtkWidget *widget, gpointer data );
static void wcs_step_cb(GtkWidget *widget, gpointer data );
/* Pertti's code end */

static int wcsentry_cb( GtkWidget *widget, gpointer data );

static void update_wcs_dialog(GtkWidget *dialog, struct wcs *wcs);
static void wcs_ok_cb( GtkWidget *widget, gpointer data );

static void close_wcsedit( GtkWidget *widget, gpointer data )
{
	g_return_if_fail(data != NULL);
	g_object_set_data(G_OBJECT(data), "wcs_dialog", NULL);
}

static void close_wcs_dialog( GtkWidget *widget, gpointer data )
{
	GtkWidget *im_window;
	im_window = g_object_get_data(G_OBJECT(data), "im_window");
	g_return_if_fail(im_window != NULL);
	g_object_set_data(G_OBJECT(im_window), "wcs_dialog", NULL);
}

/* show the current frame's fits header in a text window */
void act_control_wcs (GtkAction *action, gpointer window)
{
	GtkWidget *dialog;
//	struct image_channel *i_chan;
	struct wcs *wcs;

/*
	i_chan = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_chan == NULL || i_chan->fr == NULL) {
		error_message_sb2(window, "No frame loaded");
		return;
	}
*/
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs == NULL) {
		wcs = wcs_new();
		g_object_set_data_full(G_OBJECT(window), "wcs_of_window",
					 wcs, (GDestroyNotify)wcs_release);
	}

	dialog = g_object_get_data(G_OBJECT(window), "wcs_dialog");
	if (dialog == NULL) {
		dialog = create_wcs_edit();
		g_object_set_data(G_OBJECT(dialog), "im_window",
					 window);
		g_object_set_data_full(G_OBJECT(window), "wcs_dialog",
					 dialog, (GDestroyNotify)(gtk_widget_destroy));
		g_signal_connect (G_OBJECT (dialog), "destroy",
				    G_CALLBACK (close_wcsedit), window);
		set_named_callback (G_OBJECT (dialog), "wcs_close_button", "clicked",
				    G_CALLBACK (close_wcs_dialog));
		set_named_callback (G_OBJECT (dialog), "wcs_ok_button", "clicked",
				    G_CALLBACK (wcs_ok_cb));
/* Pertti's code begin */
		set_named_callback (G_OBJECT (dialog), "wcs_north_button", "clicked", G_CALLBACK (wcs_north_cb));
		set_named_callback (G_OBJECT (dialog), "wcs_south_button", "clicked", G_CALLBACK (wcs_south_cb));
		set_named_callback (G_OBJECT (dialog), "wcs_east_button", "clicked", G_CALLBACK (wcs_east_cb));
		set_named_callback (G_OBJECT (dialog), "wcs_west_button", "clicked", G_CALLBACK (wcs_west_cb));
		set_named_callback (G_OBJECT (dialog), "wcs_rot_inc_button", "clicked", G_CALLBACK (wcs_rot_inc_cb));
		set_named_callback (G_OBJECT (dialog), "wcs_rot_dec_button", "clicked", G_CALLBACK (wcs_rot_dec_cb));
		set_named_callback (G_OBJECT (dialog), "wcs_step_button", "clicked", G_CALLBACK (wcs_step_cb));
/* Pertti's code end */

		set_named_callback(dialog, "wcs_ra_entry", "activate", wcsentry_cb);
		set_named_callback(dialog, "wcs_dec_entry", "activate", wcsentry_cb);
		set_named_callback(dialog, "wcs_h_scale_entry", "activate", wcsentry_cb);
		set_named_callback(dialog, "wcs_v_scale_entry", "activate", wcsentry_cb);
		set_named_callback(dialog, "wcs_equinox_entry", "activate", wcsentry_cb);
		set_named_callback(dialog, "wcs_rot_entry", "activate", wcsentry_cb);
		update_wcs_dialog(dialog, wcs);
		gtk_widget_show(dialog);
	} else {
//		update_fits_header_dialog(dialog, i_chan->fr);
		update_wcs_dialog(dialog, wcs);
		gdk_window_raise(dialog->window);
	}
}

void wcsedit_refresh(gpointer window)
{
	GtkWidget *dialog;
	struct wcs *wcs;

	dialog = g_object_get_data(G_OBJECT(window), "wcs_dialog");
	if (dialog == NULL) {
		return;
	}
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs == NULL) {
		return;
	}
	update_wcs_dialog(dialog, wcs);
}


static void update_wcs_dialog(GtkWidget *dialog, struct wcs *wcs)
{
	char buf[256];

	switch(wcs->wcsset) {
	case WCS_INVALID:
		set_named_checkb_val(dialog, "wcs_unset_rb", 1);
		/* also clear the fields here */
		return;
	case WCS_INITIAL:
		set_named_checkb_val(dialog, "wcs_initial_rb", 1);
		break;
	case WCS_FITTED:
		set_named_checkb_val(dialog, "wcs_fitted_rb", 1);
		break;
	case WCS_VALID:
		set_named_checkb_val(dialog, "wcs_valid_rb", 1);
		break;
	}
	degrees_to_dms_pr(buf, wcs->xref / 15.0, 2);
	named_entry_set(dialog, "wcs_ra_entry", buf);
	degrees_to_dms_pr(buf, wcs->yref, 1);
	named_entry_set(dialog, "wcs_dec_entry", buf);
	snprintf(buf, 255, "%.0f", wcs->equinox);
	named_entry_set(dialog, "wcs_equinox_entry", buf);
	snprintf(buf, 255, "%.6f", 3600 * wcs->xinc);
	named_entry_set(dialog, "wcs_h_scale_entry", buf);
	snprintf(buf, 255, "%.6f", 3600 * wcs->yinc);
	named_entry_set(dialog, "wcs_v_scale_entry", buf);
	snprintf(buf, 255, "%.4f", wcs->rot);
	named_entry_set(dialog, "wcs_rot_entry", buf);
}

/* push values from the dialog back into the wcs
 * if some values are missing, we try to guess them
 * return 0 if we could set the wcs from values, 1 if some
 * values were defaulted, 2 if there were changes,
 * -1 if we didn;t have enough data*/
static int wcs_dialog_to_wcs(GtkWidget *dialog, struct wcs *wcs)
{
	char *text = NULL, *end;
	double ra = INV_DBL, dec = INV_DBL, equ = INV_DBL,
		xs = INV_DBL, ys = INV_DBL, rot = INV_DBL, d;
	int ret = 0, chg = 0;

	/* parse the fields */
	text = named_entry_text(dialog, "wcs_ra_entry");
//	d3_printf("ra text is |%s|\n", text);
	if (!dms_to_degrees(text, &d))
		ra = d * 15.0;
//	d3_printf("got %.4f for ra\n", ra);
	g_free(text);
	text = named_entry_text(dialog, "wcs_dec_entry");
	if (!dms_to_degrees(text, &d))
		dec = d ;
//	d3_printf("got %.4f for dec\n", dec);
	g_free(text);
	text = named_entry_text(dialog, "wcs_equinox_entry");
	d = strtod(text, &end);
	if (text != end)
		equ = d ;
	g_free(text);
	text = named_entry_text(dialog, "wcs_h_scale_entry");
	d = strtod(text, &end);
	if (text != end)
		xs = d ;
	g_free(text);
	text = named_entry_text(dialog, "wcs_v_scale_entry");
	d = strtod(text, &end);
	if (text != end)
		ys = d ;
	g_free(text);
	text = named_entry_text(dialog, "wcs_rot_entry");
	d = strtod(text, &end);
	if (text != end)
		rot = d ;
	g_free(text);
/* now see what we can do with them */
	if (ra == INV_DBL || dec == INV_DBL) {
		err_printf("cannot set wcs: invalid ra/dec\n");
		return -1; /* can't do anything without those */
	}
	if (clamp_double(&ra, 0, 360.0))
		ret = 1;
	if (clamp_double(&dec, -90.0, 90.0))
		ret = 1;
	if (equ == INV_DBL) {
		equ = 2000.0;
		ret = 1;
	}
	if (xs == INV_DBL) {
		if (ys == INV_DBL)
			xs = -P_DBL(WCS_SEC_PER_PIX);
		else
			xs = ys;
		ret = 1;
	}
	if (ys == INV_DBL) {
		if (xs == INV_DBL)
			ys = -P_DBL(WCS_SEC_PER_PIX);
		else
			ys = xs;
		ret = 1;
	}
	if (rot == INV_DBL) {
		rot = 0.0;
		ret = 1;
	}
	if (clamp_double(&rot, 0, 360.0))
		ret = 1;
/* set chg if the wcs has been changed significantly */
//	d3_printf("diff is %f\n", fabs(ra - wcs->xref));
	if (fabs(ra - wcs->xref) > (1.5 / 36000)) {
		chg = 1;
		wcs->xref = ra;
	}
	if (fabs(dec - wcs->yref) > (1.0 / 36000)) {
		chg = 2;
		wcs->yref = dec;
	}
	if (fabs(rot - wcs->rot) > (1.0 / 9900)) {
		chg = 3;
		wcs->rot = rot;
	}
	if (fabs(equ - 1.0*wcs->equinox) > 2) {
		chg = 4;
		wcs->equinox = equ;
	}
	if (fabs(xs - 3600 * wcs->xinc) > (1.0 / 990000)) {
		chg = 5;
		wcs->xinc = xs / 3600.0;
	}
	if (fabs(ys - 3600 * wcs->yinc) > (1.0 / 990000)) {
		chg = 6;
		wcs->yinc = ys / 3600.0;
	}
	if (chg || ret)
		wcs->wcsset = WCS_INITIAL;
	if (ret)
		return ret;
	if (chg) {
//		d3_printf("chg is %d\n", chg);
		return 2;
	}
	return 0;
}

static void wcs_ok_cb(GtkWidget *wid, gpointer dialog)
{
	wcsentry_cb(NULL, dialog);
}

/* called on entry activate */
static int wcsentry_cb(GtkWidget *widget, gpointer dialog)
{
	GtkWidget *window;
	int ret;
	struct wcs *wcs;
	struct gui_star_list *gsl;

	window = g_object_get_data(G_OBJECT(dialog), "im_window");
	g_return_val_if_fail(window != NULL, 0);
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	g_return_val_if_fail(wcs != NULL, 0);

	ret = wcs_dialog_to_wcs(dialog, wcs);
//	d3_printf("wdtw returns %d\n", ret);
	if (ret > 0) {
		update_wcs_dialog(dialog, wcs);
		warning_beep();
		gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
		if (gsl != NULL) {
			cat_change_wcs(gsl->sl, wcs);
		}
		gtk_widget_queue_draw(window);
	} else if (ret < 0) {
		error_beep();
	}
	return ret;
}


void act_wcs_auto (GtkAction *action, gpointer window)
{
	/* we just do the "sgpw" here */
	act_stars_add_detected(NULL, window);
	act_stars_add_gsc(NULL, window);
	act_stars_add_tycho2(NULL, window);

	if (window_auto_pairs(window) < 1)
		return;

	window_fit_wcs(window);
	gtk_widget_queue_draw(window);

	wcsedit_refresh(window);
}

void act_wcs_quiet_auto (GtkAction *action, gpointer window)
{
	/* we just do the "sgpw<shift>f<shift>s" here */
	act_stars_add_detected(NULL, window);
	act_stars_add_gsc(NULL, window);
	act_stars_add_tycho2(NULL, window);

	if (window_auto_pairs(window) < 1)
		return;
	window_fit_wcs(window);

	act_stars_rm_field(NULL, window);
	act_stars_rm_detected(NULL, window);

	wcsedit_refresh(window);
}

void act_wcs_existing (GtkAction *action, gpointer window)
{
	/* we just do the "spw" here */
	act_stars_add_detected(NULL, window);
	if (window_auto_pairs(window) < 1)
		return;

	window_fit_wcs(window);
	gtk_widget_queue_draw(window);

	wcsedit_refresh(window);
}

void act_wcs_fit_pairs (GtkAction *action, gpointer window)
{
	window_fit_wcs(window);
	gtk_widget_queue_draw(window);

	wcsedit_refresh(window);
}

void act_wcs_reload (GtkAction *action, gpointer window)
{
	struct image_channel *i_chan;
	struct wcs *wcs;
	struct gui_star_list *gsl;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	i_chan = g_object_get_data(G_OBJECT(window), "i_channel");
	if (wcs == NULL || i_chan == NULL || i_chan->fr == NULL)
		return;

	wcs_from_frame(i_chan->fr, wcs);

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl != NULL)
		cat_change_wcs(gsl->sl, wcs);

	gtk_widget_queue_draw(window);

	wcsedit_refresh(window);
}

void act_wcs_validate (GtkAction *action, gpointer window)
{
	struct wcs *wcs;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs != NULL)
		wcs->wcsset = WCS_VALID;

	wcsedit_refresh(window);
}

void act_wcs_invalidate (GtkAction *action, gpointer window)
{
	struct wcs *wcs;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs != NULL)
		wcs->wcsset = WCS_INVALID;

	wcsedit_refresh(window);
}

/* a simulated wcs 'auto match' command */
/* should return -1 if no match found */
int match_field_in_window(gpointer window)
{
	struct wcs *wcs;

	act_wcs_auto (NULL, window);

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs == NULL) {
		err_printf("No WCS found\n");
		return -1;
	}
	if (wcs->wcsset != WCS_VALID) {
		err_printf("Cannot match field\n");
		return -1;
	}
	return 0;
}

/* a simulated wcs 'quiet auto match' command */
/* returns -1 if no match found */
int match_field_in_window_quiet(gpointer window)
{
	struct wcs *wcs;
	act_wcs_quiet_auto (NULL, window);
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs == NULL) {
		err_printf("No WCS found\n");
		return -1;
	}
	if (wcs->wcsset != WCS_VALID) {
		err_printf("Cannot match field\n");
		return -1;
	}
	return 0;
}

/* Pertti's code begin */

/* Move WCS field up */
static void wcs_north_cb(GtkWidget *wid, gpointer dialog)
{
	wcs_NS_cb(+1, wid, dialog);
}

/* Move WCS field down */
static void wcs_south_cb(GtkWidget *wid, gpointer dialog)
{
	wcs_NS_cb(-1, wid, dialog);
}

/* Move WCS field left */
static void wcs_east_cb(GtkWidget *wid, gpointer dialog)
{
	wcs_EW_cb(+1, wid, dialog);
}

/* Move WCS field right */
static void wcs_west_cb(GtkWidget *wid, gpointer dialog)
{
	wcs_EW_cb(-1, wid, dialog);
}

/* Move WCS vertically, only the dec text field is changed and WCS entries updated */
static void wcs_NS_cb(int dir, GtkWidget *wid, gpointer dialog)
{
	GtkWidget *button;
	char *text = NULL, buf[256];
	double d, dec = INV_DBL, step = INV_DBL;
	int btnstate;

	if (g_object_get_data(G_OBJECT(dialog), "im_window") == NULL) {
		return;
	}
	button = g_object_get_data(G_OBJECT(dialog), "wcs_step_button");
	btnstate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	text = named_entry_text(dialog, "wcs_dec_entry");
	if (!dms_to_degrees(text, &d))
		dec = d;
	g_free(text);
	text = named_entry_text(dialog, "wcs_v_scale_entry");
	if (!dms_to_degrees(text, &d))
		step = d * (double)dir * ( 1 + 9.0 * (btnstate > 0) ) / 3600.;
	g_free(text);
	if ( (dec != INV_DBL) && (step != INV_DBL) )
	{
		dec += step;
		degrees_to_dms_pr(buf, dec , 2);
		named_entry_set(dialog, "wcs_dec_entry", buf);
		wcsentry_cb(NULL, dialog);
	}
}

/* Move WCS horizontally, only the R.A. text field is changed and WCS entries updated */
static void wcs_EW_cb(int dir, GtkWidget *wid, gpointer dialog)
{
	GtkWidget *button;
	char *text = NULL, buf[256];
	double d, ra = INV_DBL, step = INV_DBL, dec = INV_DBL, cosdec;
	int btnstate;

	if (g_object_get_data(G_OBJECT(dialog), "im_window") == NULL) {
		return;
	}
	button = g_object_get_data(G_OBJECT(dialog), "wcs_step_button");
	btnstate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	text = named_entry_text(dialog, "wcs_ra_entry");
	if (!dms_to_degrees(text, &d))
		ra = d * 15.;
	g_free(text);
	text = named_entry_text(dialog, "wcs_dec_entry");
	if (!dms_to_degrees(text, &d))
		dec = d;
	g_free(text);
	text = named_entry_text(dialog, "wcs_h_scale_entry");
	if (!dms_to_degrees(text, &d))
		step = d * (double)dir * ( 1 + 9.0 * (btnstate > 0) ) / 3600.;
	g_free(text);

	if ( (ra != INV_DBL) && (step != INV_DBL) )
	{
		cosdec = cos(degrad(dec));
		if (cosdec < 1.85185185183088e-05)
			cosdec = 1.85185185183088e-05;
		ra += step / cosdec;
		if ( ra < 0 )
			ra += 360.;
		if ( ra >= 360 )
			ra -= 360.;
		degrees_to_dms_pr(buf, ra / 15. , 2);
		named_entry_set(dialog, "wcs_ra_entry", buf);
		wcsentry_cb(NULL, dialog);
	}
}

/* Rotate WCS clockwise */
static void wcs_rot_inc_cb(GtkWidget *wid, gpointer dialog)
{
	wcs_rot_cb(+1, wid, dialog);
}

/* Rotate WCS anti-clockwise */
static void wcs_rot_dec_cb(GtkWidget *wid, gpointer dialog)
{
	wcs_rot_cb(-1, wid, dialog);
}

/* Rotate WCS, only the rotation text field is changed and WCS entries updated */
static void wcs_rot_cb(int dir, GtkWidget *wid, gpointer dialog)
{
	GtkWidget *button;
	char *text = NULL, *end, buf[256];
	double d, rot = INV_DBL;
	int btnstate;

	if (g_object_get_data(G_OBJECT(dialog), "im_window") == NULL) {
		return;
	}
	button = g_object_get_data(G_OBJECT(dialog), "wcs_step_button");
	btnstate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	text = named_entry_text(dialog, "wcs_rot_entry");
	d = strtod(text, &end);
	if (text != end)
		rot = d ;
	g_free(text);
	if (rot != INV_DBL)
	{
		rot += (double)dir * (0.1 + 0.9 * (btnstate > 0));
		if (rot < 0)
			rot += 360.;
		if (rot >= 360.)
			rot -= 360.;
		snprintf(buf, 255, "%.4f", rot);
		named_entry_set(dialog, "wcs_rot_entry", buf);
		wcsentry_cb(NULL, dialog);
	}
}

static void wcs_step_cb(GtkWidget *wid, gpointer dialog)
{
	GtkWidget *button;
	int btnstate;

	button = g_object_get_data(G_OBJECT(dialog), "wcs_step_button");
	/* label = gtk_label_get_text(GTK_LABEL(GTK_BIN(button)->child)); */
	btnstate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	if (btnstate > 0)
		gtk_label_set_text(GTK_LABEL(GTK_BIN(button)->child), "x10");
	else
		gtk_label_set_text(GTK_LABEL(GTK_BIN(button)->child), "x1");
}
/* Pertti's code end */
