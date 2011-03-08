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


static void about_cx(gpointer data)
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

static void user_quit_action(gpointer data, guint action, GtkWidget *menu_item)
{
	d3_printf("user quit\n");
	gtk_widget_destroy(GTK_WIDGET(data));
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

void star_pairs_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *window = data;
	switch(action) {
	case PAIRS_AUTO:
		window_auto_pairs(window);
		break;
	default:
		err_printf("unknown action %d in star_pairs_cb\n", action);
	}
}

static void new_frame_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	struct ccd_frame *fr;
	fr = new_frame(P_INT(FILE_NEW_WIDTH), P_INT(FILE_NEW_HEIGHT));
	frame_to_channel(fr, data, "i_channel");
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

void stars_rm_detected_action (GtkAction *action, gpointer window)
{
	star_rm_cb (window, STAR_RM_FR, NULL);
}

static GtkItemFactoryEntry star_popup_menu_items[] = {
	{"/_Edit Star", NULL, star_popup_cb, STARP_EDIT_AP, "<Item>"},
//	{"/Make _Std Star", NULL, star_popup_cb, STARP_MAKE_STD, "<Item>"},
//	{"/Make C_at Star", NULL, star_popup_cb, STARP_MAKE_CAT, "<Item>"},
	{"/_Remove Star", NULL, star_popup_cb, STARP_UNMARK_STAR, "<Item>"},
	{"/_Create Pair", NULL, star_popup_cb, STARP_PAIR, "<Item>"},
	{"/Remo_ve Pair", NULL, star_popup_cb, STARP_PAIR_RM, "<Item>"},
	{"/Move _Star", NULL, star_popup_cb, STARP_MOVE, "<Item>"},
	{"/Plot _Profile", NULL, star_popup_cb, STARP_PROFILE, "<Item>"},
	{"/_Measure", NULL, star_popup_cb, STARP_MEASURE, "<Item>"},
	{"/Sky _Histogram", NULL, star_popup_cb, STARP_SKYHIST, "<Item>"},
	{"/_Fit Psf", NULL, star_popup_cb, STARP_FIT_PSF, "<Item>"},
};


static GtkItemFactoryEntry image_popup_menu_items[] = {
	{ "/_File",		NULL,         	NULL,  		0, "<Branch>" },
/*  	{ "/File/tear",  	NULL,         	NULL,  		0, "<Tearoff>" }, */
	{ "/File/_New Frame",	"<control>n", 	new_frame_cb, 	0, "<Item>" },
	{ "/File/_Open Image...",	"<control>o", 	file_popup_cb, 	FILE_OPEN, "<Item>" },
	{ "/File/_Save Fits As...", "<control>s", file_popup_cb, FILE_SAVE_AS, "<Item>" },
	{ "/File/_Export Image",	NULL, 	NULL, 	0, "<Branch>" },
	{ "/File/_Export Image/_8-bit pnm", NULL, file_popup_cb, FILE_EXPORT_PNM8, "<Item>" },
	{ "/File/_Export Image/_16-bit pnm", NULL, file_popup_cb, FILE_EXPORT_PNM16, "<Item>" },
	{ "/File/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/File/Load _Recipe",	"r", file_popup_cb, FILE_OPEN_RCP, "<Item>" },
	{ "/File/_Create Recipe",	NULL, create_recipe_cb, 0, "<Item>" },
//	{ "/File/_Close",	"<control>c", 	file_popup_cb, 	FILE_CLOSE, "<Item>" },

	{ "/File/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/File/Show Fits _Header...",  "<shift>h", 	fits_header_cb, 1, "<Item>" },
	{ "/File/Edit O_ptions...",	"o", 	edit_options_cb, 1, "<Item>" },
	{ "/File/Camera and Telescope...",	"<shift>c", 	camera_cb, 1, "<Item>" },
	{ "/File/Guiding...",	"<shift>t", 	open_guide_cb, 1, "<Item>" },
	{ "/File/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/File/_Quit",	"<control>Q", 	user_quit_action, 0, "<Item>" },


	{ "/_Image",      	NULL,   	NULL, 		0, "<Branch>" },
//	{ "/Image/_Show Stats",  	"s",    	stats_cb,       1, "<Item>" },
	{ "/Image/sep",		NULL,   	NULL,  		0, "<Separator>" },
	{ "/Image/Curves&Histogram...",  "c", 	histogram_cb, 1, "<Item>" },
	{ "/Image/sep",		NULL,   	NULL,  		0, "<Separator>" },
	{ "/Image/Zoom _In",  	"equal",    	view_option_cb, VIEW_ZOOM_IN, "<Item>" },
	{ "/Image/Zoom _Out",  	"minus",	view_option_cb, VIEW_ZOOM_OUT, "<Item>" },
//	{ "/Image/Zoom A_ll", 	"bracketleft", view_option_cb, VIEW_ZOOM_FIT, "<Item>" },
	{ "/Image/Actual _Pixels", "bracketright", view_option_cb, VIEW_PIXELS, "<Item>" },
	{ "/Image/sep1",		NULL,   	NULL,  		0, "<Separator>" },
	{ "/Image/Pan _Center",	"<control>l",	view_option_cb, VIEW_PAN_CENTER, "<Item>" },
	{ "/Image/_Pan Cursor",	"space", view_option_cb, VIEW_PAN_CURSOR, "<Item>" },
	{ "/Image/sep2",		NULL,   	NULL,  		0, "<Separator>" },
	{ "/Image/_Auto Cuts", 	"0",		cuts_option_cb, CUTS_AUTO, "<Item>" },
	{ "/Image/_Min-Max Cuts", "9",		cuts_option_cb, CUTS_MINMAX, "<Item>" },
	{ "/Image/_Flatter", 	"f",		cuts_option_cb, CUTS_FLATTER, "<Item>" },
	{ "/Image/S_harper", 	"h",		cuts_option_cb, CUTS_SHARPER, "<Item>" },
	{ "/Image/_Brighter", 	"b",		cuts_option_cb, CUTS_BRIGHTER, "<Item>" },
	{ "/Image/_Darker", 	"d",		cuts_option_cb, CUTS_DARKER, "<Item>" },
	{ "/Image/_Invert", 	"i",		cuts_option_cb, CUTS_INVERT, "<Item>" },

	{ "/Image/Set _Contrast", NULL, 		NULL, 		0, "<Branch>" },
	{ "/Image/Set Contrast/_4 sigma", "1", 	cuts_option_cb, CUTS_CONTRAST|1, "<Item>" },
	{ "/Image/Set Contrast/5_.6 sigma", "2", cuts_option_cb, CUTS_CONTRAST|2, "<Item>" },
	{ "/Image/Set Contrast/_8 sigma", "3", 	cuts_option_cb, CUTS_CONTRAST|3, "<Item>" },
	{ "/Image/Set Contrast/_11 sigma", "4", cuts_option_cb, CUTS_CONTRAST|4, "<Item>" },
	{ "/Image/Set Contrast/1_6 sigma", "5", cuts_option_cb, CUTS_CONTRAST|5, "<Item>" },
	{ "/Image/Set Contrast/22 _sigma", "6", cuts_option_cb, CUTS_CONTRAST|6, "<Item>" },
	{ "/Image/Set Contrast/45 s_igma", "7", cuts_option_cb, CUTS_CONTRAST|7, "<Item>" },
	{ "/Image/Set Contrast/90 si_gma", "8", cuts_option_cb, CUTS_CONTRAST|8, "<Item>" },
	{ "/Image/Set Contrast/_Min-Max", NULL, 	cuts_option_cb, CUTS_MINMAX, "<Item>" },


	{ "/_Stars",      	NULL,   	NULL, 		0, "<Branch>" },
	{ "/Stars/_Detect Sources",  "s", find_stars_cb, ADD_STARS_DETECT, "<Item>" },
	{ "/Stars/Show Tar_get", "t", find_stars_cb, ADD_STARS_OBJECT, "<Item>"},
	{ "/Stars/Add From _Catalog", "a", find_stars_cb, ADD_FROM_CATALOG, "<Item>"},
	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Stars/_Create Synthetic Stars",  NULL, add_synth_stars_cb, 0, "<Item>" },
//	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
//	{ "/Stars/_Mark Stars", NULL, selection_mode_cb, SEL_ACTION_MARK_STARS, "<Item>" },
//	{ "/Stars/Reset Sel Mode", "Escape", selection_mode_cb, SEL_ACTION_NORMAL, "<Item>" },
//	{ "/Stars/Show _Sources",  NULL,	NULL,  		0, "<CheckItem>" },
	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Stars/_Edit",  "<control>e", star_edit_cb, STAR_EDIT, "<Item>" },
	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Stars/Remove Selecte_d",  "<control>d", star_rm_cb, STAR_RM_SEL, "<Item>" },
	{ "/Stars/Remove Detected _Stars", "<shift>s", star_rm_cb, STAR_RM_FR, "<Item>" },
	{ "/Stars/Remove _User Stars",  "<shift>u", star_rm_cb, STAR_RM_USER, "<Item>" },
	{ "/Stars/Remove _Field Stars",  "<shift>f", star_rm_cb, STAR_RM_FIELD, "<Item>" },
	{ "/Stars/Remove Catalo_g Objects",  "<shift>g", star_rm_cb, STAR_RM_CAT, "<Item>" },
	{ "/Stars/Remove _Off-Frame",  "<shift>o", star_rm_cb, STAR_RM_OFF, "<Item>" },
	{ "/Stars/Remove _All", "<shift>a",	star_rm_cb, STAR_RM_ALL, "<Item>" },
	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Stars/Remove All Pa_irs", "<shift>p", star_rm_cb, STAR_RM_PAIRS_ALL, "<Item>" },
	{ "/Stars/Remove Selected _Pairs",  NULL, star_rm_cb, STAR_RM_PAIRS_SEL, "<Item>" },
	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Stars/_Brighter",  "<control>b", star_display_cb, STAR_BRIGHTER, "<Item>" },
	{ "/Stars/_Fainter",  "<control>f", star_display_cb, STAR_FAINTER, "<Item>" },
	{ "/Stars/_Redraw",  "<control>r", star_display_cb, STAR_REDRAW, "<Item>" },
	{ "/Stars/sep",		NULL,         	NULL,  		0, "<Separator>" },
	{ "/Stars/Plo_t Star Profiles", NULL, star_popup_cb, STARP_PROFILE, "<Item>"},

//	{ "/Stars/D_etection Options...",  NULL,	NULL,  	0, "<Item>" },
//	{ "/Stars/S_tar Display Options...",  NULL,	NULL,  	0, "<Item>" },
//	{ "/Stars/_Catalogs...",  NULL,	NULL,  	0, "<Item>" },

	{ "/_Wcs",      	NULL,   	NULL, 		0, "<Branch>" },
	{ "/Wcs/_Edit Wcs",  "<control>w", wcsedit_cb, 0, "<Item>" },
	{ "/Wcs/_Auto Wcs",  "m", wcs_cb, WCS_AUTO, "<Item>" },
	{ "/Wcs/_Quiet Auto Wcs",  "<shift>m", wcs_cb, WCS_QUIET_AUTO, "<Item>" },
	{ "/Wcs/_Match Existing Stars",  "w", wcs_cb, WCS_EXISTING, "<Item>" },
	{ "/Wcs/Auto _Pairs", "p",	star_pairs_cb, PAIRS_AUTO, "<Item>" },
	{ "/Wcs/Fit _Wcs from Pairs",  "<shift>w", wcs_cb, WCS_FIT, "<Item>" },
	{ "/Wcs/_Reload from Frame",  "<shift>r", wcs_cb, WCS_RELOAD, "<Item>" },
	{ "/Wcs/_Force Validate",  "<control>v", wcs_cb, WCS_FORCE_VALID, "<Item>" },
	{ "/Wcs/_Invalidate",  "<control>i", wcs_cb, WCS_RESET, "<Item>" },

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

	{ "/_Help",         	NULL,         	NULL, 		0, "<Branch>" },
	{ "/Help/_GUI help", "F1",  	help_page_cb,  HELP_BINDINGS, "<Item>" },
	{ "/Help/Show _Command Line Options", NULL,  	help_page_cb,  HELP_USAGE, "<Item>" },
	{ "/Help/Show _Obscript Commands", NULL,  	help_page_cb,  HELP_OBSCRIPT, "<Item>" },
	{ "/Help/Show _Report Converter Help", NULL,  	help_page_cb,  HELP_REPCONV, "<Item>" },
	{ "/Help/_About",   	NULL, 		about_cx, 0, "<Item>" },
};


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
static void pop_free(gpointer p)
{
	d3_printf("pop_free\n");
}
*/

/*
 * mouse button event callback. It is normally called after the callback in
 * sourcesdraw has decided not to act on the click
 */
static gboolean image_clicked_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	GtkMenu *menu;
	GSList *found;
	GtkItemFactory *star_if;

//	printf("button press : %f %f state %08x button %08x \n",
//	       event->x, event->y, event->state, event->button);
	if (event->button == 3) {
		show_region_stats(data, event->x, event->y);
		if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
		    return FALSE;
		found = stars_under_click(GTK_WIDGET(data), event);
		star_if = g_object_get_data(G_OBJECT(data), "star_popup_if");
		menu = g_object_get_data(G_OBJECT(data), "image_popup");
		if (found != NULL && star_if != NULL) {
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

GtkItemFactory *get_star_popup_menu(gpointer data)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof (star_popup_menu_items) /
		sizeof (star_popup_menu_items[0]);
	accel_group = gtk_accel_group_new ();

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU,
					     "<star_popup>", accel_group);
	gtk_item_factory_create_items (item_factory, nmenu_items,
				       star_popup_menu_items, data);

	return item_factory;
}



GtkWidget *get_image_popup_menu(GtkWidget *window)
{
	GtkWidget *ret;
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof (image_popup_menu_items) /
		sizeof (image_popup_menu_items[0]);
	accel_group = gtk_accel_group_new ();

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU,
					     "<image_popup>", accel_group);
	g_object_set_data_full(G_OBJECT(window), "image_popup_if", item_factory,
				 (GDestroyNotify) g_object_unref);
	gtk_item_factory_create_items (item_factory, nmenu_items,
				       image_popup_menu_items, window);

  /* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */
	ret = gtk_item_factory_get_widget (item_factory, "<image_popup>");
	return ret;
}

static GtkWidget *get_main_menu_bar(GtkWidget *window)
{
	GtkWidget *ret;
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof (image_popup_menu_items) /
		sizeof (image_popup_menu_items[0]);
	accel_group = gtk_accel_group_new ();

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR,
					     "<main_menu>", accel_group);
	g_object_set_data_full(G_OBJECT(window), "main_menu_if", item_factory,
				 (GDestroyNotify) g_object_unref);
	gtk_item_factory_create_items (item_factory, nmenu_items,
				       image_popup_menu_items, window);

  /* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */
	ret = gtk_item_factory_get_widget (item_factory, "<main_menu>");
	return ret;
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
	GtkItemFactory *star_popup_factory;
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
	star_popup_factory = get_star_popup_menu(window);
	g_object_set_data_full(G_OBJECT(window), "star_popup_if", star_popup_factory,
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

