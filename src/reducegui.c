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

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#else
#include "libgen.h"
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "interface.h"
#include "params.h"
#include "misc.h"
#include "reduce.h"
#include "filegui.h"
#include "sourcesdraw.h"
#include "multiband.h"
#include "wcs.h"

static int progress_pr(char *msg, void *dialog);
static int log_msg(char *msg, void *dialog);
static void imf_display_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_prev_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_next_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_skip_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_rm_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_unskip_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_selall_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_add_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void imf_reload_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void select_cb(GtkList *list, GtkWidget *wid, gpointer dialog);
//static void list_button_cb(GtkWidget *wid, GdkEventButton *event, gpointer dialog);
static void imf_update_status_label(GtkWidget *label, struct image_file *imf);
static void imf_red_browse_cb(GtkWidget *wid, gpointer dialog);
static void ccdred_run_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void ccdred_one_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void ccdred_qphotone_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void show_align_cb(gpointer dialog, guint action, GtkWidget *menu_item);
static void update_selected_status_label(gpointer dialog);
static void imf_red_activate_cb(GtkWidget *wid, gpointer dialog);
static void set_processing_dialog_ccdr(GtkWidget *dialog, struct ccd_reduce *ccdr);
static void mframe_cb(gpointer dialog, guint action, GtkWidget *menu_item);


static gboolean close_processing_window( GtkWidget *widget, gpointer data )
{
	g_return_val_if_fail(data != NULL, TRUE);
//	gtk_object_set_data(GTK_OBJECT(data), "processing", NULL);
	gtk_widget_hide(widget);
	return TRUE;
}

static void mframe_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	gpointer im_window;
	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);

	mband_open_cb(im_window, 1, NULL);
}



static GtkItemFactoryEntry reduce_menu_items[] = {
	{ "/_File",		NULL,         	NULL,  		0, "<Branch>" },
	{ "/_File/_Add Files", "<control>O", imf_add_cb, 0, "<Item>" },
	{ "/_File/Remove Selecte_d Files", "<control>D", imf_rm_cb, 0, "<Item>" },
	{ "/_File/Reload Selected Files", "<control>R", imf_reload_cb, 0, "<Item>" },
	{ "/File/Sep", NULL, NULL,	0, "<Separator>" },
	{ "/_File/_Display Frame", "D", imf_display_cb, 0, "<Item>" },
	{ "/_File/_Next Frame", "N", imf_next_cb, 0, "<Item>" },
	{ "/_File/_Previous Frame", "J", imf_prev_cb, 0, "<Item>" },
	{ "/File/Sep", NULL, NULL,	0, "<Separator>" },
	{ "/_File/Sk_ip Selected Frames", "K", imf_skip_cb, 0, "<Item>" },
	{ "/_File/_Unskip Selected Frames", "U", imf_unskip_cb, 0, "<Item>" },


	{ "/_Edit", NULL, NULL,	0, "<Branch>" },
	{ "/_Edit/Select _All", "<control>A", imf_selall_cb, 0, "<Item>" },
//	{ "/_Edit/_Unselect All", "<control>U", NULL, 0, "<Item>" },

	{ "/_Reduce", NULL, NULL,	0, "<Branch>" },
	{ "/_Reduce/Reduce All", "<shift>R", ccdred_run_cb, 0, "<Item>" },
	{ "/_Reduce/Reduce One Frame", "y", ccdred_one_cb, 0, "<Item>" },
	{ "/_Reduce/Qphot One Frame", "t", ccdred_qphotone_cb, 0, "<Item>" },
	{ "/Reduce/Sep", NULL, NULL,	0, "<Separator>" },
	{ "/_Reduce/Show Alignment Stars", "A", show_align_cb, 0, "<Item>" },
	{ "/Reduce/Sep", NULL, NULL,	0, "<Separator>" },
	{ "/Reduce/_Multi-frame Photometry...", "<control>m", mframe_cb, 1, "<Item>" },

};


/* create the menu bar */
static GtkWidget *get_main_menu_bar(GtkWidget *window)
{
	GtkWidget *ret;
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof (reduce_menu_items) / 
		sizeof (reduce_menu_items[0]);
	accel_group = gtk_accel_group_new ();

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, 
					     "<main_menu>", accel_group);
	gtk_object_set_data_full(GTK_OBJECT(window), "main_menu_if", item_factory,
				 (GtkDestroyNotify) gtk_object_unref);
	gtk_item_factory_create_items (item_factory, nmenu_items, 
				       reduce_menu_items, window);

  /* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Finally, return the actual menu bar created by the item factory. */ 
	ret = gtk_item_factory_get_widget (item_factory, "<main_menu>");
	return ret;
}




/* create an image processing dialog and set the callbacks, but don't 
 * show it */
static GtkWidget *make_image_processing(gpointer window)
{
	GtkWidget *dialog;
	GtkWidget *menubar, *top_hb;

	dialog = create_image_processing();
	gtk_object_set_data(GTK_OBJECT(dialog), "im_window", window);
	gtk_object_set_data_full(GTK_OBJECT(window), "processing",
				 dialog, (GtkDestroyNotify)(gtk_widget_destroy));
	gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			    GTK_SIGNAL_FUNC (close_processing_window), window);
/*
	set_named_callback (GTK_OBJECT (dialog), "imf_display_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_display_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_next_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_next_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_skip_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_skip_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_unskip_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_unskip_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_rm_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_rm_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_select_all", "clicked",
			    GTK_SIGNAL_FUNC (imf_selall_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_add_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_add_cb));
	set_named_callback (GTK_OBJECT (dialog), "imf_reload_button", "clicked",
			    GTK_SIGNAL_FUNC (imf_reload_cb));
*/
	set_named_callback (GTK_OBJECT (dialog), "image_file_list", "select-child",
			    GTK_SIGNAL_FUNC (select_cb));
	set_named_callback (GTK_OBJECT (dialog), "bias_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));
	set_named_callback (GTK_OBJECT (dialog), "dark_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));
	set_named_callback (GTK_OBJECT (dialog), "flat_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));
	set_named_callback (GTK_OBJECT (dialog), "badpix_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));	
	set_named_callback (GTK_OBJECT (dialog), "align_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));
	set_named_callback (GTK_OBJECT (dialog), "output_file_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));
	set_named_callback (GTK_OBJECT (dialog), "recipe_browse", "clicked",
			    GTK_SIGNAL_FUNC (imf_red_browse_cb));
/*
	set_named_callback (GTK_OBJECT (dialog), "run_button", "clicked",
			    GTK_SIGNAL_FUNC (ccdred_run_cb));
	set_named_callback (GTK_OBJECT (dialog), "show_align_button", "clicked",
			    GTK_SIGNAL_FUNC (show_align_cb));
*/
	set_named_callback (GTK_OBJECT (dialog), "bias_entry", "activate",
			    GTK_SIGNAL_FUNC (imf_red_activate_cb));
	set_named_callback (GTK_OBJECT (dialog), "dark_entry", "activate",
			    GTK_SIGNAL_FUNC (imf_red_activate_cb));
	set_named_callback (GTK_OBJECT (dialog), "flat_entry", "activate",
			    GTK_SIGNAL_FUNC (imf_red_activate_cb));
	set_named_callback (GTK_OBJECT (dialog), "align_entry", "activate",
			    GTK_SIGNAL_FUNC (imf_red_activate_cb));
	set_named_callback (GTK_OBJECT (dialog), "recipe_entry", "activate",
			    GTK_SIGNAL_FUNC (imf_red_activate_cb));
	menubar = get_main_menu_bar(dialog);
	gtk_object_set_data(GTK_OBJECT(dialog), "menubar", menubar);
	//gtk_menu_bar_set_shadow_type(GTK_MENU_BAR(menubar), GTK_SHADOW_NONE);
	
	top_hb = gtk_object_get_data(GTK_OBJECT(dialog), "top_hbox");
	gtk_box_pack_start(GTK_BOX(top_hb), menubar, TRUE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(top_hb), menubar, 0);
	d3_printf("menubar: %p\n", menubar);
	gtk_widget_show(menubar);

	set_processing_dialog_ccdr(dialog, NULL);
	return dialog;
}

static void demosaic_method_activate(GtkWidget *wid, gpointer data)
{
	P_INT(CCDRED_DEMOSAIC_METHOD) = (long)data;
	par_touch(CCDRED_DEMOSAIC_METHOD);
}
 

static void stack_method_activate(GtkWidget *wid, gpointer data)
{
	P_INT(CCDRED_STACK_METHOD) = (long)data;
	par_touch(CCDRED_STACK_METHOD);
}
 
/* update the dialog to match the supplied ccdr */
/* in ccdr is null, just update the settings from the pars */
static void set_processing_dialog_ccdr(GtkWidget *dialog, struct ccd_reduce *ccdr)
{
	GtkWidget *menu, *omenu;
	GtkWidget *menuitem;
	char **c;
	long i;

	omenu = gtk_object_get_data(GTK_OBJECT(dialog), "stack_method_optmenu");
	g_return_if_fail(omenu != NULL);
	menu = gtk_menu_new();
	g_return_if_fail(menu != NULL);
	
	c = stack_methods;
	i = 0;
	while (*c != NULL) {
		menuitem = gtk_menu_item_new_with_label (*c);
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (stack_method_activate), (gpointer)i);
		d3_printf("add %s to stack method menu\n", *c);
		c++;
		i++;
	}
	gtk_widget_show(menu);
	gtk_widget_show(omenu);
	gtk_option_menu_remove_menu(GTK_OPTION_MENU (omenu));
	gtk_option_menu_set_menu(GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), 
				     P_INT(CCDRED_STACK_METHOD));
	named_spin_set(dialog, "stack_sigmas_spin", P_DBL(CCDRED_SIGMAS));
	named_spin_set(dialog, "stack_iter_spin", 1.0 * P_INT(CCDRED_ITER));

	omenu = gtk_object_get_data(GTK_OBJECT(dialog), "demosaic_method_optmenu");
	g_return_if_fail(omenu != NULL);
	menu = gtk_menu_new();
	g_return_if_fail(menu != NULL);
	
	c = demosaic_methods;
	i = 0;
	while (*c != NULL) {
		menuitem = gtk_menu_item_new_with_label (*c);
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (demosaic_method_activate), (gpointer)i);
		d3_printf("add %s to demosaic method menu\n", *c);
		c++;
		i++;
	}
	gtk_widget_show(menu);
	gtk_widget_show(omenu);
	gtk_option_menu_remove_menu(GTK_OPTION_MENU (omenu));
	gtk_option_menu_set_menu(GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), 
				     P_INT(CCDRED_DEMOSAIC_METHOD));

	if (ccdr == NULL)
		return;
	if ((ccdr->bias) && (ccdr->ops & IMG_OP_BIAS)) {
		named_entry_set(dialog, "bias_entry", ccdr->bias->filename);
		set_named_checkb_val(dialog, "bias_checkb", 1);
	}
	if ((ccdr->dark) && (ccdr->ops & IMG_OP_DARK)) {
		named_entry_set(dialog, "dark_entry", ccdr->dark->filename);
		set_named_checkb_val(dialog, "dark_checkb", 1);
	}
	if ((ccdr->flat) && (ccdr->ops & IMG_OP_FLAT)) {
		named_entry_set(dialog, "flat_entry", ccdr->flat->filename);
		set_named_checkb_val(dialog, "flat_checkb", 1);
	}
	if ((ccdr->bad_pix_map) && (ccdr->ops & IMG_OP_BADPIX)) {
		named_entry_set(dialog, "badpix_entry", ccdr->bad_pix_map->filename);
		set_named_checkb_val(dialog, "badpix_checkb", 1);
	}
	if ((ccdr->alignref) && (ccdr->ops & IMG_OP_ALIGN)) {
		named_entry_set(dialog, "align_entry", ccdr->alignref->filename);
		set_named_checkb_val(dialog, "align_checkb", 1);
	}
	if ((ccdr->ops & IMG_OP_BLUR)) {
		named_spin_set(dialog, "blur_spin", ccdr->blurv);
		set_named_checkb_val(dialog, "blur_checkb", 1);
	}
	if ((ccdr->ops & IMG_OP_ADD)) {
		named_spin_set(dialog, "add_spin", ccdr->addv);
		set_named_checkb_val(dialog, "add_checkb", 1);
	}
	if ((ccdr->ops & IMG_OP_MUL)) {
		named_spin_set(dialog, "mul_spin", ccdr->addv);
		set_named_checkb_val(dialog, "mul_checkb", 1);
	}
	if ((ccdr->ops & IMG_OP_DEMOSAIC)) {
		set_named_checkb_val(dialog, "demosaic_checkb", 1);
	}
	if ((ccdr->ops & IMG_OP_STACK)) {
		if ((ccdr->ops & IMG_OP_BG_ALIGN_MUL)) {
			set_named_checkb_val(dialog, "bg_match_mul_rb", 1);
		} else {
			set_named_checkb_val(dialog, "bg_match_add_rb", 1);
		} 
		set_named_checkb_val(dialog, "stack_checkb", 1);
	}
	if ((ccdr->ops & IMG_OP_INPLACE)) {
		set_named_checkb_val(dialog, "overwrite_checkb", 1);
	}
}

/* replace the file list in the dialog with the supplied one */
static void set_processing_dialog_imfl(GtkWidget *dialog, struct image_file_list *imfl)
{
	GtkList *list;
	GtkWidget *item;
	GList *il = NULL;
	GList *gl;
	struct image_file *imf;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);
	gtk_list_clear_items(list, -1, -1);

	gl = imfl->imlist;
	while (gl != NULL) {
		imf = gl->data;
		gl = g_list_next(gl);

		item=gtk_list_item_new_with_label("");
		imf_update_status_label(GTK_WIDGET(GTK_BIN(item)->child), imf);
		image_file_ref(imf);
		gtk_object_set_data_full(GTK_OBJECT(item), "imf",
					 imf, (GtkDestroyNotify)(image_file_release));

		il=g_list_append(il, item);
		gtk_widget_show(item);

		if (imf->flags & IMG_SKIP)
			continue;
	}
	gtk_list_append_items(list, il);
}

void set_imfl_ccdr(gpointer window, struct ccd_reduce *ccdr, 
		   struct image_file_list *imfl)
{
	GtkWidget *dialog;
	dialog = gtk_object_get_data(GTK_OBJECT(window), "processing");
	if (dialog == NULL) {
		dialog = make_image_processing(window);
	}
	g_return_if_fail(dialog != NULL);

	if (imfl) {
		image_file_list_ref(imfl);
		gtk_object_set_data_full(GTK_OBJECT(dialog), "imfl",
					 imfl, (GtkDestroyNotify)(image_file_list_release));
		set_processing_dialog_imfl(dialog, imfl);
	}
	if (ccdr) {
		ccd_reduce_ref(ccdr);
		gtk_object_set_data_full(GTK_OBJECT(dialog), "ccdred",
					 ccdr, (GtkDestroyNotify)(ccd_reduce_release));
		set_processing_dialog_ccdr(dialog, ccdr);
	}
}

/* mark selected files to be skipped */
static void imf_skip_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GList *sel = NULL;
	GtkList *list;
	GtkListItem *item;
	struct image_file *imf;
//	char label[256];

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	sel = list->selection;
	while(sel != NULL) {
		item = sel->data;
		sel = g_list_next(sel);
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		if (imf == NULL) {
			err_printf("null imf\n");
			continue;
		}
		d3_printf("skipping %s\n", imf->filename);
		imf->flags |= IMG_SKIP;
//		snprintf(label, 255, "[ %s ]", imf->filename);
//		gtk_label_set_text(GTK_LABEL(GTK_BIN(item)->child), label);
	}
	update_selected_status_label(dialog);
	if (g_list_length(list->selection) == 1) 
		imf_next_cb(dialog, 0, NULL);
}

/* remove skip marks from selected files */
static void imf_unskip_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GList *sel = NULL;
	GtkList *list;
	GtkListItem *item;
	struct image_file *imf;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	sel = list->selection;
	while(sel != NULL) {
		item = sel->data;
		sel = g_list_next(sel);
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		if (imf == NULL) {
			err_printf("null imf\n");
			continue;
		}
		d3_printf("unskipping %s\n", imf->filename);
		imf->flags &= ~IMG_SKIP;
//		gtk_label_set_text(GTK_LABEL(GTK_BIN(item)->child), imf->filename);
	}
	update_selected_status_label(dialog);
}

/* remove selected files */
static void imf_rm_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GList *sel = NULL;
	GList *sel2 = NULL;
	GtkList *list;
	GtkListItem *item;
	struct image_file *imf;
	struct image_file_list *imfl;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
	g_return_if_fail(imfl != NULL);

	sel = list->selection;
	sel2 = g_list_copy(list->selection);
	while(sel != NULL) {
		item = sel->data;
		sel = g_list_next(sel);
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		if (imf == NULL) {
			err_printf("null imf\n");
			continue;
		}
		d3_printf("removing %s\n", imf->filename);
		imfl->imlist = g_list_remove(imfl->imlist, imf);
		image_file_release(imf);
	}
	gtk_list_unselect_all(list);
	gtk_list_remove_items(list, sel2);
	g_list_free(sel2);
	update_selected_status_label(dialog);
}

/* select and display next frame in list */
static void imf_next_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GList *sel = NULL;
	GtkList *list;
	int index, len;
	GtkWidget *scw;
	GtkAdjustment *vadj;
	double nv;

	d3_printf("next\n");

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	sel = list->selection;
	if (sel == NULL) {
		gtk_list_select_item(list, 0);
		imf_display_cb(dialog, 0, NULL);
		return;
	}
	index = gtk_list_child_position(list, GTK_WIDGET(sel->data));
	d3_printf("initial position is %d\n", index);
	/* we reuse sel here! */
	sel = gtk_container_children(GTK_CONTAINER(list));
	len = g_list_length(sel);
	g_list_free(sel);
	if (index + 1 < len) { 
		gtk_list_unselect_all(list);
		index ++;
		gtk_list_select_item(list, index);
		imf_display_cb(dialog, 0, NULL);
	} else if (index + 1 == len) {
		gtk_list_unselect_all(list);
		gtk_list_select_item(list, index);
		imf_display_cb(dialog, 0, NULL);
	}

	scw = gtk_object_get_data(GTK_OBJECT(dialog), "scrolledwindow");
	g_return_if_fail(scw != NULL);
	vadj =  gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scw));
	d3_printf("vadj at %.3f\n", vadj->value);
	if (len != 0) {
		nv = (vadj->upper + vadj->lower) * index / len - vadj->page_size / 2;
		clamp_double(&nv, vadj->lower, vadj->upper - vadj->page_size);
		gtk_adjustment_set_value(vadj, nv);
		d3_printf("vadj set to %.3f\n", vadj->value);
	}
	update_selected_status_label(dialog);
}


/* select and display previous frame in list */
static void imf_prev_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GList *sel = NULL;
	GtkList *list;
	int index, len;
	GtkWidget *scw;
	GtkAdjustment *vadj;
	double nv;

	d3_printf("prev\n");

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	sel = list->selection;
	if (sel == NULL) {
		gtk_list_select_item(list, 0);
		imf_display_cb(dialog, 0, NULL);
		return;
	}
	index = gtk_list_child_position(list, GTK_WIDGET(sel->data));
	d3_printf("initial position is %d\n", index);
	/* we reuse sel here! */
	sel = gtk_container_children(GTK_CONTAINER(list));
	len = g_list_length(sel);
	g_list_free(sel);
	if (index > 0) { 
		gtk_list_unselect_all(list);
		index --;
		gtk_list_select_item(list, index);
		imf_display_cb(dialog, 0, NULL);
	} else if (index == 0) {
		gtk_list_unselect_all(list);
		gtk_list_select_item(list, index);
		imf_display_cb(dialog, 0, NULL);
	}

	scw = gtk_object_get_data(GTK_OBJECT(dialog), "scrolledwindow");
	g_return_if_fail(scw != NULL);
	vadj =  gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scw));
	d3_printf("vadj at %.3f\n", vadj->value);
	if (len != 0) {
		nv = (vadj->upper + vadj->lower) * index / len - vadj->page_size / 2;
		clamp_double(&nv, vadj->lower, vadj->upper - vadj->page_size);
		gtk_adjustment_set_value(vadj, nv);
		d3_printf("vadj set to %.3f\n", vadj->value);
	}
	update_selected_status_label(dialog);
}


static void imf_display_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GtkWidget *im_window;
	GtkList *list;
	GtkListItem *item;
	struct image_file *imf;
	struct image_file_list *imfl;

	d3_printf("display\n");

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);
	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	if (list->selection != NULL) {
		item = list->selection->data;
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		if (imf == NULL) {
			err_printf("null imf\n");
			return;
		}
		if (load_image_file(imf)) {
			return;
		}
		g_return_if_fail(imf->fr != NULL);
		frame_to_channel(imf->fr, im_window, "i_channel");
//		imf_update_status_label(dialog, imf);
	} else {
		error_beep();
		log_msg("\nNo Frame selected\n", dialog);
	}
	if (P_INT(FILE_SAVE_MEM)) {
		imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
		if (imfl)
			unload_clean_frames(imfl);
	}
	update_selected_status_label(dialog);
}

static void imf_selall_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GtkList *list;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	gtk_list_select_all(list);
}

static void imf_add_files(GSList *files, gpointer dialog)
{
	GtkList *list;
	GtkWidget *item;
	GList *il = NULL;
	struct image_file *imf;
	struct image_file_list *imfl;
	char *text;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
	if (imfl == NULL) {
		imfl = image_file_list_new();
		gtk_object_set_data_full(GTK_OBJECT(dialog), "imfl",
					 imfl, (GtkDestroyNotify)(image_file_list_release));
	}

	while (files != NULL) {
		imf = image_file_new();
		text = files->data;
		files = g_slist_next(files);

		imf->filename = strdup(text);
		item=gtk_list_item_new_with_label(imf->filename);
		image_file_ref(imf);
		gtk_object_set_data_full(GTK_OBJECT(item), "imf",
					 imf, (GtkDestroyNotify)(image_file_release));
		image_file_ref(imf);
		imfl->imlist = g_list_append(imfl->imlist, imf);

		il=g_list_append(il, item);
		gtk_widget_show(item);
		d3_printf("adding %s\n", text);
	}
	gtk_list_append_items(list, il);
	update_selected_status_label(dialog);
}

static void imf_add_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GtkList *list;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	d3_printf("imf add\n");

	file_select_list(dialog, "Select files", "*.fits", imf_add_files);
}

/* reload selected files */
static void imf_reload_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	GList *sel = NULL;
	GtkList *list;
	GtkListItem *item;
	struct image_file *imf;
	struct image_file_list *imfl;

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
	g_return_if_fail(imfl != NULL);

	sel = list->selection;
	while(sel != NULL) {
		item = sel->data;
		sel = g_list_next(sel);
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		if (imf == NULL) {
			err_printf("null imf\n");
			continue;
		}
		d3_printf("unloading %s\n", imf->filename);
		if ((imf->flags & IMG_LOADED) && (imf->fr)) {
			release_frame(imf->fr);
			imf->fr = NULL;
		}
		imf->flags &= IMG_SKIP; /* we keep the skip flag */
	}
	update_selected_status_label(dialog);
}

void switch_frame_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	GtkWidget *dialog;
	dialog = gtk_object_get_data(GTK_OBJECT(window), "processing");
	if (dialog == NULL) {
		dialog = make_image_processing(window);
	}
	g_return_if_fail(dialog != NULL);

	switch(action) {
	case SWF_NEXT:
		imf_next_cb(dialog, 0, NULL);
		break;
	case SWF_SKIP:
		imf_skip_cb(dialog, 0, NULL);
		break;
	case SWF_PREV:
		imf_prev_cb(dialog, 0, NULL);
		break;
	case SWF_QPHOT:
		ccdred_qphotone_cb(dialog, 0, NULL);
		break;
	case SWF_RED:
		ccdred_one_cb(dialog, 0, NULL);
		break;
	}
	update_selected_status_label(dialog);
}

static void imf_update_status_label(GtkWidget *label, struct image_file *imf)
{
	char *fn;
	char msg[128];
	int i;

	if (imf == NULL) {
		gtk_label_set_text(GTK_LABEL(label), "");
		return;
	}

	fn = strdup(imf->filename);

	i = snprintf(msg, 127, "%s", basename(fn));
	free(fn);
	if (imf->flags & IMG_SKIP) 
		i += snprintf(msg+i, 127-i, " SKIP");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_LOADED)
		i += snprintf(msg+i, 127-i, " LD");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_BIAS)
		i += snprintf(msg+i, 127-i, " BIAS");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_DARK)
		i += snprintf(msg+i, 127-i, " DARK");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_FLAT)
		i += snprintf(msg+i, 127-i, " FLAT");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_BADPIX)
		i += snprintf(msg+i, 127-i, " BADPIX");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_MUL)
		i += snprintf(msg+i, 127-i, " MUL");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_ADD)
		i += snprintf(msg+i, 127-i, " ADD");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_ALIGN)
		i += snprintf(msg+i, 127-i, " ALIGN");
	clamp_int(&i, 0, 127);
	if (imf->flags & IMG_OP_DEMOSAIC)
		i += snprintf(msg+i, 127-i, " DEMOSAIC");
	clamp_int(&i, 0, 127);	
	if (imf->flags & IMG_OP_PHOT)
		i += snprintf(msg+i, 127-i, " PHOT");
	gtk_label_set_text(GTK_LABEL(label), msg);
}

static void update_selected_status_label(gpointer dialog)
{
	GtkWidget *im_window;
	GtkList *list;
	GtkListItem *item;
	GList *sel;
	struct image_file *imf;

	d3_printf("update selected status\n");

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);
	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	for (sel = list->selection; sel != NULL; sel = sel->next) {
		item = sel->data;
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		imf_update_status_label(GTK_WIDGET(GTK_BIN(item)->child), imf);
	}
}

static void update_status_labels(gpointer dialog)
{
	GtkWidget *im_window;
	GtkList *list;
	GtkListItem *item;
	GList *sel;
	struct image_file *imf;

	d3_printf("update selected status\n");

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);
	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	for (sel = gtk_container_children(GTK_CONTAINER(list)); sel != NULL; sel = sel->next) {
		item = sel->data;
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
		imf_update_status_label(GTK_WIDGET(GTK_BIN(item)->child), imf);
	}
}





static void select_cb(GtkList *list, GtkWidget *wid, gpointer dialog)
{
	char *text;
	struct image_file *imf;

	return;
	gtk_label_get(GTK_LABEL(GTK_BIN(wid)->child), &text);
	d3_printf("select: %s\n", text);

	imf = gtk_object_get_data(GTK_OBJECT(wid), "imf");
	g_return_if_fail(imf != NULL);
//	update_status_label(dialog, imf);
}

/*
static void list_button_cb(GtkWidget *wid, GdkEventButton *event, gpointer dialog)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		while(gtk_events_pending())
		      gtk_main_iteration();
		imf_display_cb(NULL, dialog);
	}
}
*/

static void imf_red_browse_cb(GtkWidget *wid, gpointer dialog)
{
	GtkWidget *entry;
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "bias_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "bias_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select bias frame", "*", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "dark_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "dark_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select dark frame", "*", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "flat_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "flat_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select flat frame", "*", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "badpix_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "badpix_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select bad pixel file", "*", 1);
	}	
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "align_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "align_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select alignment reference frame", "*", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "recipe_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "recipe_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select recipe file", "*", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "output_file_browse")) {
		entry = gtk_object_get_data(GTK_OBJECT(dialog), "output_file_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select output file/dir", "*", 1);
	}
}

static int progress_pr(char *msg, void *dialog)
{
	GtkWidget *text;
//	GdkFont *font = NULL;

//	d1_printf("*%s", msg);

	text = gtk_object_get_data(GTK_OBJECT(dialog), "processing_log_text");
	g_return_val_if_fail(text != NULL, 0);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text), GTK_WRAP_CHAR);
	gtk_text_buffer_insert_at_cursor(
		gtk_text_view_get_buffer (GTK_TEXT_VIEW(text)),
		msg, -1);

	while(gtk_events_pending())
		gtk_main_iteration();

	return 0;
}


static int log_msg(char *msg, void *dialog)
{
	GtkWidget *text;
//	GdkFont *font = NULL;

//	d1_printf("*%s", msg);

	text = gtk_object_get_data(GTK_OBJECT(dialog), "processing_log_text");
	g_return_val_if_fail(text != NULL, 0);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);
	gtk_text_buffer_insert_at_cursor(
		gtk_text_view_get_buffer (GTK_TEXT_VIEW(text)),
		msg, -1);

	return 0;
}


static void dialog_to_ccdr(GtkWidget *dialog, struct ccd_reduce *ccdr)
{
	char *text;

	g_return_if_fail (ccdr != NULL);
	if (get_named_checkb_val(dialog, "bias_checkb")) {
		text = named_entry_text(dialog, "bias_entry");
		if ((ccdr->ops & IMG_OP_BIAS) && ccdr->bias && 
		    strcmp(text, ccdr->bias->filename)) {
			image_file_release(ccdr->bias);
			ccdr->bias = image_file_new();
			ccdr->bias->filename = strdup(text);
		} else if (!(ccdr->ops & IMG_OP_BIAS)) {
			ccdr->bias = image_file_new();
			ccdr->bias->filename = strdup(text);
		} 
		g_free(text);
		ccdr->ops |= IMG_OP_BIAS;
	} else {
		if ((ccdr->ops & IMG_OP_BIAS) && ccdr->bias)
			image_file_release(ccdr->bias);
		ccdr->bias = NULL;
		ccdr->ops &= ~IMG_OP_BIAS;
	}

	if (get_named_checkb_val(dialog, "dark_checkb")) {
		text = named_entry_text(dialog, "dark_entry");
		if ((ccdr->ops & IMG_OP_DARK) && ccdr->dark && 
		    strcmp(text, ccdr->dark->filename)) {
			image_file_release(ccdr->dark);
			ccdr->dark = image_file_new();
			ccdr->dark->filename = strdup(text);
		} else if (!(ccdr->ops & IMG_OP_DARK)) {
			ccdr->dark = image_file_new();
			ccdr->dark->filename = strdup(text);
		} 
		g_free(text);
		ccdr->ops |= IMG_OP_DARK;
	} else {
		if ((ccdr->ops & IMG_OP_DARK) && ccdr->dark)
			image_file_release(ccdr->dark);
		ccdr->dark = NULL;
		ccdr->ops &= ~IMG_OP_DARK;		
	}

	if (get_named_checkb_val(dialog, "flat_checkb")) {
		text = named_entry_text(dialog, "flat_entry");
		if ((ccdr->ops & IMG_OP_FLAT) && ccdr->flat && 
		    strcmp(text, ccdr->flat->filename)) {
			image_file_release(ccdr->flat);
			ccdr->flat = image_file_new();
			ccdr->flat->filename = strdup(text);
		} else if (!(ccdr->ops & IMG_OP_FLAT)) {
			ccdr->flat = image_file_new();
			ccdr->flat->filename = strdup(text);
		} 
		g_free(text);
		ccdr->ops |= IMG_OP_FLAT;
	} else {
		if ((ccdr->ops & IMG_OP_FLAT) && ccdr->flat)
			image_file_release(ccdr->flat);
		ccdr->flat = NULL;
		ccdr->ops &= ~IMG_OP_FLAT;		
	}

	if (get_named_checkb_val(dialog, "badpix_checkb")) {
		text = named_entry_text(dialog, "badpix_entry");
		if ((ccdr->ops & IMG_OP_BADPIX) && ccdr->bad_pix_map &&
		    strcmp(text, ccdr->bad_pix_map->filename)) {
			free_bad_pix(ccdr->bad_pix_map);
			ccdr->bad_pix_map = calloc(1, sizeof(struct bad_pix_map));
			ccdr->bad_pix_map->filename = strdup(text);
		} else if (!(ccdr->ops & IMG_OP_BADPIX)) {
			ccdr->bad_pix_map = calloc(1, sizeof(struct bad_pix_map));
			ccdr->bad_pix_map->filename = strdup(text);
		} 
		g_free(text);
		ccdr->ops |= IMG_OP_BADPIX;
	} else {
		if ((ccdr->ops & IMG_OP_BADPIX) && ccdr->bad_pix_map) {
			free_bad_pix(ccdr->bad_pix_map);
		}
		ccdr->bad_pix_map = NULL;
		ccdr->ops &= ~IMG_OP_BADPIX;
	}


	if (get_named_checkb_val(dialog, "align_checkb")) {
		text = named_entry_text(dialog, "align_entry");
		if ((ccdr->ops & IMG_OP_ALIGN) && ccdr->alignref && 
		    strcmp(text, ccdr->alignref->filename)) {
			image_file_release(ccdr->alignref);
			ccdr->alignref = image_file_new();
			ccdr->alignref->filename = strdup(text);
			free_alignment_stars(ccdr);
		} else if (!(ccdr->ops & IMG_OP_ALIGN)) {
			ccdr->alignref = image_file_new();
			ccdr->alignref->filename = strdup(text);
		} 
		g_free(text);
		ccdr->ops |= IMG_OP_ALIGN;
	} else {
		ccdr->ops &= ~IMG_OP_ALIGN;
		if ((ccdr->ops & IMG_OP_ALIGN) && ccdr->alignref)
			image_file_release(ccdr->alignref);
		ccdr->alignref = NULL;
	}

	if (get_named_checkb_val(dialog, "mul_checkb")) {
		ccdr->ops |= IMG_OP_MUL;
		ccdr->mulv = named_spin_get_value(dialog, "mul_spin");
	} else {
		ccdr->ops &= ~IMG_OP_MUL;
	}

	if (get_named_checkb_val(dialog, "add_checkb")) {
		ccdr->ops |= IMG_OP_ADD;
		ccdr->addv = named_spin_get_value(dialog, "add_spin");
	} else {
		ccdr->ops &= ~IMG_OP_ADD;
	}

	if (get_named_checkb_val(dialog, "blur_checkb")) {
		ccdr->ops |= IMG_OP_BLUR;
		ccdr->blurv = named_spin_get_value(dialog, "blur_spin");
	} else {
		ccdr->ops &= ~IMG_OP_BLUR;
	}

	if (get_named_checkb_val(dialog, "phot_en_checkb")) {
		gpointer window;
		int i;
		window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
		g_return_if_fail(window != NULL);
		ccdr->window = window;
		if (ccdr->wcs) {
			wcs_release(ccdr->wcs);
			ccdr->wcs = NULL;
		}
		if (ccdr->recipe)
			free(ccdr->recipe);
		text = named_entry_text(dialog, "recipe_entry");
		for(i = 0; text != NULL && text[i] == ' '; i++);
		if (text[i])
			ccdr->recipe = strdup(text+i);
		else
			ccdr->recipe = NULL;
		free(text);
		if (get_named_checkb_val(dialog, "phot_reuse_wcs_checkb")) {
			struct wcs *wcs;
			wcs = gtk_object_get_data(GTK_OBJECT(window), "wcs_of_window");
			if ((wcs == NULL) || ((wcs->wcsset & WCS_VALID) == 0)) {
				err_printf("invalid wcs inherited\n");
			} else {
				ccdr->wcs = wcs_new();
				memcpy(ccdr->wcs, wcs, sizeof(struct wcs));
				ccdr->ops |= IMG_OP_PHOT_REUSE_WCS;
			}
		} else {
			ccdr->ops &= ~(IMG_OP_PHOT_REUSE_WCS);
		}
		ccdr->ops |= IMG_OP_PHOT;
	} else {
		ccdr->ops &= ~(IMG_OP_PHOT | IMG_OP_PHOT_REUSE_WCS);
	}

	if (get_named_checkb_val(dialog, "demosaic_checkb")) {
		ccdr->ops |= IMG_OP_DEMOSAIC;
	} else {
		ccdr->ops &= ~IMG_OP_DEMOSAIC;
	}

	if (get_named_checkb_val(dialog, "stack_checkb")) {
		ccdr->ops |= IMG_OP_STACK;
		if (get_named_checkb_val(dialog, "bg_match_add_rb")) {
			ccdr->ops |= IMG_OP_BG_ALIGN_ADD;
		} else {
			ccdr->ops &= ~IMG_OP_BG_ALIGN_ADD;
		}
		if (get_named_checkb_val(dialog, "bg_match_mul_rb")) {
			ccdr->ops |= IMG_OP_BG_ALIGN_MUL;
		} else {
			ccdr->ops &= ~IMG_OP_BG_ALIGN_MUL;
		}
		P_DBL(CCDRED_SIGMAS) = named_spin_get_value(dialog, 
							    "stack_sigmas_spin");
		par_touch(CCDRED_SIGMAS);
		P_INT(CCDRED_ITER) = named_spin_get_value(dialog, 
							  "stack_iter_spin");
		par_touch(CCDRED_ITER);
	} else {
		ccdr->ops &= ~IMG_OP_STACK;
	}
}

static void ccdred_run_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	struct ccd_reduce *ccdr;	
	struct image_file_list *imfl;
	int ret, nframes;
	struct image_file *imf;
	struct ccd_frame *fr;
	GList *gl;
	int seq = 1;
	char *outf;
	GtkWidget *im_window;
	GtkWidget *menubar;


	imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);
	ccdr = gtk_object_get_data(GTK_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		gtk_object_set_data_full(GTK_OBJECT(dialog), "ccdred",
					 ccdr, (GtkDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	ccdr->ops &= ~CCDR_BG_VAL_SET;
	dialog_to_ccdr(dialog, ccdr);

	outf = named_entry_text(dialog, "output_file_entry");
	d3_printf("outf is |%s|\n", outf);

	menubar = gtk_object_get_data(GTK_OBJECT(dialog), "menubar");
	gtk_widget_set_sensitive(menubar, 0);

	if (ccdr->ops & IMG_OP_PHOT) {
		if (ccdr->multiband == NULL) {
			gpointer mbd;
			im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
			mbd = gtk_object_get_data(GTK_OBJECT(im_window), "mband_window");
			if (mbd == NULL) {
				mband_open_cb(im_window, 0, NULL);
				mbd = gtk_object_get_data(GTK_OBJECT(im_window), "mband_window");
			}
			gtk_widget_ref(GTK_WIDGET(mbd));
			ccdr->multiband = mbd;
			gtk_object_set_data_full(GTK_OBJECT(dialog), "mband_window",
						 mbd, (GtkDestroyNotify)(gtk_widget_unref));
		}
	}

	nframes = g_list_length(imfl->imlist);
	if (!(ccdr->ops & IMG_OP_STACK)) {
//		ccdr->ops &= ~(IMG_OP_BG_ALIGN_MUL | IMG_OP_BG_ALIGN_ADD);
		gl = imfl->imlist;
		while (gl != NULL) {
			imf = gl->data;
			gl = g_list_next(gl);
			if (imf->flags & IMG_SKIP)
				continue;
			ret = reduce_frame(imf, ccdr, progress_pr, dialog);
			if (ret)
				continue;
			if (ccdr->ops & IMG_OP_INPLACE) {
				save_image_file(imf, outf, 1, NULL, progress_pr, dialog);
				imf->flags &= ~(IMG_LOADED);
				release_frame(imf->fr);
				imf->fr = NULL;
			} else if (outf && outf[0]) {
				save_image_file(imf, outf, 0, 
						(nframes == 1 ? NULL : &seq), 
						progress_pr, dialog);
				imf->flags &= ~(IMG_LOADED);
				release_frame(imf->fr);
				imf->fr = NULL;
			} else if ( ((ccdr->ops & (IMG_OP_ALIGN)) == 0) && 
				   (ccdr->ops & IMG_OP_PHOT) ) {
				imf->flags &= ~(IMG_LOADED);
				release_frame(imf->fr);
				imf->fr = NULL;
			}
		}
		update_status_labels(dialog);
		goto end;
	}

	if (reduce_frames(imfl, ccdr, progress_pr, dialog))
		goto end;

	update_status_labels(dialog);

	if ((fr = stack_frames(imfl, ccdr, progress_pr, dialog)) == NULL)
		goto end;

	if (outf && outf[0]) {
		imf = image_file_new();
		imf->filename = outf;
		imf->fr = fr;
		imf->flags |= IMG_LOADED;
		get_frame(fr);
		snprintf(fr->name, 254, "%s", outf); 
		d3_printf("Writing %s\n", outf);
		save_image_file(imf, outf, 0, &seq, 
				progress_pr, dialog);
		image_file_release(imf);
	}
	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);
	frame_to_channel(fr, im_window, "i_channel");
	release_frame(fr);
end:
	gtk_widget_set_sensitive(menubar, 1);
	return;


}


static struct image_file * current_imf(gpointer dialog)
{
	GtkWidget *im_window;
	GtkList *list;
	GtkListItem *item;
	struct image_file *imf = NULL;

	d3_printf("get current imf\n");

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_val_if_fail(im_window != NULL, NULL);
	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_val_if_fail(list != NULL, NULL);

	if (list->selection == NULL) 
		gtk_list_select_item(list, 0);

	if (list->selection != NULL) {
		item = list->selection->data;
		imf = gtk_object_get_data(GTK_OBJECT(item), "imf");
	}
	return imf;
}

static void select_next_imf(gpointer dialog)
{
	GList *sel = NULL;
	GtkList *list;
	int index, len;
	GtkWidget *scw;
	GtkAdjustment *vadj;
	double nv;

	d3_printf("select next\n");

	list = gtk_object_get_data(GTK_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	sel = list->selection;
	if (sel == NULL) {
		gtk_list_select_item(list, 0);
		update_selected_status_label(dialog);
		return;
	}
	index = gtk_list_child_position(list, GTK_WIDGET(sel->data));
	d3_printf("initial position is %d\n", index);
	/* we reuse sel here! */
	sel = gtk_container_children(GTK_CONTAINER(list));
	len = g_list_length(sel);
	g_list_free(sel);
	if (index + 1 < len) { 
		gtk_list_unselect_all(list);
		index ++;
		gtk_list_select_item(list, index);
	} else if (index + 1 == len) {
		gtk_list_unselect_all(list);
		gtk_list_select_item(list, index);
	}

	scw = gtk_object_get_data(GTK_OBJECT(dialog), "scrolledwindow");
	g_return_if_fail(scw != NULL);
	vadj =  gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scw));
	d3_printf("vadj at %.3f\n", vadj->value);
	if (len != 0) {
		nv = (vadj->upper + vadj->lower) * index / len - vadj->page_size / 2;
		clamp_double(&nv, vadj->lower, vadj->upper - vadj->page_size);
		gtk_adjustment_set_value(vadj, nv);
		d3_printf("vadj set to %.3f\n", vadj->value);
	}
	update_selected_status_label(dialog);
}


static void ccdred_one_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	struct ccd_reduce *ccdr;	
	struct image_file_list *imfl;
	int ret;
	struct image_file *imf;
	char *outf;
	GtkWidget *im_window;
	GtkWidget *menubar;


	imf = current_imf(dialog);
	if (imf == NULL || imf->flags & IMG_SKIP) {
		select_next_imf(dialog);
		return;
	}

	imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);
	ccdr = gtk_object_get_data(GTK_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		gtk_object_set_data_full(GTK_OBJECT(dialog), "ccdred",
					 ccdr, (GtkDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	ccdr->ops &= ~CCDR_BG_VAL_SET;
	dialog_to_ccdr(dialog, ccdr);

	outf = named_entry_text(dialog, "output_file_entry");
	d3_printf("outf is |%s|\n", outf);

	menubar = gtk_object_get_data(GTK_OBJECT(dialog), "menubar");
	gtk_widget_set_sensitive(menubar, 0);

	if (ccdr->ops & IMG_OP_PHOT) {
		if (ccdr->multiband == NULL) {
			gpointer mbd;
			im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
			mbd = gtk_object_get_data(GTK_OBJECT(im_window), "mband_window");
			if (mbd == NULL) {
				mband_open_cb(im_window, 0, NULL);
				mbd = gtk_object_get_data(GTK_OBJECT(im_window), "mband_window");
			}
			gtk_widget_ref(GTK_WIDGET(mbd));
			ccdr->multiband = mbd;
			gtk_object_set_data_full(GTK_OBJECT(dialog), "mband_window",
						 mbd, (GtkDestroyNotify)(gtk_widget_unref));
		}
	}

	ret = reduce_frame(imf, ccdr, progress_pr, dialog);
	if ((ccdr->ops & IMG_OP_PHOT) == 0) {
		imf_display_cb(dialog, 0, NULL);
	}
	if (ret >= 0) {
		if (ccdr->ops & IMG_OP_INPLACE) {
			save_image_file(imf, outf, 1, NULL, progress_pr, dialog);
			imf->flags &= ~(IMG_LOADED);
			release_frame(imf->fr);
			imf->fr = NULL;
		} else if (outf && outf[0]) {
			save_image_file(imf, outf, 0, NULL, progress_pr, dialog);
			imf->flags &= ~(IMG_LOADED);
			release_frame(imf->fr);
			imf->fr = NULL;
		} else if ( ((ccdr->ops & (IMG_OP_ALIGN)) == 0) && 
			    (ccdr->ops & IMG_OP_PHOT) ) {
			imf->flags &= ~(IMG_LOADED);
			release_frame(imf->fr);
			imf->fr = NULL;
		}
	}
	update_selected_status_label(dialog);
	select_next_imf(dialog);
	if ((ccdr->ops & IMG_OP_PHOT)) {
		imf_display_cb(dialog, 0, NULL);
	}
	gtk_widget_set_sensitive(menubar, 1);
}

/* run quick phot on one frame */
static void ccdred_qphotone_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	struct ccd_reduce *ccdr;	
	struct image_file_list *imfl;
	int ret;
	struct image_file *imf;
	char *outf;
	GtkWidget *menubar;


	imf = current_imf(dialog);
	if (imf == NULL || imf->flags & IMG_SKIP) {
		select_next_imf(dialog);
		return;
	}

	imfl = gtk_object_get_data(GTK_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);
	ccdr = gtk_object_get_data(GTK_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		gtk_object_set_data_full(GTK_OBJECT(dialog), "ccdred",
					 ccdr, (GtkDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	ccdr->ops &= ~CCDR_BG_VAL_SET;
	dialog_to_ccdr(dialog, ccdr);
	if ((ccdr->ops & IMG_OP_PHOT) == 0) {
		error_beep();
		log_msg("\nPlease enable photometry in the setup tab\n", dialog);
		return;
	}
	outf = named_entry_text(dialog, "output_file_entry");
	d3_printf("outf is |%s|\n", outf);

	menubar = gtk_object_get_data(GTK_OBJECT(dialog), "menubar");
	gtk_widget_set_sensitive(menubar, 0);

	ccdr->ops |= IMG_QUICKPHOT;
	ret = reduce_frame(imf, ccdr, progress_pr, dialog);
	if ((ccdr->ops & IMG_OP_PHOT) == 0) {
		imf_display_cb(dialog, 0, NULL);
	}
	ccdr->ops &= ~IMG_QUICKPHOT;
	imf->flags &= ~IMG_OP_PHOT;
	update_selected_status_label(dialog);
	gtk_widget_set_sensitive(menubar, 1);
}


static void show_align_cb(gpointer dialog, guint action, GtkWidget *menu_item)
{
	struct ccd_reduce *ccdr;	
	GtkWidget *im_window;

	ccdr = gtk_object_get_data(GTK_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		gtk_object_set_data_full(GTK_OBJECT(dialog), "ccdred",
					 ccdr, (GtkDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	dialog_to_ccdr(dialog, ccdr);

	im_window = gtk_object_get_data(GTK_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);

	d3_printf("ops: %08x, align_stars: %08p\n", ccdr->ops, ccdr->align_stars);

	if (!(ccdr->ops & IMG_OP_ALIGN)) {
		error_beep();
		log_msg("\nPlease select an alignment ref frame first\n", dialog);
		return;
	}

	if (!(ccdr->ops & CCDR_ALIGN_STARS))
		load_alignment_stars(ccdr);

	remove_stars_of_type_window(im_window, TYPE_MASK(STAR_TYPE_ALIGN), 0);
	add_gui_stars_to_window(im_window, ccdr->align_stars);
	gtk_widget_queue_draw(im_window);
}

/* set the enable check buttons when the user activates a file entry */
static void imf_red_activate_cb(GtkWidget *wid, gpointer dialog)
{

	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "dark_entry")) {
		set_named_checkb_val(dialog, "dark_checkb", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "flat_entry")) {
		set_named_checkb_val(dialog, "flat_checkb", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "bias_entry")) {
		set_named_checkb_val(dialog, "bias_checkb", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "align_entry")) {
		set_named_checkb_val(dialog, "align_checkb", 1);
	}
	if (wid == gtk_object_get_data(GTK_OBJECT(dialog), "recipe_entry")) {
		set_named_checkb_val(dialog, "phot_en_checkb", 1);
	}

}




void processing_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	GtkWidget *dialog;

	dialog = gtk_object_get_data(GTK_OBJECT(window), "processing");
	if (dialog == NULL) {
		dialog = make_image_processing(window);
		gtk_widget_show_all(dialog);
	} else {
		gtk_widget_show(dialog);
		gdk_window_raise(dialog->window);
	}
}

