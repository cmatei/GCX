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

/* camera/telescope control dialog */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gcx.h"
#include "camera_indi.h"
#include "tele_indi.h"
#include "fwheel_indi.h"
#include "focus_indi.h"
#include "catalogs.h"
#include "gui.h"
#include "obsdata.h"
#include "params.h"
#include "obslist.h"
#include "cameragui.h"
#include "interface.h"
#include "misc.h"
#include "libindiclient/indigui.h"

#define AUTO_FILE_FORMAT "%s%03d"
#define DEFAULT_FRAME_NAME "frame"
#define DEFAULT_DARK_NAME "dark"

static void auto_filename(GtkWidget *dialog);
static int expose_cb(GtkWidget *window);

void status_message(GtkWidget *dialog, char *msg)
{
	GtkWidget *label;

	label = g_object_get_data(G_OBJECT(dialog), "statuslabel");
	if (label) {
		gtk_label_set_text(GTK_LABEL(label), msg);
	} else {
		info_printf("%s", msg);
	}
}

static void named_spin_set_limits(GtkWidget *dialog, char *name, double min, double max)
{
	GtkWidget *spin;
	GtkAdjustment *adj;

	g_return_if_fail(dialog != NULL);
	spin = g_object_get_data(G_OBJECT(dialog), name);
	g_return_if_fail(spin != NULL);

	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	g_return_if_fail(adj != NULL);

	g_object_set (adj, "lower", min, "upper", max, NULL);
}

/* Set current temperature in widget */
static int temperature_cb(GtkWidget *dialog)
{
	float fvalue, fmin, fmax;
	char buf[20];
	struct camera_t *camera;
	GtkWidget *main_window = g_object_get_data(G_OBJECT(dialog), "image_window");

	camera = camera_find(main_window, CAMERA_MAIN);
	if (! camera)
		return TRUE;

	camera_get_temperature(camera, &fvalue, &fmin, &fmax);
	sprintf(buf, "%.1f", fvalue);
	named_entry_set(dialog, "cooler_temp_entry", buf);
	//Return TRUE to keep callback active on temperature change
	return TRUE;
}

/* get the values from cam into the image dialog page */
void cam_to_img(GtkWidget *dialog)
{
	char buf[256];
	int mxsk, mysk;
	int binx, biny;
	int value, min, max;
	float fvalue, fmin, fmax;
	struct camera_t *camera;
	GtkWidget *main_window = g_object_get_data(G_OBJECT(dialog), "image_window");

	camera = camera_find(main_window, CAMERA_MAIN);
	if (! camera)
		return;

	g_object_set_data(G_OBJECT (dialog), "disable_signals", (void *)1);

	camera_get_binning(camera, &binx, &biny);
	sprintf(buf, "%dx%d", binx, biny);
	named_cbentry_set(dialog, "img_bin_combo", buf);

	camera_get_exposure_settings(camera, &fvalue, &fmin, &fmax);
	named_spin_set_limits(dialog, "img_exp_spin", fmin, fmax);
	named_spin_set(dialog, "img_exp_spin", fvalue);

	camera_get_size(camera, "WIDTH", &value, &min, &max);
	named_spin_set_limits(dialog, "img_width_spin", 0, 1.0 * max);
	named_spin_set(dialog, "img_width_spin", 1.0 * value);
	mxsk = max - value;

	camera_get_size(camera, "HEIGHT", &value, &min, &max);
	named_spin_set_limits(dialog, "img_height_spin", 0, 1.0 * max);
	named_spin_set(dialog, "img_height_spin", 1.0 * value);
	mysk = max - value;

	named_spin_set_limits(dialog, "img_x_skip_spin", 0, 1.0 * mxsk);
	camera_get_size(camera, "OFFX", &value, &min, &max);
	named_spin_set(dialog, "img_x_skip_spin", 1.0 * value);

	named_spin_set_limits(dialog, "img_y_skip_spin", 0, 1.0 * mysk);
	camera_get_size(camera, "OFFX", &value, &min, &max);
	named_spin_set(dialog, "img_y_skip_spin", 1.0 * value);

	camera_get_temperature(camera, &fvalue, &fmin, &fmax);
	named_spin_set_limits(dialog, "cooler_tempset_spin", fmin, fmax);
	named_spin_set(dialog, "cooler_tempset_spin", fvalue);
	temperature_cb(dialog);

	g_object_set_data(G_OBJECT (dialog), "disable_signals", 0L);
}

/* save a frame with the name specified by the dialog; increment seq number, etc */
void save_frame_auto_name(struct ccd_frame *fr, GtkWidget *dialog)
{
	char mb[1024];
	int seq;
	char fn[1024];
	char *text;

	auto_filename(dialog);
	text = named_entry_text(dialog, "file_entry");
	if (text == NULL || strlen(text) < 1) {
		if (text != NULL)
			g_free(text);
		text = g_strdup(DEFAULT_FRAME_NAME);
		named_entry_set(dialog, "file_entry", text);
	}
	seq = named_spin_get_value(dialog, "file_seqn_spin");
	check_seq_number(text, &seq);
	snprintf(fn, 1023, AUTO_FILE_FORMAT, text, seq);
	g_free(text);
	snprintf(mb, 1023, "Writing fits file: %s", fn);
	status_message(dialog, mb);
	d1_printf("%s\n", mb);
	wcs_to_fits_header(fr);
	if (get_named_checkb_val(GTK_WIDGET(dialog),
				 "file_compress_checkb")) {
		write_gz_fits_frame(fr, fn, P_STR(FILE_COMPRESS));
	} else {
		write_fits_frame(fr, fn);
	}
	seq ++;
	named_spin_set(dialog, "file_seqn_spin", seq);
}

static void dither_move(gpointer dialog, double amount)
{
	double dr, dd;
	char msg[128];

	do {
		dr = amount / 60.0 * (1.0 * random() / RAND_MAX - 0.5);
		dd = amount / 60.0 * (1.0 * random() / RAND_MAX - 0.5);
	} while (sqr(dr) + sqr(dd) < sqr(0.2 * amount / 60.0));

	snprintf(msg, 127, "Dither move: %.1f %.1f", 3600 * dr, 3600 * dd);
	status_message(dialog, msg);

//	err_printf("dither move TODO\n");

	/* FIXME: this crude hack allows me to dither for now. */
	system("/home/cmatei/dither-script");

}

static void scope_dither_cb( GtkWidget *widget, gpointer data )
{
	dither_move(data, P_DBL(TELE_DITHER_AMOUNT));
}


/* see if we need to save the frame and save it if we do; update names and
 * start the next frame if required */
static int maybe_save_frame(struct ccd_frame *fr, GtkWidget *dialog)
{
	GtkWidget *togb, *imwin;
	char mb[1024];
	int seq;
	char *text;

	imwin = g_object_get_data(G_OBJECT(dialog), "image_window");
	togb = g_object_get_data(G_OBJECT(dialog), "img_get_multiple_button");
	if (togb != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togb))) {
		if (imwin != NULL
		    && get_named_checkb_val(GTK_WIDGET(dialog), "file_match_wcs_checkb")
		    && !get_named_checkb_val(GTK_WIDGET(dialog), "img_dark_checkb"))
			match_field_in_window_quiet(imwin);

		save_frame_auto_name(fr, dialog);
                /* restart exposure if needed */
		text = named_entry_text(dialog, "current_frame_entry");
		seq = strtol(text, NULL, 10);
		g_free(text);

		d4_printf("Sequence %d\n", seq);
		if (seq > 0) {
			seq --;
			if (seq > 0) {
				if (P_INT(TELE_DITHER_ENABLE)) {
					dither_move(dialog, P_DBL(TELE_DITHER_AMOUNT));
				}

				if (capture_image(dialog)) {
					seq = 0;
					d4_printf("aborting sequence\n");
				}
			}
			sprintf(mb, "%d", seq);
			named_entry_set(dialog, "current_frame_entry", mb);
		}
		if (seq == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togb), 0);
			return FALSE;
		}

		/* don't remove the callback, wait for next exposure */
		return TRUE;
	}
	togb = g_object_get_data(G_OBJECT(dialog), "img_focus_button");
	if (togb != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togb))) {
		capture_image(dialog);

		return TRUE;
	}

	return FALSE;
}

static void set_filter_list(GtkWidget *dialog, struct fwheel_t *fw)
{

	printf("set_filter_list is not yet implemented\n");

	/* please don't use GtkCombo */
#if 0
	GtkWidget *combo;
	GList *filter_list = NULL;
	char **filters;

	combo = g_object_get_data(G_OBJECT(dialog), "obs_filter_combo");
	g_return_if_fail(combo != NULL);
	gtk_combo_disable_activate(GTK_COMBO(combo));
	filters = fwheel_get_filter_names(fw);
	d3_printf("got filters %p from fwheel %p\n", filters, fw);
	while (*filters != NULL) {
		filter_list = g_list_append(filter_list, *filters);
		filters ++;
	}
	if (filter_list != NULL) {
		gtk_signal_handler_block_by_data(G_OBJECT(GTK_COMBO(combo)->list), dialog);
		gtk_list_clear_items(GTK_LIST(GTK_COMBO(combo) -> list), 0, -1);
		gtk_combo_set_popdown_strings(GTK_COMBO(combo), filter_list);
		gtk_list_select_item(GTK_LIST(GTK_COMBO(combo) -> list), fw->filter);
		gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry), 0);
		gtk_signal_handler_unblock_by_data(G_OBJECT(GTK_COMBO(combo)->list), dialog);
	}
#endif
}

static void filter_list_select_cb(gpointer list, GtkWidget *widget,
				  gpointer dialog)
{
	printf("filter_list_select_cb is not yet implemented\n");
	/* please don't use GtkCombo */
#if 0
	int pos, ret;
	struct fwheel *fw;
	GtkWidget *combo;
	char msg[128];

	fw = g_object_get_data(G_OBJECT(dialog), "open_fwheel");
	if (fw == NULL)
		return;
	pos = gtk_list_child_position(list, widget);
	if (pos == fw->filter)
		return;
	combo = g_object_get_data(G_OBJECT(dialog), "obs_filter_combo");
	if (fw->state != FWHEEL_READY) {
		gtk_signal_handler_block_by_data(G_OBJECT(GTK_COMBO(combo)->list), dialog);
		gtk_list_select_item(GTK_LIST(GTK_COMBO(combo) -> list), fw->filter);
		gtk_signal_handler_unblock_by_data(G_OBJECT(GTK_COMBO(combo)->list), dialog);
		return;
	}
	ret = fwheel_goto_filter(fw, pos);
	if (!ret) {
		snprintf(msg, 128, "Selecting filter %d", pos);
		status_message(dialog, msg);
	} else {
		error_beep();
		/* reverting switching to the true filter */
		gtk_signal_handler_block_by_data(G_OBJECT(GTK_COMBO(combo)->list), dialog);
		gtk_list_select_item(GTK_LIST(GTK_COMBO(combo) -> list), fw->filter);
		gtk_signal_handler_unblock_by_data(G_OBJECT(GTK_COMBO(combo)->list), dialog);
	}
	gtk_widget_set_sensitive(combo, 0);
#endif
}

void close_cam_dialog( GtkWidget *widget, gpointer data )
{
}

/* we just hide the camera dialog, so it retains the settings for later */
gint delete_event( GtkWidget *widget, GdkEvent  *event, gpointer data )
{
	gtk_widget_hide(widget);
	return TRUE;
}


/* convert a bin string like "1 1" or "1,1" or "1x2" to
 * two ints; return 0 if successfull, -1 if couldn't parse
 */
static int parse_bin_string(char *text, int *bx, int *by)
{
	char *fe;
	*bx = strtol(text, &fe, 10);
	if (fe == text)
		return -1;
	while ((*fe != 0) && !isdigit(*fe))
		fe ++;
	if (*fe == 0)
		return -1;
	text = fe;
	*by = strtol(text, &fe, 10);
	if (fe == text)
		return -1;
	return 0;
}



/* read the exp settings from the img page into the cam structure */
void set_exp_from_img_dialog(struct camera_t *camera, GtkWidget *dialog)
{
	GtkComboBoxText *combo;
	char *text;
	int bx=1, by=1, ret;

	combo = g_object_get_data(G_OBJECT(dialog), "img_bin_combo");
	g_return_if_fail(combo != NULL);

	text = gtk_combo_box_text_get_active_text(combo);
	ret = parse_bin_string(text, &bx, &by);
	g_free(text);
	if (!ret) {
		camera_set_binning(camera, bx, by);
	}

	camera_set_size(camera,
		named_spin_get_value(dialog, "img_width_spin"),
		named_spin_get_value(dialog, "img_height_spin"),
		named_spin_get_value(dialog, "img_x_skip_spin"),
		named_spin_get_value(dialog, "img_y_skip_spin"));
	//cam_to_img(dialog);
}

int capture_image( GtkWidget *window)
{
	GtkWidget *main_window = g_object_get_data(G_OBJECT(window), "image_window");
	double exposure;
	struct camera_t *camera = camera_find(main_window, CAMERA_MAIN);

	if(! camera) {
		err_printf("no camera connected\n");
		return TRUE;
	}
	// Call expose_cb when image is ready
	INDI_set_callback(INDI_COMMON (camera), CAMERA_CALLBACK_EXPOSE, expose_cb, window);

	exposure = named_spin_get_value(window, "img_exp_spin"),

	camera_expose(camera, exposure);
	return FALSE;
}

static void img_get_image_cb(GtkWidget *widget, gpointer window)
{
	if (capture_image(window)) {
		//toggle_button_no_cb(window, run_button, "run_button_signal", 0);
	}
}


// This will be called when a new image is ready for processing
static int expose_cb(GtkWidget *window)
{
	GtkWidget *main_window;
	struct camera_t *camera;
	struct tele_t *tele;
	struct ccd_frame *fr;
	struct obs_data *obs;
	int r = FALSE;

	main_window = g_object_get_data(G_OBJECT(window), "image_window");
	tele = tele_find(window);
	camera = camera_find(main_window, CAMERA_MAIN);
	if(strncmp(camera->image_format, ".fits", 5) == 0) {
		// The image has already been unzipped if we get here
		// FILE *fh = fopen("guide.fit", "w+");
		// fwrite(camera->image, camera->image_size, 1, fh);
		// fclose(fh);

		fr = read_fits_file_from_mem(
			camera->image,
			camera->image_size,
			"guide.fit",
		 	0,
			NULL);

		if (!fr) {
			err_printf("Received an unreadable FITS from camera.");
			return FALSE;
		}
		if (! get_named_checkb_val(GTK_WIDGET(window), "img_dark_checkb")) {
			obs = (struct obs_data *)g_object_get_data(G_OBJECT(window), "obs_data");
			if (tele && obs) {
				obs->tele_ra = tele_get_ra(tele);
				obs->tele_dec = tele_get_dec(tele);
			}

			ccd_frame_add_obs_info(fr, obs);
		}
		if (!fr->stats.statsok)
			frame_stats(fr);
		frame_to_window (fr, main_window);
		r = maybe_save_frame(fr, window);
		release_frame(fr);
	} else {
		err_printf("Received unsupported image format: %s\n", camera->image_format);
	}

	//fwheel_poll(dialog);
	//obs_list_sm(dialog);

	// FALSE will remove the callback event
	return r;
}

/* called when the temp setpoint is changed */
static void cooler_temp_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *main_window = g_object_get_data(G_OBJECT(data), "image_window");
	struct camera_t *camera = camera_find(main_window, CAMERA_MAIN);
	double temp_set;

	if(! camera) {
		err_printf("no camera connected\n");
		return;
	}

	temp_set = gtk_spin_button_get_value (GTK_SPIN_BUTTON(widget));
	camera_set_temperature(camera, temp_set);
}

/* called when something on the img page is changed */
static void img_changed_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *main_window = g_object_get_data(G_OBJECT(data), "image_window");
	long *disable_signals = g_object_get_data(G_OBJECT(data), "disable_signals");
	struct camera_t *camera = camera_find(main_window, CAMERA_MAIN);
	if(! camera) {
		err_printf("no camera connected\n");
		return;
	}
	if (! disable_signals)
		set_exp_from_img_dialog(camera, GTK_WIDGET(data));
}

/* update obs fields in the dialog */
static void update_obs_entries( GtkWidget *dialog, struct obs_data *obs)
{
	char buf[256];
	double ha, airm;

	if (obs->objname != NULL)
		named_entry_set(dialog, "obs_object_entry", obs->objname);
	snprintf(buf, 256, "%.0f", obs->equinox);
	named_entry_set(dialog, "obs_epoch_entry", buf);
	degrees_to_dms(buf, obs->ra / 15.0);
	named_entry_set(dialog, "obs_ra_entry", buf);
	degrees_to_dms_pr(buf, obs->dec, 1);
	named_entry_set(dialog, "obs_dec_entry", buf);

	if (obs->filter != NULL)
		named_cbentry_set(dialog, "obs_filter_combo", obs->filter);
	ha = obs_current_hour_angle(obs);
	airm = obs_current_airmass(obs);
	snprintf(buf, 255, "HA: %.3f, Airmass: %.2f", ha, airm);
	named_label_set(dialog, "obj_comment_label", buf);

}


/* if we are in auto filename mode, generate a new name based on the
 * current obs. Reset the sequence number if the name has changed */
static void auto_filename(GtkWidget *dialog)
{
	struct obs_data *obs;
	char *text;
	char name[256];
	int seq = 1;

	if (!get_named_checkb_val(dialog, "file_auto_name_checkb"))
		return;

	text = named_entry_text(dialog, "file_entry");
	if (get_named_checkb_val(dialog, "img_dark_checkb")) {
		if (text == NULL || strcmp(text, DEFAULT_DARK_NAME)) {
			named_entry_set(dialog, "file_entry", DEFAULT_DARK_NAME);
			check_seq_number(DEFAULT_DARK_NAME, &seq);
			named_spin_set(dialog, "file_seqn_spin", seq);
		}
		g_free(text);
		return;
	}
	obs = g_object_get_data(G_OBJECT(dialog), "obs_data");
	if (obs == NULL) {
		d3_printf("no obs to set the filename from\n");
		g_free(text);
		return;
	}
	if (obs->filter != NULL)
		snprintf(name, 255, "%s-%s-", obs->objname, obs->filter);
	else
		snprintf(name, 255, "%s-", obs->objname);
	if (text == NULL || strcmp(text, name)) {
		named_entry_set(dialog, "file_entry", name);
		check_seq_number(name, &seq);
		named_spin_set(dialog, "file_seqn_spin", seq);
	}
	g_free(text);
}

/* called when the object or filter on the obs page is changed */
static void obsdata_cb( GtkWidget *widget, gpointer data )
{
	struct obs_data *obs;
	int ret;
	char *text, *end;
	char buf[128];
	GtkWidget *wid;
	GtkComboBoxText *combo;
	double d;

	obs = g_object_get_data(G_OBJECT(data), "obs_data");
	if (obs == NULL) {
		obs = obs_data_new();
		if (obs == NULL) {
			err_printf("cannot create new obs\n");
			return;
		}
		g_object_set_data_full(G_OBJECT(data), "obs_data", obs,
				       (GDestroyNotify) obs_data_release);
		combo = g_object_get_data(G_OBJECT(data), "obs_filter_combo");
		text = gtk_combo_box_text_get_active_text(combo);
		replace_strval(&obs->filter, text);
	}
	wid = g_object_get_data(G_OBJECT(data), "obs_object_entry");
	if (widget == wid) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		d3_printf("looking up %s\n", text);
		ret = obs_set_from_object(obs, text);
		if (ret < 0) {
			snprintf(buf, 128, "Cannot find object %s", text);
			status_message(data, buf);
			replace_strval(&obs->objname, text);
			auto_filename(data);
			return;
		}
		g_free(text);
		update_obs_entries(data, obs);
		auto_filename(data);
		return;
	}
	wid = g_object_get_data(G_OBJECT (data), "obs_filter_combo");
	if (widget == wid) {
		text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (widget));
		d3_printf("obs_cb: setting filter to %s\n", text);
		replace_strval(&obs->filter, text);
		update_obs_entries(data, obs);
		auto_filename(data);
		return;
	}
	wid = g_object_get_data(G_OBJECT(data), "obs_ra_entry");
	if (widget == wid) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		if (!dms_to_degrees(text, &d)) {
			obs->ra = d * 15.0;
			update_obs_entries(data, obs);
		} else {
			error_beep();
			status_message(data, "Bad R.A. Value");
		}
		g_free(text);
		return;
	}
	wid = g_object_get_data(G_OBJECT(data), "obs_dec_entry");
	if (widget == wid) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		if (!dms_to_degrees(text, &d)) {
			obs->dec = d;
			update_obs_entries(data, obs);
		} else {
			error_beep();
			status_message(data, "Bad Declination Value");
		}
		g_free(text);
		return;
	}
	wid = g_object_get_data(G_OBJECT(data), "obs_epoch_entry");
	if (widget == wid) {
		text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
		d = strtod(text, &end);
		if (text != end) {
			obs->equinox = d;
			update_obs_entries(data, obs);
		} else {
			error_beep();
			status_message(data, "Bad Equinox Value");
		}
		g_free(text);
		return;
	}
}

static gboolean obsdata_focus_out_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	obsdata_cb(GTK_WIDGET(widget), data);
	return FALSE;
}


/* external interface for setting the obs object */
/* returns 0 if the object was found */
int set_obs_object(GtkWidget *dialog, char *objname)
{
	GtkWidget *wid;
	wid = g_object_get_data(G_OBJECT(dialog), "obs_object_entry");
	g_return_val_if_fail(wid != NULL, -1);
	named_entry_set(dialog, "obs_object_entry", objname);
	obsdata_cb(wid, dialog);
	return 0;
}


/* set the image window to a ratio of the full sensor area */
static void reset_img_window_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *main_window = g_object_get_data(G_OBJECT(data), "image_window");
	struct camera_t *camera = camera_find(main_window, CAMERA_MAIN);
	int val, min, max_w, max_h, width, height, skip_x, skip_y;

	if(! camera) {
		err_printf("no camera connected\n");
		return;
	}
	camera_get_size(camera, "WIDTH", &val, &min, &max_w);
	camera_get_size(camera, "HEIGHT", &val, &min, &max_h);

	width = max_w;
	height = max_h;
	if (widget == g_object_get_data(G_OBJECT(data), "img_halfsz")) {
		width = width * 1000 / 1414;
		height = height * 1000 / 1414;
	}
	if (widget == g_object_get_data(G_OBJECT(data), "img_quartersz")) {
		width = width * 1000 / 2000;
		height = height * 1000 / 2000;
	}
	if (widget == g_object_get_data(G_OBJECT(data), "img_eighthsz")) {
		width = width * 1000 / 4000;
		height = height * 1000 / 4000;
	}

	skip_x = (max_w - width) / 2;
	skip_y = (max_h - height) / 2;
	camera_set_size(camera, width, height, skip_x, skip_y);
	cam_to_img(data);
}

static void img_multiple_cb( GtkWidget *widget, gpointer data )
{
	char buf[64];
	int nf;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
		//gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), "Multiple");
		g_object_set (widget, "label", "Multiple", NULL);
		return;
	}


	//gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), "Stop");
	g_object_set (widget, "label", "Stop", NULL);

	nf = named_spin_get_value(data, "img_number_spin");
	snprintf(buf, 63, "%d", nf);
	named_entry_set(data, "current_frame_entry", buf);
	img_get_image_cb(widget, data);
}

static void img_focus_cb( GtkWidget *widget, gpointer data )
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
		//gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), "Focus");
		g_object_set (widget, "label", "Focus", NULL);
		return;
	}

	// gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), "Stop");
	g_object_set (widget, "label", "Stop", NULL);

	img_get_image_cb(widget, data);
}

/* send an abort slew message */
static void scope_abort_cb( GtkWidget *widget, gpointer data )
{
	GtkWidget *main_window = g_object_get_data(G_OBJECT(data), "image_window");
	struct tele_t *tele = tele_find(main_window);

	if (tele) {
		tele_abort(tele);
		status_message(data, "Aborted");
		err_printf("Aborted");
	}
}


/* slew to the current obs coordinates */
static int scope_goto_cb( GtkWidget *widget, gpointer data )
{
	struct obs_data *obs;
	int ret;
	char msg[512];
	struct tele_t *tele;
	GtkWidget *main_window = g_object_get_data(G_OBJECT(data), "image_window");

	tele = tele_find(main_window);
	if (! tele) {
		err_printf("No telescope found\n");
		return -1;
	}

	obs = g_object_get_data(G_OBJECT(data), "obs_data");
	if (obs == NULL)
		return -1;

	ret = obs_check_limits(obs, data);
	if (ret) {
		snprintf(msg, 511, "Object %s (ha=%.2f dec=%.2f)\n"
			 "is outside slew limits\n"
			 "%s\nStart slew?", obs->objname,
			 obs_current_hour_angle(obs), obs->dec, last_err());
		if (modal_yes_no(msg, NULL) != 1)
			return -1;
	}
	ret = tele_set_coords(tele, TELE_COORDS_SLEW, obs->ra, obs->dec, obs->equinox);
	if (ret) {
		err_printf("Failed to slew to target\n");
		return -1;
	}
	return 0;
}

/* sync to the current obs coordinates */
static void scope_sync_cb( GtkWidget *widget, gpointer data )
{
	struct obs_data *obs;
	int ret;
	struct tele_t *tele;
	GtkWidget *main_window = g_object_get_data(G_OBJECT(data), "image_window");

	tele = tele_find(main_window);

	if (! tele) {
		err_printf("No telescope found\n");
		return;
	}

	obs = g_object_get_data(G_OBJECT(data), "obs_data");
	if (obs == NULL) {
		error_beep();
		status_message(data, "Set obs first");
		return;
	}
	ret = tele_set_coords(tele, TELE_COORDS_SYNC, obs->ra, obs->dec, obs->equinox);
	if (!ret) {
		status_message(data, "Synchronised");
	} else {
		err_printf("Failed to synchronize\n");
	}
}

/* external interface for slew_cb */
int goto_dialog_obs(GtkWidget *dialog)
{
	return scope_goto_cb(NULL, dialog);
}


/* correct the scope's pointing with the difference between
 * the current image window wcs (if fitted) and the telescope
 * position.  Note that this function returns once the movement
 * has been submitted, and does not wait for movement to complete */
static int scope_auto_cb( GtkWidget *widget, gpointer data )
{
	struct wcs *wcs;
	GtkWidget *imw;
	struct obs_data *obs;
	struct tele_t *tele;

	imw = g_object_get_data(G_OBJECT(data), "image_window");
	g_return_val_if_fail(imw != NULL, -1);
	tele = tele_find(imw);
	if (! tele)
		return -1;

	obs = g_object_get_data(G_OBJECT(data), "obs_data");
	if (obs == NULL)
		return -1;

	wcs = g_object_get_data(G_OBJECT(imw), "wcs_of_window");
	g_return_val_if_fail(wcs != NULL, -1);
	g_return_val_if_fail(wcs->wcsset != WCS_INVALID, -1);

	tele_center_move(tele, obs->ra - wcs->xref, obs->dec - wcs->yref);
	return 0;
}

/* external interface for auto_cb */
int center_matched_field(GtkWidget *dialog)
{
	return scope_auto_cb(NULL, dialog);
}

static void enable_camera_widgets(GtkWidget *dialog, int state)
{

	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "img_get_img_button")),
		state);
	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "img_get_multiple_button")),
		state);
	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "img_focus_button")),
		state);
	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "img_abort_button")),
		state);

}

static void enable_telescope_widgets(GtkWidget *dialog, int state)
{

	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "scope_goto_button")),
		state);
	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "scope_abort_button")),
		state);
	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "scope_auto_button")),
		state);
	gtk_widget_set_sensitive(
		GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "scope_sync_button")),
		state);
}

static gboolean cam_ready_cb(gpointer data)
{
	GtkWidget *window = data;
	GtkWidget *dialog;
	struct camera_t *camera;

	dialog = g_object_get_data(G_OBJECT(window), "cam_dialog");
	camera = camera_find(window, CAMERA_MAIN);
	if (camera) {
		INDI_set_callback(INDI_COMMON (camera), CAMERA_CALLBACK_TEMPERATURE, temperature_cb, dialog);
		enable_camera_widgets(dialog, TRUE);
		cam_to_img(dialog);

	}
	//Return FALSE to remove the callback event
	return FALSE;
}

static gboolean tele_ready_cb(gpointer data)
{
	GtkWidget *window = data;
	GtkWidget *dialog;
	struct tele_t *tele;

	dialog = g_object_get_data(G_OBJECT(window), "cam_dialog");
	tele = tele_find(window);
	if (tele) {
		enable_telescope_widgets(dialog, TRUE);
	}
	//Return FALSE to remove the callback event
	return FALSE;
}

static gboolean fwheel_ready_cb(gpointer data)
{
	GtkWidget *window = data;
	GtkWidget *dialog;
	struct fwheel_t *fwheel;

	dialog = g_object_get_data(G_OBJECT(window), "cam_dialog");
	fwheel = fwheel_find(window);
	if (fwheel) {
		gtk_widget_set_sensitive(
			GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "fwheel_combo")),
			TRUE);
		set_filter_list(dialog, fwheel);
	}
	//Return FALSE to remove the callback event
	return FALSE;
}

static gboolean focuser_ready_cb(gpointer data)
{
	return FALSE;
}

void cam_set_callbacks(GtkWidget *dialog)
{
	GtkWidget *combo;

	g_signal_connect (G_OBJECT (dialog), "delete_event",
			  G_CALLBACK (delete_event), dialog);
	set_named_callback(dialog, "scope_goto_button", "clicked", scope_goto_cb);
	set_named_callback(dialog, "scope_auto_button", "clicked", scope_auto_cb);
	set_named_callback(dialog, "scope_sync_button", "clicked", scope_sync_cb);
//	set_named_callback(dialog, "scope_align_button", "clicked", scope_align_cb);
	set_named_callback(dialog, "scope_abort_button", "clicked", scope_abort_cb);
	set_named_callback(dialog, "scope_dither_button", "clicked", scope_dither_cb);
	set_named_callback(dialog, "obs_list_abort_button", "clicked", scope_abort_cb);
	set_named_callback(dialog, "obs_list_file_button", "clicked", obs_list_select_file_cb);
	set_named_callback(dialog, "img_get_img_button", "clicked", img_get_image_cb);
	set_named_callback(dialog, "img_fullsz", "clicked", reset_img_window_cb);
	set_named_callback(dialog, "img_halfsz", "clicked", reset_img_window_cb);
	set_named_callback(dialog, "img_quartersz", "clicked", reset_img_window_cb);
	set_named_callback(dialog, "img_eighthsz", "clicked", reset_img_window_cb);
	set_named_callback(dialog, "img_focus_button", "toggled", img_focus_cb);
	set_named_callback(dialog, "img_get_multiple_button", "toggled", img_multiple_cb);
	set_named_callback(dialog, "img_width_spin", "changed", img_changed_cb);
	set_named_callback(dialog, "img_height_spin", "changed", img_changed_cb);
	//set_named_callback(dialog, "img_exp_spin", "changed", img_changed_cb);
	combo = g_object_get_data(G_OBJECT(dialog), "img_bin_combo");

	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (img_changed_cb), dialog);
	combo = g_object_get_data(G_OBJECT(dialog), "obs_filter_combo");
	g_signal_connect (G_OBJECT (combo), "changed",
			  G_CALLBACK (filter_list_select_cb), dialog);
	set_named_callback(dialog, "obs_object_entry", "activate", obsdata_cb);
	set_named_callback(dialog, "obs_object_entry", "focus-out-event",
			   obsdata_focus_out_cb);
	set_named_callback(dialog, "obs_ra_entry", "activate", obsdata_cb);
	set_named_callback(dialog, "obs_ra_entry", "focus-out-event",
			   obsdata_focus_out_cb);
	set_named_callback(dialog, "obs_dec_entry", "activate", obsdata_cb);
	set_named_callback(dialog, "obs_dec_entry", "focus-out-event",
			   obsdata_focus_out_cb);
	set_named_callback(dialog, "obs_epoch_entry", "activate", obsdata_cb);
	set_named_callback(dialog, "obs_epoch_entry", "focus-out-event",
			   obsdata_focus_out_cb);
	set_named_callback(dialog, "obs_list_fname", "activate", obs_list_fname_cb);
	set_named_callback(dialog, "cooler_tempset_spin", "changed", cooler_temp_cb);
	obs_list_callbacks(dialog);
}

/* initialise the telescope page params from pars */
static void set_scope_params_from_par(gpointer dialog)
{
	named_spin_set(dialog, "e_limit_spin", P_DBL(TELE_E_LIMIT));
	set_named_checkb_val(dialog, "e_limit_checkb", P_INT(TELE_E_LIMIT_EN));
	named_spin_set(dialog, "w_limit_spin", P_DBL(TELE_W_LIMIT));
	set_named_checkb_val(dialog, "w_limit_checkb", P_INT(TELE_W_LIMIT_EN));
	named_spin_set(dialog, "n_limit_spin", P_DBL(TELE_N_LIMIT));
	set_named_checkb_val(dialog, "n_limit_checkb", P_INT(TELE_N_LIMIT_EN));
	named_spin_set(dialog, "s_limit_spin", P_DBL(TELE_S_LIMIT));
	set_named_checkb_val(dialog, "s_limit_checkb", P_INT(TELE_S_LIMIT_EN));
}

#if 0
void act_control_camera (GtkAction *action, gpointer window)
{
	GtkWidget *dialog;
	GtkWidget* create_camera_control (void);

	dialog = g_object_get_data(G_OBJECT(window), "cam_dialog");
	if (dialog == NULL) {
		dialog = create_camera_control();
		g_object_ref(dialog);
		g_object_set_data_full(G_OBJECT(window), "cam_dialog", dialog,
				       (GDestroyNotify)gtk_widget_destroy);

		g_object_set_data(G_OBJECT(dialog), "image_window", window);
		g_signal_connect (G_OBJECT(dialog), "destroy",
				  G_CALLBACK(close_cam_dialog), window);

//		set_filter_list(dialog);

		cam_set_callbacks(dialog);
		set_scope_params_from_par(dialog);

//		cam_to_focus(dialog);
		if (! camera_find(window, CAMERA_MAIN)) {
			enable_camera_widgets(dialog, FALSE);
			camera_set_ready_callback(window, CAMERA_MAIN, cam_ready_cb, window);
		}
		if (! tele_find(window)) {
			enable_telescope_widgets(dialog, FALSE);
			tele_set_ready_callback(window, tele_ready_cb, window);
		}
		if (! fwheel_find(window)) {
			gtk_widget_set_sensitive(
				GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "obs_filter_combo")),
				FALSE);
			fwheel_set_ready_callback(window, fwheel_ready_cb, window);
		}
		if (! focuser_find(window)) {
			focuser_set_ready_callback(window, focuser_ready_cb, window);
		}

		cam_to_img(dialog);
		gtk_widget_show_all(dialog);
	} else {
		gtk_window_present (GTK_WINDOW (dialog));
	}
//	cam_dialog_update(dialog);
//	cam_dialog_edit(dialog);
}

#endif
