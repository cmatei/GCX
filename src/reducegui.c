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
#include "gcximageview.h"
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



/* file switch actions */
#define SWF_NEXT 1
#define SWF_SKIP 2
#define SWF_PREV 3
#define SWF_QPHOT 4
#define SWF_RED 5



static int progress_pr(char *msg, void *dialog);
static int log_msg(char *msg, void *dialog);

static void imf_display_cb (GtkAction *action, gpointer dialog);
static void imf_prev_cb(GtkAction *action, gpointer dialog);
static void imf_next_cb(GtkAction *action, gpointer dialog);
static void imf_rm_cb(GtkAction *action, gpointer dialog);
static void imf_reload_cb(GtkAction *action, gpointer dialog);
static void imf_skip_cb(GtkAction *action, gpointer dialog);
static void imf_unskip_cb(GtkAction *action, gpointer dialog);
static void imf_selall_cb(GtkAction *action, gpointer dialog);
static void imf_add_cb (GtkAction *action, gpointer dialog);
static void ccdred_run_cb(GtkAction *action, gpointer dialog);
static void ccdred_one_cb(GtkAction *action, gpointer dialog);
static void ccdred_qphotone_cb(GtkAction *action, gpointer dialog);
static void show_align_cb(GtkAction *action, gpointer dialog);
static void mframe_cb(GtkAction *action, gpointer dialog);

static void update_selected_status_label(gpointer dialog);
static void update_status_labels(gpointer dialog);
static void imf_red_activate_cb(GtkWidget *wid, gpointer dialog);
static void set_processing_dialog_ccdr(GtkWidget *dialog, struct ccd_reduce *ccdr);
static void imf_update_status_label(GtkTreeModel *list, GtkTreeIter *iter);
static void imf_red_browse_cb(GtkWidget *wid, gpointer dialog);


static gboolean close_processing_window( GtkWidget *widget, gpointer data )
{
	g_return_val_if_fail(data != NULL, TRUE);
//	g_object_set_data(G_OBJECT(data), "processing", NULL);
	gtk_widget_hide(widget);
	return TRUE;
}

static void mframe_cb(GtkAction *action, gpointer dialog)
{
	gpointer im_window;

	im_window = g_object_get_data(G_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);

	act_control_mband(NULL, im_window);
}

/* name, stock id, label, accel, tooltip, callback */
static GtkActionEntry reduce_menu_actions[] = {
	/* File */
	{ "file-menu",             NULL, "_File" },
	{ "file-add",              NULL, "_Add Files",              "<control>O", NULL, G_CALLBACK (imf_add_cb) },
	{ "file-remove-selected",  NULL, "Remove Selecte_d Files",  "<control>D", NULL, G_CALLBACK (imf_rm_cb) },
	{ "file-reload-selected",  NULL, "Reload Selected Files",   "<control>R", NULL, G_CALLBACK (imf_reload_cb) },
	{ "frame-display",         NULL, "_Display Frame",          "D",          NULL, G_CALLBACK (imf_display_cb) },
	{ "frame-next",            NULL, "_Next Frame",             "N",          NULL, G_CALLBACK (imf_next_cb) },
	{ "frame-prev",            NULL, "_Previous Frame",         "J",          NULL, G_CALLBACK (imf_prev_cb) },
	{ "frame-skip-selected",   NULL, "S_kip Selected Frames",   "K",          NULL, G_CALLBACK (imf_skip_cb) },
	{ "frame-unskip-selected", NULL, "_Unskip Selected Frames", "U",          NULL, G_CALLBACK (imf_unskip_cb) },

	/* Edit */
	{ "edit-menu",  NULL, "_Edit" },
	{ "select-all", NULL, "Select _All", "<control>A", NULL, G_CALLBACK (imf_selall_cb) },

	/* Reduce */
	{ "reduce-menu",          NULL, "_Reduce" },
	{ "reduce-all",           NULL, "Reduce All",                 "<shift>R",   NULL, G_CALLBACK (ccdred_run_cb) },
	{ "reduce-one",           NULL, "Reduce One Frame",           "Y",          NULL, G_CALLBACK (ccdred_one_cb) },
	{ "qphot-one",            NULL, "Qphot One Frame",            "T",          NULL, G_CALLBACK (ccdred_qphotone_cb) },
	{ "show-alignment-stars", NULL, "Show Alignment Stars",       "A",          NULL, G_CALLBACK (show_align_cb) },
	{ "phot-multi-frame",     NULL, "_Multi-frame Photometry...", "<control>M", NULL, G_CALLBACK (mframe_cb) },

};

/* create the menu bar */
static GtkWidget *get_main_menu_bar(GtkWidget *window, GtkWidget *notebook_page)
{
	GtkWidget *ret;
	GtkUIManager *ui;
	GError *error;
	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;

	static char *reduce_ui =
		"<menubar name='reduce-menubar'>"
		"  <menu name='file' action='file-menu'>"
		"    <menuitem name='Add Files' action='file-add'/>"
		"    <menuitem name='Remove Selected Files' action='file-remove-selected'/>"
		"    <menuitem name='Reload Selected Files' action='file-reload-selected'/>"
		"    <separator name='separator1'/>"
		"    <menuitem name='Display Frame' action='frame-display'/>"
		"    <menuitem name='Next Frame' action='frame-next'/>"
		"    <menuitem name='Previous Frame' action='frame-prev'/>"
		"    <separator name='separator2'/>"
		"    <menuitem name='Skip Selected Frames' action='frame-skip-selected'/>"
		"    <menuitem name='Unskip Selected Frames' action='frame-unskip-selected'/>"
		"  </menu>"
		"  <menu name='edit' action='edit-menu'>"
		"    <menuitem name='Select All' action='select-all'/>"
		"  </menu>"
		"  <menu name='Reduce' action='reduce-menu'>"
		"    <menuitem name='Reduce All' action='reduce-all'/>"
		"    <menuitem name='Reduce One Frame' action='reduce-one'/>"
		"    <menuitem name='Qphot One Frame' action='qphot-one'/>"
		"    <separator name='separator1'/>"
		"    <menuitem name='Show Alignment Stars' action='show-alignment-stars'/>"
		"    <separator name='separator2'/>"
		"    <menuitem name='Multi-frame Photometry...' action='phot-multi-frame'/>"
		"  </menu>"
		"</menubar>";

	action_group = gtk_action_group_new ("ReduceActions");
	gtk_action_group_add_actions (action_group, reduce_menu_actions,
				      G_N_ELEMENTS (reduce_menu_actions), window);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	error = NULL;
	gtk_ui_manager_add_ui_from_string (ui, reduce_ui, strlen(reduce_ui), &error);
	if (error) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	ret = gtk_ui_manager_get_widget (ui, "/reduce-menubar");

	/* save the accelerators on the page */
	accel_group = gtk_ui_manager_get_accel_group (ui);
	g_object_ref (accel_group);

	g_object_set_data_full (G_OBJECT(notebook_page), "reduce-accel-group", accel_group,
				(GDestroyNotify) g_object_unref);

	g_object_ref (ret);
	g_object_unref (ui);

	return ret;
}

static void reduce_switch_accels (GtkNotebook *notebook, GtkWidget *page, int page_num, gpointer window)
{
	GtkAccelGroup *accel_group;
	GtkWidget *opage;
	int i;

	/* remove accel groups created by us */
	for (i = 0; i < 2; i++) {
		opage = gtk_notebook_get_nth_page (notebook, i);

		accel_group = g_object_get_data (G_OBJECT(opage), "reduce-accel-group");
		g_return_if_fail (accel_group != NULL);

		/* seems we can't remove an accel group which isn't there. The list
		   returned by gtk_accel_groups_from_object should not be freed. */
		if (g_slist_index(gtk_accel_groups_from_object(G_OBJECT(window)), accel_group) != -1)
			gtk_window_remove_accel_group (GTK_WINDOW(window), accel_group);
	}

	/* workaround for gtk 2.20, where I don't know what's wrong with the page argument */
	page = gtk_notebook_get_nth_page (notebook, page_num);

	/* add current page accel group */
	accel_group = g_object_get_data (G_OBJECT(page), "reduce-accel-group");
	gtk_window_add_accel_group (GTK_WINDOW(window), accel_group);
}



/* create an image processing dialog and set the callbacks, but don't
 * show it */
static GtkWidget *make_image_processing(gpointer window)
{
	GtkWidget *dialog;
	GtkWidget *menubar, *top_hb;
	GtkNotebook *notebook;
	GtkWidget *page;
	GtkAccelGroup *accel_group;

	dialog = create_image_processing();
	g_object_set_data(G_OBJECT(dialog), "im_window", window);
	g_object_set_data_full(G_OBJECT(window), "processing",
				 dialog, (GDestroyNotify)(gtk_widget_destroy));
	g_signal_connect (G_OBJECT (dialog), "delete_event",
			    G_CALLBACK (close_processing_window), window);

	set_named_callback (G_OBJECT (dialog), "bias_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "dark_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "flat_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "badpix_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "align_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "output_file_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "recipe_browse", "clicked",
			    G_CALLBACK (imf_red_browse_cb));
	set_named_callback (G_OBJECT (dialog), "bias_entry", "activate",
			    G_CALLBACK (imf_red_activate_cb));
	set_named_callback (G_OBJECT (dialog), "dark_entry", "activate",
			    G_CALLBACK (imf_red_activate_cb));
	set_named_callback (G_OBJECT (dialog), "flat_entry", "activate",
			    G_CALLBACK (imf_red_activate_cb));
	set_named_callback (G_OBJECT (dialog), "badpix_entry", "activate",
			    G_CALLBACK (imf_red_activate_cb));
	set_named_callback (G_OBJECT (dialog), "align_entry", "activate",
			    G_CALLBACK (imf_red_activate_cb));
	set_named_callback (G_OBJECT (dialog), "recipe_entry", "activate",
			    G_CALLBACK (imf_red_activate_cb));

	/* each page should remember its accelerators, we need to switch them
	   (as they apply on the whole window) when switching pages */
	notebook = g_object_get_data (G_OBJECT(dialog), "notebook3");
	g_return_val_if_fail (notebook != NULL, NULL);

	page = gtk_notebook_get_nth_page (notebook, 0);
	menubar = get_main_menu_bar(dialog, page);

	/* activate reduce page accels for the dialog */
	accel_group = g_object_get_data(G_OBJECT(page), "reduce-accel-group");
	gtk_window_add_accel_group (GTK_WINDOW (dialog), accel_group);

	/* setup dummy accels for the setup page */
	page = gtk_notebook_get_nth_page (notebook, 1);
	accel_group = gtk_accel_group_new ();
	g_object_ref (accel_group);
	g_object_set_data_full (G_OBJECT(page), "reduce-accel-group", accel_group,
				(GDestroyNotify) g_object_unref);

	/* the ole switcheroo */
	g_signal_connect (G_OBJECT(notebook), "switch-page",
			  G_CALLBACK (reduce_switch_accels), dialog);


	g_object_set_data(G_OBJECT(dialog), "menubar", menubar);

	top_hb = g_object_get_data(G_OBJECT(dialog), "top_hbox");
	gtk_box_pack_start(GTK_BOX(top_hb), menubar, TRUE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(top_hb), menubar, 0);
	d3_printf("menubar: %p\n", menubar);
	gtk_widget_show(menubar);

	set_processing_dialog_ccdr(dialog, NULL);
	return dialog;
}

static void demosaic_method_activate(GtkComboBoxText *combo, gpointer dialog)
{
	char *str = gtk_combo_box_text_get_active_text (combo);
	int i;

	for (i = 0; demosaic_methods[i]; i++) {
		if (!strcmp(demosaic_methods[i], str)) {
			P_INT(CCDRED_DEMOSAIC_METHOD) = i;
			par_touch(CCDRED_DEMOSAIC_METHOD);
			g_free(str);
			return;
		}
	}

	g_free(str);
}


static void stack_method_activate(GtkComboBoxText *combo, gpointer dialog)
{
	char *str = gtk_combo_box_text_get_active_text (combo);
	int i;

	for (i = 0; stack_methods[i]; i++) {
		if (!strcmp(stack_methods[i], str)) {
			P_INT(CCDRED_STACK_METHOD) = i;
			par_touch(CCDRED_STACK_METHOD);
			g_free(str);
			return;
		}
	}

	g_free(str);
}

/* update the dialog to match the supplied ccdr */
/* in ccdr is null, just update the settings from the pars */
static void set_processing_dialog_ccdr(GtkWidget *dialog, struct ccd_reduce *ccdr)
{
	GtkComboBoxText *stack_combo, *demosaic_combo;
	GtkWidget *entry;

	stack_combo = g_object_get_data (G_OBJECT(dialog), "stack_method_combo");
	g_return_if_fail(stack_combo != NULL);

	entry = gtk_bin_get_child (GTK_BIN(stack_combo));
	gtk_entry_set_text (GTK_ENTRY(entry), stack_methods[P_INT(CCDRED_STACK_METHOD)]);

	g_signal_connect (G_OBJECT(stack_combo), "changed",
			  G_CALLBACK(stack_method_activate), dialog);


	named_spin_set(dialog, "overscan_spin", P_DBL(CCDRED_OVERSCAN_PEDESTAL));
	named_spin_set(dialog, "stack_sigmas_spin", P_DBL(CCDRED_SIGMAS));
	named_spin_set(dialog, "stack_iter_spin", 1.0 * P_INT(CCDRED_ITER));

	demosaic_combo = g_object_get_data (G_OBJECT(dialog), "demosaic_method_combo");
	g_return_if_fail(demosaic_combo != NULL);

	entry = gtk_bin_get_child (GTK_BIN(demosaic_combo));
	gtk_entry_set_text (GTK_ENTRY(entry), demosaic_methods[P_INT(CCDRED_DEMOSAIC_METHOD)]);

	g_signal_connect (G_OBJECT(demosaic_combo), "changed",
			  G_CALLBACK(demosaic_method_activate), dialog);

	if (ccdr == NULL)
		return;

	if (ccdr->ops & IMG_OP_OVERSCAN) {
		set_named_checkb_val(dialog, "overscan_checkb", 1);
	}

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
	GtkTreeIter iter;
	GtkListStore *list;
	GList *gl;
	struct image_file *imf;

	list = g_object_get_data(G_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	gtk_list_store_clear (list);

	gl = imfl->imlist;
	while (gl != NULL) {
		imf = gl->data;
		gl = g_list_next(gl);

		image_file_ref(imf);

		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    IMFL_COL_FILENAME, basename(imf->filename),
				    IMFL_COL_STATUS, "",
				    IMFL_COL_IMF, imf,
				    -1);

		imf_update_status_label (GTK_TREE_MODEL(list), &iter);
	}
}

void set_imfl_ccdr(gpointer window, struct ccd_reduce *ccdr,
		   struct image_file_list *imfl)
{
	GtkWidget *dialog;
	dialog = g_object_get_data(G_OBJECT(window), "processing");
	if (dialog == NULL) {
		dialog = make_image_processing(window);
	}
	g_return_if_fail(dialog != NULL);

	if (imfl) {
		image_file_list_ref(imfl);
		g_object_set_data_full(G_OBJECT(dialog), "imfl",
					 imfl, (GDestroyNotify)(image_file_list_release));
		set_processing_dialog_imfl(dialog, imfl);
	}
	if (ccdr) {
		ccd_reduce_ref(ccdr);
		g_object_set_data_full(G_OBJECT(dialog), "ccdred",
					 ccdr, (GDestroyNotify)(ccd_reduce_release));
		set_processing_dialog_ccdr(dialog, ccdr);
	}
}

/* mark selected files to be skipped */
static void imf_skip_cb(GtkAction *action, gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *sel, *tmp;
	struct image_file *imf;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	for (tmp = sel; tmp; tmp = tmp->next) {
		gtk_tree_model_get_iter (list, &iter, tmp->data);
		gtk_tree_model_get (list, &iter, IMFL_COL_IMF, (GValue *) &imf, -1);

		imf->flags |= IMG_SKIP;
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);

	update_selected_status_label(dialog);
}

/* remove skip marks from selected files */
static void imf_unskip_cb(GtkAction *action, gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *sel, *tmp;
	struct image_file *imf;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	for (tmp = sel; tmp; tmp = tmp->next) {
		gtk_tree_model_get_iter (list, &iter, tmp->data);
		gtk_tree_model_get (list, &iter, IMFL_COL_IMF, (GValue *) &imf, -1);

		imf->flags &= ~IMG_SKIP;
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);

	update_selected_status_label(dialog);
}

/* remove selected files */
static void imf_rm_cb(GtkAction *action, gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *sel, *tmp;
	struct image_file *imf;
	struct image_file_list *imfl;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	imfl = g_object_get_data (G_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);

	selection = gtk_tree_view_get_selection (view);

	/* returns a list of GtkTreePaths, which ... */
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	/* ... must be converted to GtkTreeRowReferences,
	   so we can change the underlying model by ... */
	for (tmp = sel; tmp; tmp = tmp->next) {
		path = tmp->data;
		tmp->data = gtk_tree_row_reference_new (list, path);
		gtk_tree_path_free (path);
	}

	/* ... converting them back into GtkTreePaths, out of which
	   we can get the GtkTreeIters which we can actualy
	   remove from the GtkListStore */
	for (tmp = sel; tmp; tmp = tmp->next) {
		path = gtk_tree_row_reference_get_path (tmp->data);
		gtk_tree_model_get_iter (list, &iter, path);

		gtk_tree_model_get (list, &iter, IMFL_COL_IMF, (GValue *) &imf, -1);
		d3_printf("removing %s\n", imf->filename);

		imfl->imlist = g_list_remove(imfl->imlist, imf);
		image_file_release(imf);

		gtk_list_store_remove (GTK_LIST_STORE(list), &iter);
	}

	g_list_foreach (sel, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (sel);
}

/* select and display next frame in list */
static void imf_next_cb(GtkAction *action, gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *sel;
	int *indices;
	int index, len;
	GtkWidget *scw;
	GtkAdjustment *vadj;
	double nv;
	gdouble value, lower, upper, page_size;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel == NULL) {
		if (gtk_tree_model_get_iter_first (list, &iter)) {
			path = gtk_tree_model_get_path (list, &iter);

			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);

			gtk_tree_path_free(path);
		}

		imf_display_cb (NULL, dialog);
		return;
	}


	path = sel->data;

        /* the array should not be freed */
	indices = gtk_tree_path_get_indices (path);
	index = indices[0];

	len = gtk_tree_model_iter_n_children (list, NULL);

	/* unlike gtk_tree_path_prev, there is no 'if exists' choice for _next */
	if (index + 1 < len) {
		gtk_tree_path_next (path);

		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, path);

		imf_display_cb (NULL, dialog);
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);


	d3_printf("initial position is %d\n", index);

	scw = g_object_get_data(G_OBJECT(dialog), "scrolledwindow");
	g_return_if_fail(scw != NULL);

	vadj =  gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scw));
	g_object_get (G_OBJECT(vadj), "value", &value,
		      "lower", &lower, "upper", &upper,
		      "page-size", &page_size, NULL);

	d3_printf("vadj at %.3f\n", value);

	if (len != 0) {
		nv = (upper + lower) * index / len - page_size / 2;
		clamp_double(&nv, lower, upper - page_size);
		gtk_adjustment_set_value(vadj, nv);
		d3_printf("vadj set to %.3f\n", nv);
	}

	update_selected_status_label (dialog);
}


/* select and display previous frame in list */
static void imf_prev_cb(GtkAction *action, gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *sel;
	int *indices;
	int index, len;
	GtkWidget *scw;
	GtkAdjustment *vadj;
	double nv;
	gdouble value, lower, upper, page_size;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel == NULL) {
		if (gtk_tree_model_get_iter_first (list, &iter)) {
			path = gtk_tree_model_get_path (list, &iter);

			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);

			gtk_tree_path_free(path);
		}

		imf_display_cb (NULL, dialog);
		return;
	}

	path = sel->data;

        /* the array should not be freed */
	indices = gtk_tree_path_get_indices (path);
	index = indices[0];

	len = gtk_tree_model_iter_n_children (list, NULL);

	if (gtk_tree_path_prev (path)) {

		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, path);

		imf_display_cb (NULL, dialog);
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);

	d3_printf("initial position is %d\n", index);

	scw = g_object_get_data(G_OBJECT(dialog), "scrolledwindow");
	g_return_if_fail(scw != NULL);

	vadj =  gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scw));
	g_object_get (G_OBJECT(vadj), "value", &value,
		      "lower", &lower, "upper", &upper,
		      "page-size", &page_size, NULL);

	d3_printf("vadj at %.3f\n", value);

	if (len != 0) {
		nv = (upper + lower) * index / len - page_size / 2;
		clamp_double(&nv, lower, upper - page_size);
		gtk_adjustment_set_value(vadj, nv);
		d3_printf("vadj set to %.3f\n", nv);
	}

	update_selected_status_label (dialog);
}


static void imf_display_cb(GtkAction *action, gpointer dialog)
{
	GtkWidget *im_window;
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GList *sel;
	struct image_file *imf;
	struct image_file_list *imfl;

	d3_printf("display\n");

	im_window = g_object_get_data(G_OBJECT(dialog), "im_window");
	g_return_if_fail (im_window != NULL);

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel != NULL) {
		gtk_tree_model_get_iter (list, &iter, sel->data);
		gtk_tree_model_get (list, &iter, IMFL_COL_IMF, &imf, -1);

		if (imf == NULL) {
			err_printf("NULL imf\n");
			goto out;
		}

		if (load_image_file(imf))
			goto out;

		frame_to_window (imf->fr, im_window);
	} else {
		error_beep();
		log_msg("\nNo Frame selected\n", dialog);
	}

	if (P_INT(FILE_SAVE_MEM)) {
		imfl = g_object_get_data(G_OBJECT(dialog), "imfl");
		if (imfl)
			unload_clean_frames(imfl);
	}

	update_selected_status_label(dialog);

out:
	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);

	return;
}

/* select all files */
static void imf_selall_cb(GtkAction *action, gpointer dialog)
{
	GtkTreeView *view;
	GtkTreeSelection *selection;

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_select_all (selection);
}

static void imf_add_files(GSList *files, gpointer dialog)
{
	GtkListStore *list;
	GtkTreeIter iter;
	struct image_file *imf;
	struct image_file_list *imfl;
	char *text;

	d4_printf("imf_add_files, dialog %p\n", dialog);
	list = g_object_get_data(G_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	imfl = g_object_get_data(G_OBJECT(dialog), "imfl");
	if (imfl == NULL) {
		imfl = image_file_list_new();
		g_object_set_data_full (G_OBJECT(dialog), "imfl", imfl,
					  (GDestroyNotify) (image_file_list_release));
	}

	while (files != NULL) {
		imf = image_file_new();
		text = files->data;
		files = g_slist_next(files);
		imf->filename = strdup(text);
		image_file_ref(imf);

		imfl->imlist = g_list_append(imfl->imlist, imf);

		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    IMFL_COL_FILENAME, basename(imf->filename),
				    IMFL_COL_STATUS, "",
				    IMFL_COL_IMF, imf,
				    -1);

		imf_update_status_label (GTK_TREE_MODEL(list), &iter);

		d3_printf("adding %s\n", text);
	}
}

static void imf_add_cb(GtkAction *action, gpointer dialog)
{
	file_select_list(dialog, "Select files", "*.fits", imf_add_files);
}

/* reload selected files */
static void imf_reload_cb(GtkAction *action, gpointer dialog)
{

	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *sel, *tmp;
	struct image_file *imf;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	for (tmp = sel; tmp; tmp = tmp->next) {
		gtk_tree_model_get_iter (list, &iter, tmp->data);
		gtk_tree_model_get (list, &iter, IMFL_COL_IMF, (GValue *) &imf, -1);

		d3_printf("unloading %s\n", imf->filename);

		if ((imf->flags & IMG_LOADED) && imf->fr) {
			release_frame (imf->fr);
			imf->fr = NULL;
		}

		imf->flags &= IMG_SKIP; /* we keep the skip flag */
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);

	update_selected_status_label(dialog);
}

static void switch_frame_cb(gpointer window, guint action)
{
	GtkWidget *dialog;
	dialog = g_object_get_data(G_OBJECT(window), "processing");
	if (dialog == NULL) {
		dialog = make_image_processing(window);
	}
	g_return_if_fail(dialog != NULL);

	switch(action) {
	case SWF_NEXT:
		imf_next_cb(NULL, dialog);
		break;
	case SWF_SKIP:
		imf_skip_cb(NULL, dialog);
		break;
	case SWF_PREV:
		imf_prev_cb(NULL, dialog);
		break;
	case SWF_QPHOT:
		ccdred_qphotone_cb(NULL, dialog);
		break;
	case SWF_RED:
		ccdred_one_cb(NULL, dialog);
		break;
	}
	update_selected_status_label(dialog);
}

void act_process_next (GtkAction *action, gpointer window)
{
	switch_frame_cb (window, SWF_NEXT);
}

void act_process_prev (GtkAction *action, gpointer window)
{
	switch_frame_cb (window, SWF_PREV);
}

void act_process_skip (GtkAction *action, gpointer window)
{
	switch_frame_cb (window, SWF_SKIP);
}

void act_process_qphot (GtkAction *action, gpointer window)
{
	switch_frame_cb (window, SWF_QPHOT);
}

void act_process_reduce (GtkAction *action, gpointer window)
{
	switch_frame_cb (window, SWF_RED);
}

static void imf_update_status_label(GtkTreeModel *list, GtkTreeIter *iter)
{
	char msg[128];
	int i = 0;
	struct image_file *imf;

	gtk_tree_model_get (list, iter, IMFL_COL_IMF, (GValue *) &imf, -1);
	g_return_if_fail(imf != NULL);

	msg[0] = 0;

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

	d3_printf("updating status to %s\n", msg);
	gtk_list_store_set (GTK_LIST_STORE(list), iter, IMFL_COL_STATUS, msg, -1);
}

static void update_selected_status_label(gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *sel, *tmp;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	for (tmp = sel; tmp; tmp = tmp->next) {
		gtk_tree_model_get_iter (list, &iter, tmp->data);
		imf_update_status_label (list, &iter);
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);
}

static void update_status_labels(gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeIter iter;
	gboolean valid;

	list = g_object_get_data(G_OBJECT(dialog), "image_file_list");
	g_return_if_fail(list != NULL);

	valid = gtk_tree_model_get_iter_first (list, &iter);
	while (valid) {
		imf_update_status_label (list, &iter);

		valid = gtk_tree_model_iter_next (list, &iter);
	}
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
	if (wid == g_object_get_data(G_OBJECT(dialog), "bias_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "bias_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select bias frame", "*", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "dark_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "dark_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select dark frame", "*", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "flat_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "flat_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select flat frame", "*", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "badpix_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "badpix_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select bad pixel file", "*", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "align_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "align_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select alignment reference frame", "*", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "recipe_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "recipe_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select recipe file", "*", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "output_file_browse")) {
		entry = g_object_get_data(G_OBJECT(dialog), "output_file_entry");
		g_return_if_fail(entry != NULL);
		file_select_to_entry(dialog, entry, "Select output file/dir", "*", 1);
	}
}

static int log_msg(char *msg, void *dialog)
{
	GtkWidget *text;

	text = g_object_get_data(G_OBJECT(dialog), "processing_log_text");
	g_return_val_if_fail(text != NULL, 0);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);
	gtk_text_buffer_insert_at_cursor(
		gtk_text_view_get_buffer (GTK_TEXT_VIEW(text)),
		msg, -1);

	return 0;
}

static int progress_pr(char *msg, void *dialog)
{
	log_msg (msg, dialog);

	while (gtk_events_pending())
		gtk_main_iteration();

	return 0;
}


static void dialog_to_ccdr(GtkWidget *dialog, struct ccd_reduce *ccdr)
{
	char *text;

	g_return_if_fail (ccdr != NULL);

	if (get_named_checkb_val(dialog, "overscan_checkb")) {
		ccdr->overscan = named_spin_get_value(dialog, "overscan_spin");
		ccdr->ops |= IMG_OP_OVERSCAN;
	} else {
		ccdr->ops &= ~IMG_OP_OVERSCAN;
	}

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
		window = g_object_get_data(G_OBJECT(dialog), "im_window");
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
			wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
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

static void ccdred_run_cb(GtkAction *action, gpointer dialog)
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


	imfl = g_object_get_data(G_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);
	ccdr = g_object_get_data(G_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		g_object_set_data_full(G_OBJECT(dialog), "ccdred",
					 ccdr, (GDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	ccdr->ops &= ~CCDR_BG_VAL_SET;
	dialog_to_ccdr(dialog, ccdr);

	outf = named_entry_text(dialog, "output_file_entry");
	d3_printf("outf is |%s|\n", outf);

	menubar = g_object_get_data(G_OBJECT(dialog), "menubar");
	gtk_widget_set_sensitive(menubar, 0);

	if (ccdr->ops & IMG_OP_PHOT) {
		if (ccdr->multiband == NULL) {
			gpointer mbd;
			im_window = g_object_get_data(G_OBJECT(dialog), "im_window");
			mbd = g_object_get_data(G_OBJECT(im_window), "mband_window");
			if (mbd == NULL) {
				act_control_mband(NULL, im_window);
				mbd = g_object_get_data(G_OBJECT(im_window), "mband_window");
			}
			g_object_ref(GTK_WIDGET(mbd));
			ccdr->multiband = mbd;
			g_object_set_data_full(G_OBJECT(dialog), "mband_window",
						 mbd, (GDestroyNotify)(g_object_unref));
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
	im_window = g_object_get_data(G_OBJECT(dialog), "im_window");
	g_return_if_fail(im_window != NULL);

	frame_to_window (fr, im_window);
	release_frame(fr);
end:
	gtk_widget_set_sensitive(menubar, 1);
	return;


}


static struct image_file * current_imf(gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *sel;
	struct image_file *imf = NULL;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_val_if_fail (list != NULL, NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_val_if_fail (view != NULL, NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel == NULL) {
		if (gtk_tree_model_get_iter_first (list, &iter)) {
			gtk_tree_model_get (list, &iter, IMFL_COL_IMF, (GValue *) &imf, -1);

			path = gtk_tree_model_get_path (list, &iter);

			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);

			gtk_tree_path_free(path);
		}

		return imf;
	}

	gtk_tree_model_get_iter (list, &iter, sel->data);
	gtk_tree_model_get (list, &iter, IMFL_COL_IMF, (GValue *) &imf, -1);

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);

	return imf;
}

static void select_next_imf(gpointer dialog)
{
	GtkTreeModel *list;
	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *sel;
	int *indices;
	int index, len;
	GtkWidget *scw;
	GtkAdjustment *vadj;
	double nv;
	gdouble value, lower, upper, page_size;

	list = g_object_get_data (G_OBJECT(dialog), "image_file_list");
	g_return_if_fail (list != NULL);

	view = g_object_get_data (G_OBJECT(dialog), "image_file_view");
	g_return_if_fail (view != NULL);

	selection = gtk_tree_view_get_selection (view);
	sel = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (sel == NULL) {
		if (gtk_tree_model_get_iter_first (list, &iter)) {
			path = gtk_tree_model_get_path (list, &iter);

			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);

			gtk_tree_path_free(path);
		}

		update_selected_status_label (dialog);
		return;
	}


	path = sel->data;

        /* the array should not be freed */
	indices = gtk_tree_path_get_indices (path);
	index = indices[0];

	len = gtk_tree_model_iter_n_children (list, NULL);

	/* unlike gtk_tree_path_prev, there is no 'if exists' choice for _next */
	if (index + 1 < len) {
		gtk_tree_path_next (path);

		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, path);
	}

	g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (sel);


	d3_printf("initial position is %d\n", index);

	scw = g_object_get_data(G_OBJECT(dialog), "scrolledwindow");
	g_return_if_fail(scw != NULL);

	vadj =  gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scw));
	g_object_get (G_OBJECT(vadj), "value", &value,
		      "lower", &lower, "upper", &upper,
		      "page-size", &page_size, NULL);

	d3_printf("vadj at %.3f\n", value);

	if (len != 0) {
		nv = (upper + lower) * index / len - page_size / 2;
		clamp_double(&nv, lower, upper - page_size);
		gtk_adjustment_set_value(vadj, nv);
		d3_printf("vadj set to %.3f\n", nv);
	}

	update_selected_status_label (dialog);
}


static void ccdred_one_cb(GtkAction *action, gpointer dialog)
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

	imfl = g_object_get_data(G_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);
	ccdr = g_object_get_data(G_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		g_object_set_data_full(G_OBJECT(dialog), "ccdred",
					 ccdr, (GDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	ccdr->ops &= ~CCDR_BG_VAL_SET;
	dialog_to_ccdr(dialog, ccdr);

	outf = named_entry_text(dialog, "output_file_entry");
	d3_printf("outf is |%s|\n", outf);

	menubar = g_object_get_data(G_OBJECT(dialog), "menubar");
	gtk_widget_set_sensitive(menubar, 0);

	if (ccdr->ops & IMG_OP_PHOT) {
		if (ccdr->multiband == NULL) {
			gpointer mbd;
			im_window = g_object_get_data(G_OBJECT(dialog), "im_window");
			mbd = g_object_get_data(G_OBJECT(im_window), "mband_window");
			if (mbd == NULL) {
				act_control_mband(NULL, im_window);
				mbd = g_object_get_data(G_OBJECT(im_window), "mband_window");
			}
			g_object_ref(GTK_WIDGET(mbd));
			ccdr->multiband = mbd;
			g_object_set_data_full(G_OBJECT(dialog), "mband_window",
						 mbd, (GDestroyNotify)(g_object_unref));
		}
	}

	ret = reduce_frame(imf, ccdr, progress_pr, dialog);
	if ((ccdr->ops & IMG_OP_PHOT) == 0) {
		imf_display_cb(NULL, dialog);
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
		imf_display_cb(NULL, dialog);
	}
	gtk_widget_set_sensitive(menubar, 1);
}

/* run quick phot on one frame */
static void ccdred_qphotone_cb(GtkAction *action, gpointer dialog)
{
	struct ccd_reduce *ccdr;
	struct image_file_list *imfl;
	struct image_file *imf;
	char *outf;
	GtkWidget *menubar;


	imf = current_imf(dialog);
	if (imf == NULL || imf->flags & IMG_SKIP) {
		select_next_imf(dialog);
		return;
	}

	imfl = g_object_get_data(G_OBJECT(dialog), "imfl");
	g_return_if_fail (imfl != NULL);
	ccdr = g_object_get_data(G_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		g_object_set_data_full(G_OBJECT(dialog), "ccdred",
					 ccdr, (GDestroyNotify)(ccd_reduce_release));
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

	menubar = g_object_get_data(G_OBJECT(dialog), "menubar");
	gtk_widget_set_sensitive(menubar, 0);

	ccdr->ops |= IMG_QUICKPHOT;
	reduce_frame(imf, ccdr, progress_pr, dialog);
	if ((ccdr->ops & IMG_OP_PHOT) == 0) {
		imf_display_cb(NULL, dialog);
	}
	ccdr->ops &= ~IMG_QUICKPHOT;
	imf->flags &= ~IMG_OP_PHOT;
	update_selected_status_label(dialog);
	gtk_widget_set_sensitive(menubar, 1);
}


static void show_align_cb(GtkAction *action, gpointer dialog)
{
	struct ccd_reduce *ccdr;
	GtkWidget *im_window;

	ccdr = g_object_get_data(G_OBJECT(dialog), "ccdred");
	if (ccdr == NULL) {
		ccdr = ccd_reduce_new();
		g_object_set_data_full(G_OBJECT(dialog), "ccdred",
					 ccdr, (GDestroyNotify)(ccd_reduce_release));
	}
	g_return_if_fail (ccdr != NULL);
	dialog_to_ccdr(dialog, ccdr);

	im_window = g_object_get_data(G_OBJECT(dialog), "im_window");
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
	if (wid == g_object_get_data(G_OBJECT(dialog), "dark_entry")) {
		set_named_checkb_val(dialog, "dark_checkb", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "flat_entry")) {
		set_named_checkb_val(dialog, "flat_checkb", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "bias_entry")) {
		set_named_checkb_val(dialog, "bias_checkb", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "badpix_entry")) {
		set_named_checkb_val(dialog, "badpix_checkb", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "align_entry")) {
		set_named_checkb_val(dialog, "align_checkb", 1);
	}
	if (wid == g_object_get_data(G_OBJECT(dialog), "recipe_entry")) {
		set_named_checkb_val(dialog, "phot_en_checkb", 1);
	}

}




void act_control_processing (GtkAction *action, gpointer window)
{
	GtkWidget *dialog;

	dialog = g_object_get_data(G_OBJECT(window), "processing");
	if (dialog == NULL) {
		dialog = make_image_processing(window);
		gtk_widget_show_all(dialog);
	} else {
		gtk_window_present (GTK_WINDOW (dialog));
	}
}
