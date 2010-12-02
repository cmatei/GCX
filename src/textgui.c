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

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "interface.h"
#include "params.h"
#include "helpmsg.h"

static void update_fits_header_dialog(GtkWidget *dialog, struct ccd_frame *fr);
static void update_help_text(GtkWidget *dialog, char * page);


static void close_text_window( GtkWidget *widget, gpointer data )
{
	g_return_if_fail(data != NULL);
	gtk_object_set_data(GTK_OBJECT(data), "text_window", NULL);
}

static void close_fits_dialog( GtkWidget *widget, gpointer data )
{
	GtkWidget *im_window;
	im_window = gtk_object_get_data(GTK_OBJECT(data), "im_window");
	g_return_if_fail(im_window != NULL);
	gtk_object_set_data(GTK_OBJECT(im_window), "text_window", NULL);
}

static void set_named_callback(void *dialog, char *name, char *callback, void *func)
{
	GtkObject *wid;
	wid = gtk_object_get_data(GTK_OBJECT(dialog), name);
	if (wid == NULL) {
		err_printf("cannot find object : %s\n", name);
	}
	g_return_if_fail(wid != NULL);
	gtk_signal_connect (GTK_OBJECT (wid), callback,
			    GTK_SIGNAL_FUNC (func), dialog);
}


/* show the current frame's fits header in a text window */
void fits_header_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	GtkWidget *dialog;
	struct image_channel *i_chan;
	char title[256];

	i_chan = gtk_object_get_data(GTK_OBJECT(window), "i_channel");
	if (i_chan == NULL || i_chan->fr == NULL) {
		err_printf_sb2(window, "No frame loaded");
		error_beep();
		return;
	}

	dialog = gtk_object_get_data(GTK_OBJECT(window), "text_window");
	if (dialog == NULL) {
		dialog = create_show_text();
		gtk_object_set_data(GTK_OBJECT(dialog), "im_window",
					 window);
		gtk_object_set_data_full(GTK_OBJECT(window), "text_window",
					 dialog, (GtkDestroyNotify)(gtk_widget_destroy));
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
				    GTK_SIGNAL_FUNC (close_text_window), window);
		set_named_callback (GTK_OBJECT (dialog), "close_button", "clicked",
				    GTK_SIGNAL_FUNC (close_fits_dialog));
		gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);
		update_fits_header_dialog(dialog, i_chan->fr);
		gtk_widget_show(dialog);
	} else {
		update_fits_header_dialog(dialog, i_chan->fr);
		gdk_window_raise(dialog->window);
	}
	snprintf(title, 255, "%s header", i_chan->fr->name);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
}

/* show a help page in a text window */
void help_page_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	GtkWidget *dialog;
	char *page = NULL;

	switch(action) {
	case HELP_BINDINGS:
		page = help_bindings_page;
		break;
	case HELP_USAGE:
		page = help_usage_page;
		break;
	case HELP_OBSCRIPT:
		page = help_obscmd_page;
		break;
	case HELP_REPCONV:
		page = help_rep_conv_page;
		break;
	default:
		err_printf("cannot find help page %d\n", action);
		return;
	}

	dialog = gtk_object_get_data(GTK_OBJECT(window), "text_window");
	if (dialog == NULL) {
		dialog = create_show_text();
		gtk_object_set_data(GTK_OBJECT(dialog), "im_window",
					 window);
		gtk_object_set_data_full(GTK_OBJECT(window), "text_window",
					 dialog, (GtkDestroyNotify)(gtk_widget_destroy));
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
				    GTK_SIGNAL_FUNC (close_text_window), window);
		set_named_callback (GTK_OBJECT (dialog), "close_button", "clicked",
				    GTK_SIGNAL_FUNC (close_fits_dialog));
		gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);
		update_help_text(dialog, page);
		gtk_widget_show(dialog);
	} else {
		update_help_text(dialog, page);
		gdk_window_raise(dialog->window);
	}
	gtk_window_set_title(GTK_WINDOW(dialog), "Help");
}

static void update_help_text(GtkWidget *dialog, char *page)
{
	GtkWidget *text;
	GtkTextBuffer *buffer;

	text = gtk_object_get_data(GTK_OBJECT(dialog), "text1");
	g_return_if_fail(text != NULL);
	g_return_if_fail(page != NULL);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(buffer, page, -1);
}


static void update_fits_header_dialog(GtkWidget *dialog, struct ccd_frame *fr)
{
	GtkWidget *text;
	GtkTextBuffer *buffer;
	char line[82];
	int i;

	text = gtk_object_get_data(GTK_OBJECT(dialog), "text1");
	g_return_if_fail(text != NULL);

        /* kill old text */
	/* FIXME: this is rather crude */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(buffer, "", -1);

	line[80] = '\n';
	line[81] = 0;
	for (i=0; i< fr->nvar; i++) {
		memcpy(line, fr->var[i], 80);
		gtk_text_buffer_insert_at_cursor(buffer, line, -1);
	}
}



