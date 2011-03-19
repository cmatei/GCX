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

/* Here we handle the main image window and menus */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "sourcesdraw.h"
#include "params.h"
#include "wcs.h"
#include "cameragui.h"
#include "filegui.h"
#include "interface.h"
#include "misc.h"
#include "query.h"

/* yes/no modal dialog */

static void yes_no_yes( GtkWidget *widget, gpointer data )
{
	int *retv;
	retv = g_object_get_data(G_OBJECT(data), "return_val");
	if (retv != NULL)
		*retv = 1;
	gtk_widget_destroy(data);
}

static void yes_no_no( GtkWidget *widget, gpointer data )
{
	gtk_widget_destroy(data);
}

static void close_yes_no( GtkWidget *widget, gpointer data )
{
	gtk_main_quit();
}


/* create the modal y/n dialog and run the mainloop until it exits
 * display text as a bunch of labels. newlines inside the text
 * return 0/1 for n/y, or -1 for errors */
/* title is the window title; if NULL, 'gcx question' is used */
int modal_yes_no(char *text, char *title)
{
	int retval = 0;

	GtkWidget *dialog, *label, *vbox;
	dialog = create_yes_no();
	g_object_set_data(G_OBJECT(dialog), "return_val", &retval);
	vbox = g_object_get_data(G_OBJECT(dialog), "vbox");
	g_return_val_if_fail(vbox != NULL, 0);
	g_signal_connect (G_OBJECT (dialog), "destroy",
			    G_CALLBACK (close_yes_no), dialog);
	set_named_callback (G_OBJECT (dialog), "yes_button", "clicked",
			    G_CALLBACK (yes_no_yes));
	set_named_callback (G_OBJECT (dialog), "no_button", "clicked",
			    G_CALLBACK (yes_no_no));
	if (title != NULL) {
		gtk_window_set_title(GTK_WINDOW(dialog), title);
	}
	if (text != NULL) { /* add the message */
		label = gtk_label_new (text);
		g_object_ref (label);
		g_object_set_data_full (G_OBJECT (dialog), "message", label,
					(GDestroyNotify) g_object_unref);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
		gtk_misc_set_alignment (GTK_MISC (label), 0.5, 1);
		gtk_widget_show (label);
	}
	gtk_widget_show(dialog);
	gtk_main();
	return retval;
}

static void entry_prompt_yes( GtkWidget *widget, gpointer data )
{
	int *retv;
	char **value;
	GtkWidget *entry;

	entry = g_object_get_data(G_OBJECT(data), "entry");
	g_return_if_fail(entry != NULL);
	value = g_object_get_data(G_OBJECT(data), "entry_val");
	if (value != NULL) {
		*value = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	}
	retv = g_object_get_data(G_OBJECT(data), "return_val");
	if (retv != NULL)
		*retv = 1;

	gtk_widget_destroy(data);
}


/* prompt the user for a value; return 1 if ok was pressed, 0 otehrwise
 * update the value pointer with a maloced string when ok was pressed */

int modal_entry_prompt(char *text, char *title, char *initial, char **value)
{
	int retval = 0;

	GtkWidget *dialog, *label, *entry;
	dialog = create_entry_prompt();
	g_object_set_data(G_OBJECT(dialog), "return_val", &retval);
	g_object_set_data(G_OBJECT(dialog), "entry_val", value);
	entry = g_object_get_data(G_OBJECT(dialog), "entry");
	g_return_val_if_fail(entry != NULL, 0);
	label = g_object_get_data(G_OBJECT(dialog), "label");
	g_return_val_if_fail(entry != NULL, 0);
	g_signal_connect (G_OBJECT (dialog), "destroy",
			    G_CALLBACK (close_yes_no), dialog);
	set_named_callback (G_OBJECT (dialog), "ok_button", "clicked",
			    G_CALLBACK (entry_prompt_yes));
	set_named_callback (G_OBJECT (dialog), "entry", "activate",
			    G_CALLBACK (entry_prompt_yes));
	set_named_callback (G_OBJECT (dialog), "cancel_button", "clicked",
			    G_CALLBACK (yes_no_no));
	if (title != NULL) {
		gtk_window_set_title(GTK_WINDOW(dialog), title);
	}
	if (text != NULL) { /* add the message */
		gtk_label_set_text(GTK_LABEL(label), text);
	}
	if (initial != NULL) {
		gtk_entry_set_text(GTK_ENTRY(entry), initial);
	}
	gtk_widget_show(dialog);
	gtk_main();
	return retval;
}





GtkWidget* create_about_cx (void);

/* beep - or otherwise show a user action may be in error */
void error_beep(void)
{
	gdk_beep();
}

void warning_beep(void)
{
//	gdk_beep();
}

#define ERR_SIZE 1024
/* print the error string and save it to storage */
int info_printf_sb2(gpointer window, const char *fmt, ...)
{
	va_list ap, ap2;
	int ret;
	char err_string[ERR_SIZE+1];
	GtkWidget *label;

#ifdef __va_copy
	__va_copy(ap2, ap);
#else
	ap2 = ap;
#endif
	va_start(ap, fmt);
	va_start(ap2, fmt);
	ret = vsnprintf(err_string, ERR_SIZE, fmt, ap2);
	if (ret > 0 && err_string[ret-1] == '\n')
		err_string[ret-1] = 0;
	d1_printf("%s\n", err_string);
	label = g_object_get_data(G_OBJECT(window), "statuslabel2");
	gtk_label_set_text(GTK_LABEL(label), err_string);
	va_end(ap);
	return ret;
}


#define ERR_SIZE 1024
/* print the error string and save it to storage */
int err_printf_sb2(gpointer window, const char *fmt, ...)
{
	va_list ap, ap2;
	int ret;
	char err_string[ERR_SIZE+1];
	GtkWidget *label;

#ifdef __va_copy
	__va_copy(ap2, ap);
#else
	ap2 = ap;
#endif
	va_start(ap, fmt);
	va_start(ap2, fmt);
	ret = vsnprintf(err_string, ERR_SIZE, fmt, ap2);
	if (ret > 0 && err_string[ret-1] == '\n')
		err_string[ret-1] = 0;
	err_printf("%s\n", err_string);
	label = g_object_get_data(G_OBJECT(window), "statuslabel2");
	gtk_label_set_text(GTK_LABEL(label), err_string);
	va_end(ap);
	return ret;
}


static void about_cx_action(GtkAction *action, gpointer data)
{
	GtkWidget *about_cx;
	about_cx = create_about_cx();
	g_object_ref(about_cx);
	gtk_widget_show(about_cx);
}

void close_about_cx( GtkWidget *widget, gpointer data )
{
	gtk_widget_destroy(GTK_WIDGET(data));
}

static int destroy_cb(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
	return 1;
}

static void user_quit_action(GtkAction *action, gpointer window)
{
	d3_printf("user quit\n");
	gtk_widget_destroy(GTK_WIDGET(window));
}

int window_auto_pairs(gpointer window)
{
	struct gui_star_list *gsl;
	int ret;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return -1;
	info_printf_sb2(window, "Looking for Star Pairs...");
	while (gtk_events_pending ())
		gtk_main_iteration ();
	remove_pairs(window, 0);
	ret = auto_pairs(gsl);
	if (ret < 1) {
		info_printf_sb2(window, "%s\n", last_err());
	} else {
		info_printf_sb2(window, "Found %d Matching Pairs", ret);
	}
	gtk_widget_queue_draw(window);
	return ret;
}

void star_pairs_action(GtkAction *action, gpointer window)
{
	window_auto_pairs(window);
}

static void new_frame_action (GtkAction *action, gpointer window)
{
	struct ccd_frame *fr;
	fr = new_frame(P_INT(FILE_NEW_WIDTH), P_INT(FILE_NEW_HEIGHT));
	frame_to_channel(fr, window, "i_channel");
}

void star_rm_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *window = data;
	int ret;

	switch(action) {
	case STAR_RM_ALL:
		remove_stars(window, TYPE_MASK_ALL, 0);
		break;
	case STAR_RM_FR:
		remove_stars(window, TYPE_MASK(STAR_TYPE_SIMPLE), 0);
		break;
	case STAR_RM_USER:
		remove_stars(window, TYPE_MASK(STAR_TYPE_USEL), 0);
		break;
	case STAR_RM_FIELD:
		remove_stars(window, TYPE_MASK(STAR_TYPE_SREF), 0);
		break;
	case STAR_RM_CAT:
		remove_stars(window, TYPE_MASK(STAR_TYPE_CAT), 0);
		break;
	case STAR_RM_SEL:
		remove_stars(window, TYPE_MASK_ALL, STAR_SELECTED);
		break;
	case STAR_RM_PAIRS_ALL:
		remove_pairs(window, 0);
		break;
	case STAR_RM_PAIRS_SEL:
		remove_pairs(window, STAR_SELECTED);
		break;
	case STAR_RM_OFF:
		ret = remove_off_frame_stars(window);
		if (ret >= 0) {
			info_printf_sb2(window, "stars", 5000,
				       " %d off-frame stars removed", ret);
		}
		break;
	default:
		err_printf("unknown action %d in star_rm_cb\n", action);
	}
}

void stars_rm_all_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_ALL, NULL);
}

void stars_rm_selected_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_SEL, NULL);
}

void stars_rm_user_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_USER, NULL);
}

void stars_rm_field_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_FIELD, NULL);
}

void stars_rm_cat_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_CAT, NULL);
}

void stars_rm_off_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_OFF, NULL);
}

void stars_rm_detected_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_FR, NULL);
}

void stars_rm_pairs_all_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_PAIRS_ALL, NULL);
}

void stars_rm_pairs_sel_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_PAIRS_SEL, NULL);
}


void destroy( GtkWidget *widget,
              gpointer   data )
{
	gtk_main_quit ();
}

void show_xy_status(GtkWidget *window, double x, double y)
{
	info_printf_sb2(window, "%.0f, %.0f", x, y);
}


/*
 * mouse button event callback. It is normally called after the callback in
 * sourcesdraw has decided not to act on the click
 */
static gboolean image_clicked_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	GtkMenu *menu, *star_popup;
	GSList *found;

//	printf("button press : %f %f state %08x button %08x \n",
//	       event->x, event->y, event->state, event->button);
	if (event->button == 3) {
		show_region_stats(data, event->x, event->y);
		if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
		    return FALSE;
		found = stars_under_click(GTK_WIDGET(data), event);
		star_popup = g_object_get_data(G_OBJECT(data), "star_popup");
		menu = g_object_get_data(G_OBJECT(data), "image_popup");
		if (found != NULL && star_popup != NULL) {
			if (found != NULL)
				g_slist_free(found);
			return FALSE;
		}
		if(menu) {
			// d4_printf("menu=%p\n", menu);
			gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
				       event->button, event->time);
		}
	}
	if (event->button == 2) {
		show_region_stats(data, event->x, event->y);
		if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
		    return FALSE;
		pan_cursor(data);
	}
	if (event->button == 1) {
		found = stars_under_click(GTK_WIDGET(data), event);
		if (!(event->state & GDK_SHIFT_MASK) && (found == NULL))
			gsl_unselect_all(data);
		if ((event->state & GDK_CONTROL_MASK) || (found != NULL)) {
			g_slist_free(found);
			return FALSE;
		}
		show_region_stats(data, event->x, event->y);
	}
	return TRUE;
}

#define DRAG_MIN_MOVE 2
static gint motion_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer window)
{
	int x, y, dx, dy;
	GdkModifierType state;
	static int ox, oy;

	if (event->is_hint)
		gdk_window_get_pointer (event->window, &x, &y, &state);
	else
	{
		x = event->x;
		y = event->y;
		state = event->state;
	}
//	d3_printf("motion %d %d state %d\n", x - ox, y - oy, state);
//	show_xy_status(window, 1.0 * x, 1.0 * y);
	if (state & GDK_BUTTON1_MASK) {
		dx = x - ox;
		dy = y - oy;
//		printf("moving by %d %d\n", x - ox, y - oy);
		if (abs(dx) > DRAG_MIN_MOVE || abs(dy) > DRAG_MIN_MOVE) {
			if (dx > DRAG_MIN_MOVE)
				dx -= DRAG_MIN_MOVE;
			else if (dx < -DRAG_MIN_MOVE)
				dx += DRAG_MIN_MOVE;
			else
				dx = 0;
			if (dy > DRAG_MIN_MOVE)
				dy -= DRAG_MIN_MOVE;
			else if (dy < -DRAG_MIN_MOVE)
				dy += DRAG_MIN_MOVE;
			else
				dy = 0;
			drag_adjust_cuts(window, dx, dy);
			ox = x;
			oy = y;
		}
	} else {
		ox = x;
		oy = y;
	}
	return TRUE;
}


/* name, stock id, label, accel, tooltip, callback */
static GtkActionEntry star_popup_actions[] = {
	{ "star-popup", NULL, NULL },

	{ "star-edit",         NULL, "_Edit Star",     NULL, NULL, G_CALLBACK (starp_edit_ap_action) },
	{ "star-remove",       NULL, "_Remove Star",   NULL, NULL, G_CALLBACK (starp_unmark_star_action) },
	{ "star-pair-add",     NULL, "_Create Pair",   NULL, NULL, G_CALLBACK (starp_pair_add_action) },
	{ "star-pair-rm",      NULL, "Remo_ve Pair",   NULL, NULL, G_CALLBACK (starp_pair_rm_action) },
	{ "star-move",         NULL, "Move _Star",     NULL, NULL, G_CALLBACK (starp_move_star_action) },
	{ "star-plot-profile", NULL, "Plot _Profile",  NULL, NULL, G_CALLBACK (starp_plot_profile_action) },
	{ "star-measure",      NULL, "_Measure",       NULL, NULL, G_CALLBACK (starp_measure_star_action) },
	{ "star-plot-skyhist", NULL, "Sky _Histogram", NULL, NULL, G_CALLBACK (starp_plot_skyhist_action) },
	{ "star-fit-psf",      NULL, "_Fit Psf",       NULL, NULL, G_CALLBACK (starp_fit_psf_action) },
};

GtkWidget *get_star_popup_menu (gpointer window)
{
	GtkWidget *ret;
	GtkUIManager *ui;
	GError *error;
	GtkActionGroup *action_group;
	static char *star_popup_ui =
		"  <popup name='star-popup'>"
		"    <menu name='starp' action='star-popup'>"
		"      <menuitem name='Edit Star'     action='star-edit'/>"
		"      <menuitem name='Remove Star'   action='star-remove'/>"
		"      <menuitem name='Create Pair'   action='star-pair-add'/>"
		"      <menuitem name='Remove Pair'   action='star-pair-rm'/>"
		"      <menuitem name='Move Star'     action='star-move'/>"
		"      <menuitem name='Plot Profile'  action='star-plot-profile'/>"
		"      <menuitem name='Measure'       action='star-measure'/>"
		"      <menuitem name='Sky Histogram' action='star-plot-skyhist'/>"
		"      <menuitem name='Fit PSF'       action='star-fit-psf'/>"
		"    </menu>"
		"  </popup>";

	action_group = gtk_action_group_new ("StarPopupActions");
	gtk_action_group_add_actions (action_group, star_popup_actions,
				      G_N_ELEMENTS (star_popup_actions), window);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	error = NULL;
	gtk_ui_manager_add_ui_from_string (ui, star_popup_ui, strlen(star_popup_ui), &error);
	if (error) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	ret = gtk_ui_manager_get_widget (ui, "/star-popup");

	g_object_set_data (G_OBJECT(ret), "actions", action_group);

        /* Make sure that the accelerators work */
	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (ui));

	g_object_ref (ret);
	g_object_unref (ui);

	return ret;
}


#if 0
static GtkItemFactoryEntry image_popup_menu_items[] = {

	{ "/_Processing",         	NULL,         	NULL, 		0, "<Branch>" },
	{ "/Processing/_Next Frame",  "n",    	switch_frame_cb, SWF_NEXT, "<Item>" },
	{ "/Processing/_Skip Frame",  "k",	switch_frame_cb, SWF_SKIP, "<Item>" },
	{ "/Processing/_Previous Frame",  "j",	switch_frame_cb, SWF_PREV, "<Item>" },
	{ "/Processing/_Reduce Current",  "y",	switch_frame_cb, SWF_RED, "<Item>" },
	{ "/Processing/_Qphot Reduce Current",  "t",	switch_frame_cb, SWF_QPHOT, "<Item>" },
	{ "/Processing/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Processing/_Center Stars",  NULL, photometry_cb, PHOT_CENTER_STARS, "<Item>" },
//	{ "/Processing/_Center Stars and Plot Errors",  NULL, photometry_cb, PHOT_CENTER_PLOT, "<Item>" },
	{ "/Processing/Quick Aperture P_hotometry",  "<shift>p", photometry_cb, PHOT_RUN, "<Item>" },
	{ "/Processing/Photometry to _Multi-Frame",  "<control>p", photometry_cb, PHOT_RUN|PHOT_TO_MBDS, "<Item>" },
	{ "/Processing/Photometry to _File", NULL, photometry_cb, PHOT_RUN|PHOT_TO_FILE, "<Item>" },
	{ "/Processing/Photometry to _AAVSO File", NULL, photometry_cb, PHOT_RUN|PHOT_TO_FILE_AA, "<Item>" },
	{ "/Processing/Photometry to _stdout", NULL, photometry_cb, PHOT_RUN|PHOT_TO_STDOUT, "<Item>" },
	{ "/Processing/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Processing/_CCD Reduction...", 	"l",  	processing_cb, 		1, "<Item>" },
	{ "/Processing/_Multi-frame Reduction...", "<control>m", mband_open_cb, 1, "<Item>" },

	{ "/_Catalogs",         	NULL,         	NULL, 		0, "<Branch>" },
	{ "/Catalogs/Load Field Stars From _GSC Catalog", "g", find_stars_cb,
	  ADD_STARS_GSC, "<Item>"},
	{ "/Catalogs/Load Field Stars From _Tycho2 Catalog", "<control>t", find_stars_cb,
	  ADD_STARS_TYCHO2, "<Item>"},
	{ "/Catalogs/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Catalogs/Load Field Stars From GSC-_2 File", "<control>g", file_popup_cb,
	  FILE_LOAD_GSC2, "<Item>"},
	{ "/Catalogs/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Catalogs/Download GSC-_ACT stars from CDS", NULL, cds_query_cb, QUERY_GSC_ACT, "<Item>"},
	{ "/Catalogs/Download _UCAC-2 stars from CDS", NULL, cds_query_cb, QUERY_UCAC2, "<Item>"},
	{ "/Catalogs/Download G_SC-2 stars from CDS", NULL, cds_query_cb, QUERY_GSC2, "<Item>"},
	{ "/Catalogs/Download USNO-_B stars from CDS", NULL, cds_query_cb, QUERY_USNOB, "<Item>"},

};

#endif

/* these actions are common for the image window menu and popup menu */
/* name, stock id, label, accel, tooltip, callback */
static GtkActionEntry image_actions[] = {
	{ "image-file", NULL, "_File" },

	/* File */
	{ "image-new", NULL, "_New Frame", "<control>N", NULL, G_CALLBACK (new_frame_action) },
	{ "image-open", NULL, "_Open Image...", "<control>O", NULL, G_CALLBACK (file_open_action) },
	{ "image-save", NULL, "_Save Fits As...", "<control>S", NULL, G_CALLBACK (file_save_action) },
	{ "image-export", NULL, "_Export Image" },
	{ "image-export-pnm8", NULL, "_8-bit pnm", NULL, NULL, G_CALLBACK (file_export_pnm8_action) },
	{ "image-export-pnm16", NULL, "_16-bit pnm", NULL, NULL, G_CALLBACK (file_export_pnm16_action) },
	{ "recipe-open", NULL, "Load _Recipe", "R", NULL, G_CALLBACK (file_load_recipy_action) },
	{ "recipe-create", NULL, "_Create Recipe", NULL, NULL, G_CALLBACK (create_recipe_action) },
	{ "image-show-header", NULL, "Show Fits _Header...", "<shift>H", NULL, G_CALLBACK (fits_header_action) },
	{ "options-edit", NULL, "Edit O_ptions", "O", NULL, G_CALLBACK (edit_options_action) },
	{ "camera-scope-control", NULL, "Camera and Telescope...", "<shift>C", NULL, G_CALLBACK (camera_action) },
	{ "guide-control", NULL, "Guiding...", "<shift>T", NULL, G_CALLBACK (open_guide_action) },
	{ "user-quit", NULL, "_Quit", "<control>Q", NULL, G_CALLBACK (user_quit_action) },

	/* Image */
	{ "image-image", NULL, "_Image" },
	{ "image-set-contrast", NULL, "Set _Contrast" },
	{ "image-curves", NULL, "Curves&Histogram...", "C", NULL, G_CALLBACK (histogram_action) },
	{ "image-zoom-in", NULL, "Zoom _In", "equal", NULL, G_CALLBACK (view_zoom_in_action) },
	{ "image-zoom-out", NULL, "Zoom _Out", "minus", NULL, G_CALLBACK (view_zoom_out_action) },
	{ "image-zoom-pixels", NULL, "Actual _Pixels", "bracketright", NULL, G_CALLBACK (view_pixels_action) },
	{ "image-pan-center", NULL, "Pan _Center", "<control>L", NULL, G_CALLBACK (view_pan_center_action) },
	{ "image-pan-cursor", NULL, "_Pan Cursor", "space", NULL, G_CALLBACK (view_pan_cursor_action) },
	{ "image-cuts-auto", NULL, "_Auto Cuts", "0", NULL, G_CALLBACK (cuts_auto_action) },
	{ "image-cuts-minmax", NULL, "_Min-Max Cuts", "9", NULL, G_CALLBACK (cuts_minmax_action) },
	{ "image-cuts-flatter", NULL, "_Flatter", "F", NULL, G_CALLBACK (cuts_flatter_action) },
	{ "image-cuts-sharper", NULL, "S_harper", "H", NULL, G_CALLBACK (cuts_sharper_action) },
	{ "image-cuts-brighter", NULL, "_Brighter", "B", NULL, G_CALLBACK (cuts_brighter_action) },
	{ "image-cuts-darker", NULL, "_Darker", "D", NULL, G_CALLBACK (cuts_darker_action) },
	{ "image-cuts-1", NULL, "_4 sigma", "1", NULL, G_CALLBACK (cuts_contrast_1_action) },
	{ "image-cuts-2", NULL, "5_.6 sigma", "2", NULL, G_CALLBACK (cuts_contrast_2_action) },
	{ "image-cuts-3", NULL, "_8 sigma", "3", NULL, G_CALLBACK (cuts_contrast_3_action) },
	{ "image-cuts-4", NULL, "_11 sigma", "4", NULL, G_CALLBACK (cuts_contrast_4_action) },
	{ "image-cuts-5", NULL, "1_6 sigma", "5", NULL, G_CALLBACK (cuts_contrast_5_action) },
	{ "image-cuts-6", NULL, "22 _sigma", "6", NULL, G_CALLBACK (cuts_contrast_6_action) },
	{ "image-cuts-7", NULL, "45 s_igma", "7", NULL, G_CALLBACK (cuts_contrast_7_action) },
	{ "image-cuts-8", NULL, "90 si_gma", "8", NULL, G_CALLBACK (cuts_contrast_8_action) },

	/* Stars */
	{ "image-stars", NULL, "_Stars" },
	{ "image-stars-add-detect",  NULL, "_Detect Sources",        "S",          NULL, G_CALLBACK (stars_add_detect_action)  },
	{ "image-stars-show-target", NULL, "Show Tar_get",           "T",          NULL, G_CALLBACK (stars_show_target_action) },
	{ "image-stars-add-catalog", NULL, "Add From _Catalog",      "A",          NULL, G_CALLBACK (stars_add_catalog_action) },
	{ "image-stars-synthetic",   NULL, "_Create Synthetic Stars", NULL,        NULL, G_CALLBACK (stars_add_synth_action)   },
	{ "image-stars-edit",        NULL, "_Edit",                  "<control>E", NULL, G_CALLBACK (stars_edit_action)        },
	{ "image-stars-rm-selected", NULL, "Remove Selecte_d",       "<control>D", NULL, G_CALLBACK (stars_rm_selected_action) },
	{ "image-stars-rm-detected", NULL, "Remove Detected _Stars", "<shift>S",   NULL, G_CALLBACK (stars_rm_detected_action) },
	{ "image-stars-rm-user",     NULL, "Remove _User Stars",     "<shift>U",   NULL, G_CALLBACK (stars_rm_user_action)     },
	{ "image-stars-rm-field",    NULL, "Remove _Field Stars",    "<shift>F",   NULL, G_CALLBACK (stars_rm_field_action)    },
	{ "image-stars-rm-cat",      NULL, "Remove Catalo_g Objects","<shift>G",   NULL, G_CALLBACK (stars_rm_cat_action)      },
	{ "image-stars-rm-off",      NULL, "Remove _Off-Frame",      "<shift>O",   NULL, G_CALLBACK (stars_rm_off_action)      },
	{ "image-stars-rm-all",      NULL, "Remove _All",            "<shift>A",   NULL, G_CALLBACK (stars_rm_all_action)      },
	{ "image-stars-rm-pairs-all",NULL, "Remove All Pa_irs",      "<shift>P",   NULL, G_CALLBACK (stars_rm_pairs_all_action)},
	{ "image-stars-rm-pairs-sel",NULL, "Remove Selected _Pairs", NULL,         NULL, G_CALLBACK (stars_rm_pairs_sel_action)},
	{ "image-stars-brighter",    NULL, "_Brighter",              "<control>B", NULL, G_CALLBACK (star_display_brighter_action) },
	{ "image-stars-fainter",     NULL, "_Fainter",               "<control>F", NULL, G_CALLBACK (star_display_fainter_action)  },
	{ "image-stars-redraw",      NULL, "_Redraw",                "<control>R", NULL, G_CALLBACK (star_display_redraw_action)   },
	{ "image-stars-plot-profiles", NULL, "Plo_t Star Profiles",  NULL,         NULL, G_CALLBACK (starp_plot_profile_action)    },

	/* WCS */
	{ "wcs-menu", NULL, "_Wcs" },
	{ "image-wcs-edit", NULL, "_Edit Wcs", "<control>W", NULL, G_CALLBACK (wcs_edit_action) },
	{ "image-wcs-auto", NULL, "_Auto Wcs", "M", NULL, G_CALLBACK (wcs_auto_action) },
	{ "image-wcs-quiet-auto", NULL, "_Quiet Auto Wcs", "<shift>M", NULL, G_CALLBACK (wcs_quiet_auto_action) },
	{ "image-wcs-existing", NULL, "_Match Existing Stars", "W", NULL, G_CALLBACK (wcs_existing_action) },
	{ "image-wcs-auto-pairs", NULL, "Auto _Pairs", "p", NULL, G_CALLBACK (star_pairs_action) },
	{ "image-wcs-fit-pairs", NULL, "Fit _Wcs from Pairs", "<shift>W", NULL, G_CALLBACK (wcs_fit_pairs_action) },
	{ "image-wcs-reload", NULL, "_Reload from Frame", "<shift>R", NULL, G_CALLBACK (wcs_reload_action) },
	{ "image-wcs-validate", NULL, "_Force Validate", "<control>V", NULL, G_CALLBACK (wcs_validate_action) },
	{ "image-wcs-invalidate", NULL, "_Invalidate", "<control>I", NULL, G_CALLBACK (wcs_invalidate_action) },

	/* Catalogs */
	{ "catalogs-menu",         NULL, "_Catalogs" },
	{ "catalogs-add-gsc",      NULL, "Load Field Stars From _GSC Catalog",    "G",          NULL, G_CALLBACK (stars_add_gsc_action) },
	{ "catalogs-add-tycho2",   NULL, "Load Field Stars From _Tycho2 Catalog", "<control>T", NULL, G_CALLBACK (stars_add_tycho2_action) },
	{ "catalogs-add-gsc-file", NULL, "Load Field Stars From GSC-_2 File",     "<control>G", NULL, G_CALLBACK (file_load_gsc2_action) },
	{ "catalogs-add-gsc-act",  NULL, "Download GSC-_ACT stars from CDS",      NULL,         NULL, G_CALLBACK (cds_query_gsc_act_action) },
	{ "catalogs-add-ucac2",    NULL, "Download _UCAC-2 stars from CDS",       NULL,         NULL, G_CALLBACK (cds_query_ucac2_action) },
	{ "catalogs-add-gsc2",     NULL, "Download G_SC-2 stars from CDS",        NULL,         NULL, G_CALLBACK (cds_query_gsc2_action) },
	{ "catalogs-add-usnob",    NULL, "Download USNO-_B stars from CDS",       NULL,         NULL, G_CALLBACK (cds_query_usnob_action) },

	/* Processing */
	{ "processing-menu", NULL, "_Processing" },
	{ "swf-next",   NULL, "_Next Frame",           "N", NULL, G_CALLBACK (switch_frame_next_action) },
	{ "swf-skip",   NULL, "_Skip Frame",           "K", NULL, G_CALLBACK (switch_frame_skip_action) },
	{ "swf-prev",   NULL, "_Previous Frame",       "J", NULL, G_CALLBACK (switch_frame_prev_action) },
	{ "swf-reduce", NULL, "_Reduce Current",       "Y", NULL, G_CALLBACK (switch_frame_reduce_action) },
	{ "swf-qphot",  NULL, "_Qphot Reduce Current", "T", NULL, G_CALLBACK (switch_frame_qphot_action) },
	{ "phot-center-stars",  NULL, "_Center Stars",              NULL,         NULL, G_CALLBACK (phot_center_stars_action) },
	{ "phot-quick",         NULL, "Quick Aperture P_hotometry", "<shift>P",   NULL, G_CALLBACK (phot_quick_action) },
	{ "phot-multi-frame",   NULL, "Photometry to _Multi-Frame", "<control>P", NULL, G_CALLBACK (phot_multi_frame_action) },
	{ "phot-to-file",       NULL, "Photometry to _File",        NULL,         NULL, G_CALLBACK (phot_to_file_action) },
	{ "phot-to-aavso-file", NULL, "Photometry to _AAVSO File",  NULL,         NULL, G_CALLBACK (phot_to_aavso_action) },
	{ "phot-to-stdout",     NULL, "Photometry to _stdout",      NULL,         NULL, G_CALLBACK (phot_to_stdout_action) },
	{ "ccd-reduction",         NULL, "_CCD Reduction...",         "L",          NULL, G_CALLBACK (processing_action) },
	{ "multi-frame-reduction", NULL, "_Multi-frame Reduction...", "<control>M", NULL, G_CALLBACK (mband_open_action) },


	/* Help */
	{ "help-menu", NULL, "_Help" },
	{ "help-bindings", NULL, "_GUI help",                   "F1", NULL, G_CALLBACK (help_bindings_action) },
	{ "help-usage",    NULL, "Show _Command Line Options",  NULL, NULL, G_CALLBACK (help_usage_action) },
	{ "help-obscript", NULL, "Show _Obscript Commands",     NULL, NULL, G_CALLBACK (help_obscript_action) },
	{ "help-repconv",  NULL, "Show _Report Converter Help", NULL, NULL, G_CALLBACK (help_repconv_action) },
	{ "help-about",    NULL, "_About",                      NULL, NULL, G_CALLBACK (about_cx_action) },
};

static char *image_common_ui =
	"<menu name='file' action='image-file'>"
	"  <menuitem name='New Frame'  action='image-new'/>"
	"  <menuitem name='Open Image' action='image-open'/>"
	"  <menuitem name='Save Image' action='image-save'/>"
	"  <menu name='export' action='image-export'>"
	"	<menuitem name='8-bit pnm'  action='image-export-pnm8'/>"
	"	<menuitem name='16-bit pnm' action='image-export-pnm16'/>"
	"  </menu>"
	"  <separator name='separator1'/>"
	"  <menuitem name='Load Recipe'   action='recipe-open'/>"
	"  <menuitem name='Create Recipe' action='recipe-create'/>"
	"  <separator name='separator2'/>"
	"  <menuitem name='Show FITS Header' action='image-show-header'/>"
	"  <menuitem name='Edit Options' action='options-edit'/>"
	"  <menuitem name='Camera/Scope' action='camera-scope-control'/>"
	"  <menuitem name='Guiding'      action='guide-control'/>"
	"  <separator name='separator3'/>"
	"  <menuitem name='Quit'         action='user-quit'/>"
	"</menu>"
	"<menu name='image-image' action='image-image'>"
	"  <menuitem name='image-curves' action='image-curves'/>"
	"  <separator name='separator1'/>"
	"  <menuitem name='image-zoom-in' action='image-zoom-in'/>"
	"  <menuitem name='image-zoom-out' action='image-zoom-out'/>"
	"  <menuitem name='image-zoom-pixels' action='image-zoom-pixels'/>"
	"  <separator name='separator2'/>"
	"  <menuitem name='image-pan-center' action='image-pan-center'/>"
	"  <menuitem name='image-pan-cursor' action='image-pan-cursor'/>"
	"  <separator name='separator3'/>"
	"  <menuitem name='image-cuts-auto' action='image-cuts-auto'/>"
	"  <menuitem name='image-cuts-minmax' action='image-cuts-minmax'/>"
	"  <menuitem name='image-cuts-flatter' action='image-cuts-flatter'/>"
	"  <menuitem name='image-cuts-sharper' action='image-cuts-sharper'/>"
	"  <menuitem name='image-cuts-brighter' action='image-cuts-brighter'/>"
	"  <menuitem name='image-cuts-darker' action='image-cuts-darker'/>"
	"  <menu name='image-set-contrast' action='image-set-contrast'>"
	"    <menuitem name='image-cuts-1' action='image-cuts-1'/>"
	"    <menuitem name='image-cuts-2' action='image-cuts-2'/>"
	"    <menuitem name='image-cuts-3' action='image-cuts-3'/>"
	"    <menuitem name='image-cuts-4' action='image-cuts-4'/>"
	"    <menuitem name='image-cuts-5' action='image-cuts-5'/>"
	"    <menuitem name='image-cuts-6' action='image-cuts-6'/>"
	"    <menuitem name='image-cuts-7' action='image-cuts-7'/>"
	"    <menuitem name='image-cuts-8' action='image-cuts-8'/>"
	"    <menuitem name='image-cuts-minmax'    action='image-cuts-minmax'/>"
	"  </menu>"
	"</menu>"
	"<menu name='image-stars' action='image-stars'>"
	"  <menuitem name='image-stars-add-detect' action='image-stars-add-detect'/>"
	"  <menuitem name='image-stars-show-target' action='image-stars-show-target'/>"
	"  <menuitem name='image-stars-add-catalog' action='image-stars-add-catalog'/>"
	"  <separator name='separator1'/>"
	"  <menuitem name='image-stars-synthetic' action='image-stars-synthetic'/>"
	"  <separator name='separator2'/>"
	"  <menuitem name='image-stars-edit' action='image-stars-edit'/>"
	"  <separator name='separator3'/>"
	"  <menuitem name='image-stars-rm-selected' action='image-stars-rm-selected'/>"
	"  <menuitem name='image-stars-rm-detected' action='image-stars-rm-detected'/>"
	"  <menuitem name='image-stars-rm-user' action='image-stars-rm-user'/>"
	"  <menuitem name='image-stars-rm-field' action='image-stars-rm-field'/>"
	"  <menuitem name='image-stars-rm-cat' action='image-stars-rm-cat'/>"
	"  <menuitem name='image-stars-rm-off' action='image-stars-rm-off'/>"
	"  <menuitem name='image-stars-rm-all' action='image-stars-rm-all'/>"
	"  <separator name='separator4'/>"
	"  <menuitem name='image-stars-rm-pairs-all' action='image-stars-rm-pairs-all'/>"
	"  <menuitem name='image-stars-rm-pairs-sel' action='image-stars-rm-pairs-sel'/>"
	"  <separator name='separator5'/>"
	"  <menuitem name='image-stars-brighter' action='image-stars-brighter'/>"
	"  <menuitem name='image-stars-fainter' action='image-stars-fainter'/>"
	"  <menuitem name='image-stars-redraw' action='image-stars-redraw'/>"
	"  <separator name='separator6'/>"
	"  <menuitem name='image-stars-plot-profiles' action='image-stars-plot-profiles'/>"
	"</menu>"
	"<menu name='wcs' action='wcs-menu'>"
	"  <menuitem name='image-wcs-edit' action='image-wcs-edit'/>"
	"  <menuitem name='image-wcs-auto' action='image-wcs-auto'/>"
	"  <menuitem name='image-wcs-quiet-auto' action='image-wcs-quiet-auto'/>"
	"  <menuitem name='image-wcs-existing' action='image-wcs-existing'/>"
	"  <menuitem name='image-wcs-auto-pairs' action='image-wcs-auto-pairs'/>"
	"  <menuitem name='image-wcs-fit-pairs' action='image-wcs-fit-pairs'/>"
	"  <menuitem name='image-wcs-reload' action='image-wcs-reload'/>"
	"  <menuitem name='image-wcs-validate' action='image-wcs-validate'/>"
	"  <menuitem name='image-wcs-invalidate' action='image-wcs-invalidate'/>"
	"</menu>"
	"<menu name='processing' action='processing-menu'>"
	"  <menuitem name='swf-next' action='swf-next'/>"
	"  <menuitem name='swf-skip' action='swf-skip'/>"
	"  <menuitem name='swf-prev' action='swf-prev'/>"
	"  <menuitem name='swf-reduce' action='swf-reduce'/>"
	"  <menuitem name='swf-qphot' action='swf-qphot'/>"
	"  <separator name='separator1'/>"
	"  <menuitem name='phot-center-stars' action='phot-center-stars'/>"
	"  <menuitem name='phot-quick' action='phot-quick'/>"
	"  <menuitem name='phot-multi-frame' action='phot-multi-frame'/>"
	"  <menuitem name='phot-to-file' action='phot-to-file'/>"
	"  <menuitem name='phot-to-aavso-file' action='phot-to-aavso-file'/>"
	"  <menuitem name='phot-to-stdout' action='phot-to-stdout'/>"
	"  <separator name='separator2'/>"
	"  <menuitem name='ccd-reduction' action='ccd-reduction'/>"
	"  <menuitem name='multi-frame-reduction' action='multi-frame-reduction'/>"
	"</menu>"
	"<menu name='catalogs' action='catalogs-menu'>"
	"  <menuitem name='catalogs-add-gsc' action='catalogs-add-gsc'/>"
	"  <menuitem name='catalogs-add-tycho2' action='catalogs-add-tycho2'/>"
	"  <separator name='separator1'/>"
	"  <menuitem name='catalogs-add-gsc-file' action='catalogs-add-gsc-file'/>"
	"  <separator name='separator2'/>"
	"  <menuitem name='catalogs-add-gsc-act' action='catalogs-add-gsc-act'/>"
	"  <menuitem name='catalogs-add-ucac2' action='catalogs-add-ucac2'/>"
	"  <menuitem name='catalogs-add-gsc2' action='catalogs-add-gsc2'/>"
	"  <menuitem name='catalogs-add-usnob' action='catalogs-add-usnob'/>"
	"</menu>"
	"<menu name='help' action='help-menu'>"
	"  <menuitem name='help-bindings' action='help-bindings'/>"
	"  <menuitem name='help-usage' action='help-usage'/>"
	"  <menuitem name='help-obscript' action='help-obscript'/>"
	"  <menuitem name='help-about' action='help-about'/>"
	"</menu>";

GtkWidget *get_image_popup_menu(GtkWidget *window)
{
	GtkWidget *ret;
	GtkUIManager *ui;
	GError *error;
	GtkActionGroup *action_group;
	char *image_ui;

	action_group = gtk_action_group_new ("ImageActions");
	gtk_action_group_add_actions (action_group, image_actions,
				      G_N_ELEMENTS (image_actions), window);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	asprintf(&image_ui, "<popup name='image-popup'>%s</popup>", image_common_ui);

	error = NULL;
	gtk_ui_manager_add_ui_from_string (ui, image_ui, strlen(image_ui), &error);
	if (error) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		free(image_ui);
		return NULL;
	}
	free(image_ui);

	ret = gtk_ui_manager_get_widget (ui, "/image-popup");

        /* Make sure that the accelerators work */
//	gtk_window_add_accel_group (GTK_WINDOW (window),
//				    gtk_ui_manager_get_accel_group (ui));

	g_object_ref (ret);
	g_object_unref (ui);

	return ret;
}

static GtkWidget *get_main_menu_bar(GtkWidget *window)
{
	GtkWidget *ret;
	GtkUIManager *ui;
	GError *error;
	GtkActionGroup *action_group;
	char *image_ui;

	action_group = gtk_action_group_new ("ImageActions");
	gtk_action_group_add_actions (action_group, image_actions,
				      G_N_ELEMENTS (image_actions), window);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	asprintf(&image_ui, "<menubar name='image-menubar'>%s</menubar>", image_common_ui);

	error = NULL;
	gtk_ui_manager_add_ui_from_string (ui, image_ui, strlen(image_ui), &error);
	if (error) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		free(image_ui);
		return NULL;
	}
	free(image_ui);

	ret = gtk_ui_manager_get_widget (ui, "/image-menubar");

        /* Make sure that the accelerators work */
	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (ui));

	g_object_ref (ret);
	g_object_unref (ui);

	return ret;

#if 0
	GtkWidget *ret;
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof (image_popup_menu_items) /
		sizeof (image_popup_menu_items[0]);
	accel_group = gtk_accel_group_new ();

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR,
					     "<main_menu>", accel_group);
	gtk_item_factory_create_items (item_factory, nmenu_items,
				       image_popup_menu_items, window);

  /* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */
	ret = gtk_item_factory_get_widget (item_factory, "<main_menu>");
	return ret;
#endif
}


GtkWidget * create_image_window()
{
	GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *image;
	GtkWidget *alignment;
	GtkWidget *image_popup;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *menubar;
	GtkWidget *star_popup;
	GtkWidget *statuslabel1;
	GtkWidget *statuslabel2;


	image = gtk_drawing_area_new();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_object_ref_sink(window);

	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (destroy_cb), NULL);

	gtk_window_set_title (GTK_WINDOW (window), "gcx");
	gtk_container_set_border_width (GTK_CONTAINER (window), 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	vbox = gtk_vbox_new(0, 0);
	hbox = gtk_hbox_new(0, 0);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
//					GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	gtk_widget_show(alignment);

	statuslabel1 = gtk_label_new ("");
	g_object_ref (statuslabel1);
	gtk_misc_set_padding (GTK_MISC (statuslabel1), 6, 0);
	g_object_set_data_full (G_OBJECT (window), "statuslabel1", statuslabel1,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (statuslabel1);

	statuslabel2 = gtk_label_new ("");
	g_object_ref (statuslabel2);
	gtk_misc_set_padding (GTK_MISC (statuslabel2), 3, 3);
	gtk_misc_set_alignment (GTK_MISC (statuslabel2), 0, 0.5);
	g_object_set_data_full (G_OBJECT (window), "statuslabel2", statuslabel2,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (statuslabel1);

	menubar = get_main_menu_bar(window);
	//gtk_menu_bar_set_shadow_type(GTK_MENU_BAR(menubar), GTK_SHADOW_NONE);

	gtk_widget_show(menubar);
	gtk_box_pack_start(GTK_BOX(hbox), menubar, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), statuslabel1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, 1, 1, 0);
	gtk_box_pack_start(GTK_BOX(vbox), statuslabel2, 0, 0, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), alignment);
	gtk_container_add(GTK_CONTAINER(alignment), image);

	g_signal_connect(G_OBJECT(scrolled_window), "button_press_event",
			 G_CALLBACK(sources_clicked_cb), window);
	g_signal_connect(G_OBJECT(scrolled_window), "button_press_event",
			 G_CALLBACK(image_clicked_cb), window);
	g_signal_connect(G_OBJECT(image), "motion_notify_event",
			 G_CALLBACK(motion_event_cb), window);
	g_signal_connect(G_OBJECT(image), "expose_event",
			 G_CALLBACK(image_expose_cb), window);

	gtk_widget_set_events(image,  GDK_BUTTON_PRESS_MASK
			      | GDK_POINTER_MOTION_MASK
			      | GDK_POINTER_MOTION_HINT_MASK);

	g_object_set_data(G_OBJECT(window), "scrolled_window", scrolled_window);
	g_object_set_data(G_OBJECT(window), "image_alignment", alignment);
	g_object_set_data(G_OBJECT(window), "image", image);

  	gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);

	gtk_widget_show(image);
	gtk_widget_show (scrolled_window);
	gtk_widget_show(vbox);

	image_popup = get_image_popup_menu(window);
	g_object_set_data_full(G_OBJECT(window), "image_popup", image_popup,
			       (GDestroyNotify) g_object_unref);

	star_popup = get_star_popup_menu(window);
	g_object_set_data_full(G_OBJECT(window), "star_popup", star_popup,
			       (GDestroyNotify) g_object_unref);

//	gtk_widget_show_all(window);
	gtk_widget_show(image_popup);

	return window;
}

GtkWidget* create_about_cx (void)
{
	GtkWidget *about_cx;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *frame1;
	GtkWidget *vbox2;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *label5;
	GtkWidget *label6;
	GtkWidget *label7;
	GtkWidget *label12;
	GtkWidget *label13;
	GtkWidget *label14;
	GtkWidget *label15;
	GtkWidget *hseparator1;
	GtkWidget *label8;
	GtkWidget *label9;
	GtkWidget *label10;
	GtkWidget *label11;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;

	about_cx = gtk_dialog_new ();
	g_object_set_data (G_OBJECT (about_cx), "about_cx", about_cx);
	gtk_window_set_title (GTK_WINDOW (about_cx), ("About gcx"));
	GTK_WINDOW (about_cx)->type = GTK_WINDOW_TOPLEVEL;
	gtk_window_set_position (GTK_WINDOW (about_cx), GTK_WIN_POS_CENTER);

	//deprecated gtk_window_set_policy (GTK_WINDOW (about_cx), TRUE, TRUE, FALSE);
	gtk_window_set_resizable (GTK_WINDOW (about_cx), TRUE);

	dialog_vbox1 = GTK_DIALOG (about_cx)->vbox;
	g_object_set_data (G_OBJECT (about_cx), "dialog_vbox1", dialog_vbox1);
	gtk_widget_show (dialog_vbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	g_object_ref (vbox1);
	g_object_set_data_full (G_OBJECT (about_cx), "vbox1", vbox1,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

	frame1 = gtk_frame_new ("gcx version " VERSION);
	g_object_ref (frame1);
	g_object_set_data_full (G_OBJECT (about_cx), "frame1", frame1,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame1), 5);

	vbox2 = gtk_vbox_new (TRUE, 0);
	g_object_ref (vbox2);
	g_object_set_data_full (G_OBJECT (about_cx), "vbox2", vbox2,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame1), vbox2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);

	label3 = gtk_label_new (("A program that controls astronomical"));
	g_object_ref (label3);
	g_object_set_data_full (G_OBJECT (about_cx), "label3", label3,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (vbox2), label3, FALSE, FALSE, 0);

	label4 = gtk_label_new (("CCD cameras and does things on images."));
	g_object_ref (label4);
	g_object_set_data_full (G_OBJECT (about_cx), "label4", label4,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (vbox2), label4, FALSE, FALSE, 0);

	label5 = gtk_label_new (("(c) 2002-2009 Radu Corlan"));
	g_object_ref (label5);
	g_object_set_data_full (G_OBJECT (about_cx), "label5", label5,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label5);
	gtk_box_pack_start (GTK_BOX (vbox2), label5, FALSE, FALSE, 0);

	label6 = gtk_label_new (("Tycho2 routines (c) 2002, 2003 Alexandru Dan Corlan"));
	g_object_ref (label6);
	g_object_set_data_full (G_OBJECT (about_cx), "label6", label6,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label6);
	gtk_box_pack_start (GTK_BOX (vbox2), label6, FALSE, FALSE, 0);

	label12 = gtk_label_new (("WCS conversion routines from Classic AIPS"));
	g_object_ref (label12);
	g_object_set_data_full (G_OBJECT (about_cx), "label12", label12,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label12);
	gtk_box_pack_start (GTK_BOX (vbox2), label12, FALSE, FALSE, 0);

	label7 = gtk_label_new (("Sidereal time routines from libnova (c) 2000 Liam Girdwood"));
	g_object_ref (label7);
	g_object_set_data_full (G_OBJECT (about_cx), "label7", label7,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label7);
	gtk_box_pack_start (GTK_BOX (vbox2), label7, FALSE, FALSE, 0);

	label13 = gtk_label_new (("GUI improvements (c) 2005 Pertti Paakkonen"));
	g_object_ref (label13);
	g_object_set_data_full (G_OBJECT (about_cx), "label13", label13,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label13);
	gtk_box_pack_start (GTK_BOX (vbox2), label13, FALSE, FALSE, 0);

	label14 = gtk_label_new (("gtk2 port and DSLR raw file support (c) 2009 Matei Conovici"));
	g_object_ref (label14);
	g_object_set_data_full (G_OBJECT (about_cx), "label14", label14,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label14);
	gtk_box_pack_start (GTK_BOX (vbox2), label14, FALSE, FALSE, 0);

	label15 = gtk_label_new (("Full-color enhancements (c) 2009 Geoffrey Hausheer"));
	g_object_ref (label15);
	g_object_set_data_full (G_OBJECT (about_cx), "label15", label15,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label15);
	gtk_box_pack_start (GTK_BOX (vbox2), label15, FALSE, FALSE, 0);

	hseparator1 = gtk_hseparator_new ();
	g_object_ref (hseparator1);
	g_object_set_data_full (G_OBJECT (about_cx), "hseparator1", hseparator1,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox2), hseparator1, FALSE, FALSE, 0);

	label8 = gtk_label_new (("gcx comes with ABSOLUTELY NO WARRANTY"));
	g_object_ref (label8);
	g_object_set_data_full (G_OBJECT (about_cx), "label8", label8,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label8);
	gtk_box_pack_start (GTK_BOX (vbox2), label8, FALSE, FALSE, 0);

	label9 = gtk_label_new (("This is free software, distributed under"));
	g_object_ref (label9);
	g_object_set_data_full (G_OBJECT (about_cx), "label9", label9,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label9);
	gtk_box_pack_start (GTK_BOX (vbox2), label9, FALSE, FALSE, 0);

	label10 = gtk_label_new (("the GNU General Public License v2 or later;"));
	g_object_ref (label10);
	g_object_set_data_full (G_OBJECT (about_cx), "label10", label10,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label10);
	gtk_box_pack_start (GTK_BOX (vbox2), label10, FALSE, FALSE, 0);

	label11 = gtk_label_new (("See file 'COPYING' for details."));
	g_object_ref (label11);
	g_object_set_data_full (G_OBJECT (about_cx), "label11", label11,
				  (GDestroyNotify) g_object_unref);
	gtk_widget_show (label11);
	gtk_box_pack_start (GTK_BOX (vbox2), label11, FALSE, FALSE, 0);

	dialog_action_area1 = GTK_DIALOG (about_cx)->action_area;
	g_object_set_data (G_OBJECT (about_cx), "dialog_action_area1", dialog_action_area1);
	gtk_widget_show (dialog_action_area1);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);

	button1 = gtk_button_new_with_label("Close");
	g_object_ref (button1);
	g_object_set_data_full (G_OBJECT (about_cx), "button1", button1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (dialog_action_area1), button1, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (button1), "clicked",
			    G_CALLBACK (close_about_cx),
			    about_cx);

	return about_cx;
}

