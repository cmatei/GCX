/*******************************************************************************
  Copyright(c) 2015 Matei Conovici. All rights reserved.

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

  Contact Information: mconovici@gmail.com
*******************************************************************************/

/* dialog for editing gcx parameters */

#include <gtk/gtk.h>

#include "gcx.h"
#include "params.h"
#include "gcxparamsdialog.h"

struct _GcxParamsDialogPrivate {

	GtkWidget *title_label;
	GtkWidget *desc_label;
	GtkWidget *type_label;
	GtkWidget *value_comboboxtext;
	GtkWidget *value_entry;
	GtkWidget *status_label;
	GtkWidget *fname_label;

	/* Par tree */
	GtkTreeView      *par_treeview;
	GtkTreeModel     *par_treemodel;
	GtkTreeSelection *par_treeselection;


	/* the selected par */
	gboolean    par_selected;
	GtkTreeIter selected_par_iter;
};

enum {
	PAR_COLUMN_LABEL = 0,
	PAR_COLUMN_PARID,
	PAR_COLUMN_CHANGED,
};

G_DEFINE_TYPE_WITH_PRIVATE (GcxParamsDialog, gcx_params_dialog, GTK_TYPE_DIALOG);


/* helpers */
static void     par_tree_model_fill  (GtkTreeModel *tree);

static inline GcxPar   par_tree_iter_get_par     (GtkTreeModel *tree, GtkTreeIter *iter);
static inline void     par_tree_iter_set_par     (GtkTreeModel *tree, GtkTreeIter *iter, GcxPar p);

static inline void     par_tree_iter_set_changed (GtkTreeModel *tree, GtkTreeIter *iter, gboolean state);
static inline gboolean par_tree_iter_get_changed (GtkTreeModel *tree, GtkTreeIter *iter);

static gboolean par_tree_get_selected (GcxParamsDialog *dialog, GcxPar *p);

/* callbacks */
static void par_selection_changed_cb (GtkTreeSelection *select, gpointer user);
static void par_entry_changed_cb     (GtkEditable *editable, gpointer user);

static void par_save_clicked_cb      (GtkButton *button, gpointer user);
static void par_default_clicked_cb   (GtkButton *button, gpointer user);
static void par_reload_clicked_cb    (GtkButton *button, gpointer user);
static void par_close_clicked_cb     (GtkButton *button, gpointer user);

static void
gcx_params_dialog_init (GcxParamsDialog *self)
{
	GcxParamsDialogPrivate *priv;
	GtkTreeViewColumn *column;

	priv = self->priv = gcx_params_dialog_get_instance_private (self);

	gtk_widget_init_template (GTK_WIDGET (self));

	par_tree_model_fill (priv->par_treemodel);

	/* Add the columns to the tree view */
	column = gtk_tree_view_column_new_with_attributes
		("Option", gtk_cell_renderer_text_new (), "text", PAR_COLUMN_LABEL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->par_treeview), column);

	column = gtk_tree_view_column_new_with_attributes
		("Changes", gtk_cell_renderer_text_new (), "text", PAR_COLUMN_CHANGED, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->par_treeview), column);
}

static void
gcx_params_dialog_class_init (GcxParamsDialogClass *class)
{
	gtk_widget_class_set_template_from_resource
		(GTK_WIDGET_CLASS (class), "/org/gcx/ui/paramsdialog.ui");

	/* widgets */
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, par_treemodel);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, par_treeview);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, par_treeselection);

	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, title_label);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, desc_label);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, type_label);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, value_comboboxtext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, value_entry);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, status_label);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxParamsDialog, fname_label);

	/* callbacks */
	gtk_widget_class_bind_template_callback
		(GTK_WIDGET_CLASS (class), par_selection_changed_cb);
	gtk_widget_class_bind_template_callback
		(GTK_WIDGET_CLASS (class), par_entry_changed_cb);

	gtk_widget_class_bind_template_callback
		(GTK_WIDGET_CLASS (class), par_save_clicked_cb);
	gtk_widget_class_bind_template_callback
		(GTK_WIDGET_CLASS (class), par_default_clicked_cb);
	gtk_widget_class_bind_template_callback
		(GTK_WIDGET_CLASS (class), par_reload_clicked_cb);
	gtk_widget_class_bind_template_callback
		(GTK_WIDGET_CLASS (class), par_close_clicked_cb);
}


GtkWidget *
gcx_params_dialog_new (const gchar *title,
		       GtkWindow   *parent)
{
	GtkWidget *result;

	result = g_object_new (GCX_TYPE_PARAMS_DIALOG,
			       "transient-for", parent,
			       "title", title ? title : _("Gcx Options"),
			       NULL);
	return result;
}


static GcxPar
par_tree_iter_get_par (GtkTreeModel *tree, GtkTreeIter *iter)
{
	GcxPar p;
	gtk_tree_model_get (tree, iter, PAR_COLUMN_PARID, &p, -1);
	return p;
}

static void
par_tree_iter_set_par (GtkTreeModel *tree, GtkTreeIter *iter, GcxPar p)
{
	gtk_tree_store_set (GTK_TREE_STORE (tree), iter, PAR_COLUMN_PARID, p, -1);
}

static gboolean
par_tree_iter_get_changed (GtkTreeModel *tree, GtkTreeIter *iter)
{
	const char *text;
	gtk_tree_model_get (tree, iter, PAR_COLUMN_CHANGED, &text, -1);
	return (!strcmp(text, "*"));
}

static void
par_tree_iter_set_changed (GtkTreeModel *tree, GtkTreeIter *iter, gboolean state)
{
	gtk_tree_store_set (GTK_TREE_STORE (tree), iter, PAR_COLUMN_CHANGED, state ? "*" : "", -1);
}

static gboolean
par_tree_get_selected (GcxParamsDialog *dialog, GcxPar *p)
{
	GcxParamsDialogPrivate *priv = dialog->priv;

	if (!priv->par_selected)
		return FALSE;

	*p = par_tree_iter_get_par (priv->par_treemodel, &priv->selected_par_iter);

	return TRUE;
}

/* Recursively builds the par tree */
static void
make_tree (GcxPar p, GtkTreeIter *parent, GtkTreeModel *tree)
{
	GtkTreeIter iter;

	gtk_tree_store_append (GTK_TREE_STORE (tree), &iter, parent);
	gtk_tree_store_set (GTK_TREE_STORE (tree), &iter,
			    PAR_COLUMN_LABEL,   PAR(p)->comment,
			    PAR_COLUMN_PARID,   p,
			    PAR_COLUMN_CHANGED, (PAR(p)->flags & PAR_USER) ? "*" : "",
			    -1);

	if (PAR_TYPE(p) == PAR_TREE) {
		p = PAR(p)->child;
		while (p != PAR_NULL) {
			make_tree (p, &iter, tree);
			p = PAR(p)->next;
		}
	}
}

/* Fills in the GtkTreeModel that represents the par tree */
static void
par_tree_model_fill (GtkTreeModel *tree)
{
	GcxPar p;

	for (p = PAR_FIRST; p != PAR_NULL; p = PAR(p)->next) {
		make_tree(p, NULL, tree);
	}
}

/* return a string describing the current parameter type. */
static const char *
par_type_string(GcxPar p)
{
	switch (PAR_TYPE(p)) {
	case PAR_INTEGER:
		if (PAR_FORMAT(p) == FMT_BOOL) {
			return "Type: Truth value";
		} else if (PAR_FORMAT(p) == FMT_OPTION) {
			return "Type: Multiple choices";
		}
		return "Type: Integer";
	case PAR_STRING:
		return "Type: String";
	case PAR_DOUBLE:
		return "Type: Real Number";
	case PAR_TREE:
		return "Type: Subtree";
	}

	return "Type: BAD";
}

/* return a string describing the current parameter status. */
static const char *
par_status_string(GcxPar p)
{
	if ((p == PAR_NULL) || (PAR_TYPE(p) == PAR_TREE))
		return NULL;

	if (PAR(p)->flags & PAR_USER) {
		return "User Set";
	} else if (PAR(p)->flags & PAR_FROM_RC) {
		return "From File";
	} else {
		return "Default";
	}
}

/* refresh the labels of the selected par in the editing dialog */
static void
par_update_current (GcxParamsDialog *dialog)
{
	GcxParamsDialogPrivate *priv = dialog->priv;
	GcxPar p;
	gboolean editable = TRUE;
	char **c;
	char buf[256];

	if (!par_tree_get_selected(dialog, &p)) {
		d3_printf("No par selected.\n");
		return;
	}

	d4_printf("setup_par_labels, par %d\n", p);

	gtk_label_set_text (GTK_LABEL (priv->title_label),  PAR(p)->comment);
	gtk_label_set_text (GTK_LABEL (priv->desc_label),   PAR(p)->description);
	gtk_label_set_text (GTK_LABEL (priv->type_label),   par_type_string (p));
	gtk_label_set_text (GTK_LABEL (priv->status_label), par_status_string (p));

	strcpy(buf, "Config file: ");
	par_pathname(p, buf+13, 255-13);
	gtk_label_set_text (GTK_LABEL (priv->fname_label),  buf);

	/* setup combo for editing */
	gtk_widget_hide (priv->value_comboboxtext);
	if (PAR_TYPE(p) != PAR_TREE) {
		g_signal_handlers_block_by_func
			(G_OBJECT(priv->value_entry), par_entry_changed_cb, dialog);

		/* FIXME: this looks like a gtk bug. The combo will display the drop arrow even if
		   empty, if it never had any text added to it. Add some 'just in case' then
		   remove all */
		gtk_combo_box_text_append_text
			(GTK_COMBO_BOX_TEXT (priv->value_comboboxtext), "DISCARD_HACK");
		gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (priv->value_comboboxtext));

		editable = TRUE;
		if (PAR_FORMAT(p) == FMT_BOOL) {
			editable = FALSE;

			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->value_comboboxtext), "yes");
			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->value_comboboxtext), "no");
		} else if ((PAR_FORMAT(p) == FMT_OPTION) && PAR(p)->choices != NULL) {
			editable = FALSE;

			c = PAR(p)->choices;
			while (*c != NULL) {
				gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (priv->value_comboboxtext), *c);
				c++;
			}
		}
		gtk_editable_set_editable(GTK_EDITABLE(priv->value_entry), editable);

		make_value_string (p, buf, 255);
		gtk_entry_set_text (GTK_ENTRY (priv->value_entry), buf);

		d4_printf("setting val to %s\n", buf);

		gtk_widget_show (priv->value_comboboxtext);

		g_signal_handlers_unblock_by_func
			(G_OBJECT(priv->value_entry), par_entry_changed_cb, dialog);
	}
}

/* update the status of the current entry */
static void
par_update_current_status (GcxParamsDialog *dialog)
{
	GcxParamsDialogPrivate *priv = dialog->priv;
	GcxPar p;

	if (!par_tree_get_selected (dialog, &p)) {
		d3_printf ("No par selected.\n");
		return;
	}

	gtk_label_set_text (GTK_LABEL (priv->status_label), par_status_string (p));
}

/* walks the par tree and updates the 'changed' column of the tree elements */
static void
par_tree_update_changes (GtkTreeModel *tree, GtkTreeIter iter)
{
	GtkTreeIter child;
	GcxPar p;
	gboolean changed;

	do {
		if (gtk_tree_model_iter_children (tree, &child, &iter)) {
			par_tree_update_changes (tree, child);

			p = par_tree_iter_get_par (tree, &iter);
			changed = (PAR(p)->flags & PAR_USER) ? TRUE : FALSE;
			par_tree_iter_set_changed (tree, &iter, changed);
		} else {
			p = par_tree_iter_get_par (tree, &iter);
			changed = (PAR(p)->flags & PAR_USER) ? TRUE : FALSE;

			d3_printf("updating leaf %s, %schanged\n", PAR(p)->comment, changed ? "" : "not ");

			par_tree_iter_set_changed (tree, &iter, changed);
		}
	} while (gtk_tree_model_iter_next (tree, &iter));
}

static void
par_update_changes (GcxParamsDialog *dialog)
{
	GcxParamsDialogPrivate *priv = dialog->priv;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_first (priv->par_treemodel, &iter);
	par_tree_update_changes (priv->par_treemodel, iter);
}


extern void error_beep(void);

static void
par_entry_changed_cb (GtkEditable *entry, gpointer user)
{
	GcxParamsDialog *dialog = GCX_PARAMS_DIALOG (user);
	GcxParamsDialogPrivate *priv = dialog->priv;
	GcxPar p;
	const char *text;
	char buf[256];

	if (!par_tree_get_selected (dialog, &p)) {
		d3_printf("No par selected.\n");
		return;
	}

	text = gtk_entry_get_text (GTK_ENTRY (priv->value_entry));

	make_value_string (p, buf, 255);
	if (!strcmp (buf, text))
		return;

	if (!try_update_par_value (p, text)) { // we have a successful update
		par_touch (p);

		par_update_current_status (dialog);
		par_update_changes (dialog);
	} else {
		error_beep();
		d3_printf ("cannot parse text\n");
	}
}

static void
par_selection_changed_cb (GtkTreeSelection *select, gpointer user)
{
	GcxParamsDialog *dialog = GCX_PARAMS_DIALOG (user);
	GcxParamsDialogPrivate *priv = dialog->priv;
	GtkTreeModel *model;

	/* true if we have a selection */
	priv->par_selected = gtk_tree_selection_get_selected(select, &model, &priv->selected_par_iter);
	par_update_current (dialog);
}

static void
par_save_clicked_cb (GtkButton *button, gpointer user)
{
	GcxParamsDialog *dialog = GCX_PARAMS_DIALOG (user);

	/* FIXME: NULL arg should be gcx instance */
	/* FIXME: save should take care of par flags */
	gcx_save_rcfile (NULL);

	par_update_current (dialog);
	par_update_changes (dialog);
}

#if 0
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
//                      par_item_update_value(p);
//                      d3_printf("defaulting par %d\n", p);
                        PAR(p)->flags &= ~PAR_USER;
                        PAR(p)->flags &= ~PAR_FROM_RC;
                        PAR(p)->flags &= ~PAR_TO_SAVE;
                        par_item_update_status(tree, p);
        }
}
#endif

/* recursively restore defaults on a par (sub)tree */
static void
par_tree_restore_defaults (GtkTreeModel *tree, GtkTreeIter iter)
{
	GtkTreeIter child;
	GcxPar p;

	if (gtk_tree_model_iter_children (tree, &child, &iter)) {
		p = par_tree_iter_get_par (tree, &iter);
		PAR(p)->flags &= ~PAR_USER;

		do {
			par_tree_restore_defaults (tree, child);
		} while (gtk_tree_model_iter_next (tree, &child));
	} else {
		p = par_tree_iter_get_par (tree, &iter);
		if (PAR(p)->flags & PAR_USER) {
			if (PAR_TYPE(p) == PAR_STRING)
				change_par_string (p, PAR(p)->defval.s);
			else
				memcpy(&(PAR(p)->val), &(PAR(p)->defval), sizeof(union pval));

			PAR(p)->flags &= ~(PAR_USER | PAR_FROM_RC | PAR_TO_SAVE);
		}
	}
}

static void
par_default_clicked_cb (GtkButton *button, gpointer user)
{
	GcxParamsDialog *dialog = GCX_PARAMS_DIALOG (user);
	GcxParamsDialogPrivate *priv = dialog->priv;
	GcxPar p;

	if (!par_tree_get_selected (dialog, &p)) {
		d3_printf ("No par selected.\n");
		return;
	}

	par_tree_restore_defaults (priv->par_treemodel, priv->selected_par_iter);
	par_update_current (dialog);
	par_update_changes (dialog);
}

static void
par_reload_clicked_cb (GtkButton *button, gpointer user)
{
	GcxParamsDialog *dialog = GCX_PARAMS_DIALOG (user);

	/* FIXME: NULL arg should be gcx instance */
	/* FIXME: load should take care of par flags */
	gcx_load_rcfile (NULL, NULL);

	par_update_current (dialog);
	par_update_changes (dialog);
}

static void
par_close_clicked_cb (GtkButton *button, gpointer user)
{
	GcxParamsDialog *dialog = GCX_PARAMS_DIALOG (user);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}





// compat
void act_control_options (GtkAction *action, gpointer window)
{
	GtkWidget *parwin;

	parwin = gcx_params_dialog_new (_("Gcx Options"), window);
	gtk_window_present (GTK_WINDOW (parwin));
}
