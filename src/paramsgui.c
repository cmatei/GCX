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

/* gui functions related to par tree editing */
/* hopefully, all the mess is contained in this file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "gcx.h"
#include "gcx-imageview.h"
#include "params.h"
#include "catalogs.h"
#include "sourcesdraw.h"
#include "interface.h"
#include "misc.h"
#include "gui.h"



static void par_update_current(gpointer parwin);






/* update the item's value in the entry/optionmenu/checkbox
 */


static void par_item_restore_default_gui(GtkTreeView *tree, GcxPar p)
{
	if (PAR(p)->flags & (PAR_TREE)) { // recurse tree
		GcxPar pp;
		PAR(p)->flags &= ~PAR_USER;
		update_tree_label(tree, p);
		pp = PAR(p)->child;
		while(pp != PAR_NULL) {
			par_item_restore_default_gui(tree, pp);
			pp = PAR(pp)->next;
		}
	} else if (PAR(p)->flags & (PAR_USER)) {
			if (PAR_TYPE(p) == PAR_STRING) {
				change_par_string(p, PAR(p)->defval.s);
			} else {
				memcpy(&(PAR(p)->val), &(PAR(p)->defval), sizeof(union pval));
			}
//			par_item_update_value(p);
//			d3_printf("defaulting par %d\n", p);
			PAR(p)->flags &= ~PAR_USER;
			PAR(p)->flags &= ~PAR_FROM_RC;
			PAR(p)->flags &= ~PAR_TO_SAVE;
			par_item_update_status(tree, p);
	}
}

static void update_ancestors_state_gui(GtkTreeView *tree, GcxPar p)
{
	p = PAR(p)->parent;
	while (p != PAR_NULL && PAR_TYPE(p) == PAR_TREE) {
		int type = 0;
		PAR(p)->flags &= ~(PAR_USER | PAR_FROM_RC);
		type = or_child_type(p, 0);
		PAR(p)->flags |= (type & (PAR_USER | PAR_FROM_RC));
		update_tree_label(tree, p);
		p = PAR(p)->parent;
	}
}

static void restore_defaults_one_par(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	GtkTreeView *tree = GTK_TREE_VIEW(data);
	GcxPar p;

	gtk_tree_model_get (model, iter, 1, &p, -1);

	par_item_restore_default_gui(tree, p);
	update_ancestors_state_gui(tree, p);
}

static void restore_defaults_cb(GtkWidget *widget, gpointer parwin )
{
	GtkTreeView *tree;
	GtkTreeSelection *selection;

	tree = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(parwin), "par_tree"));
	g_return_if_fail(tree != NULL);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_selected_foreach(selection, restore_defaults_one_par, tree);

	par_update_current(parwin);
}



/* new par editing callbacks */


static void par_edit_changed(GtkEditable *editable, gpointer parwin)
{
	GtkTreeView *tree;
	GcxPar p;

	d4_printf("changed\n");
	p = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parwin), "selpar"));
	tree = g_object_get_data(G_OBJECT(parwin), "par_tree");

	param_edited(tree, p, editable);
	update_par_labels(p, parwin);
}



static void par_load_cb( GtkWidget *widget, gpointer parwin )
{
	GtkTreeView *tree;

	tree = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(parwin), "par_tree"));
	g_return_if_fail(tree != NULL);

	/* FIXME: NULL arg should be gcx instance */
	if (! gcx_load_rcfile(NULL, NULL))
		par_update_items(tree, PAR_FIRST);

	par_update_current(parwin);
}

static void close_par_dialog(GtkWidget *widget, gpointer parwin)
{
	gpointer window;
	window = g_object_get_data(G_OBJECT(parwin), "im_window");
	g_object_set_data(G_OBJECT(window), "params_window", NULL);
}
