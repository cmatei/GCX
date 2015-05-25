
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "gcx.h"
#include "gcxexposuredialog.h"

struct _GcxExposureDialogPrivate
{
	GtkWidget *cam_binx_combotext;
	GtkWidget *cam_biny_combotext;
	GtkWidget *cam_filter_combotext;
	GtkWidget *cam_frametype_combotext;
	GtkWidget *cam_readout_combotext;
	GtkWidget *cam_iso_combotext;

	GtkWidget *cam_exposure_time_spin;
	GtkWidget *cam_exposure_count_spin;
	GtkWidget *cam_exposure_time_progress;
	GtkWidget *cam_exposure_count_progress;
	GtkWidget *cam_autosave_entry;
	GtkWidget *cam_get_button;
	GtkWidget *cam_multiple_button;
	GtkWidget *cam_abort_button;

	GtkWidget *cam_xskip_spin;
	GtkWidget *cam_yskip_spin;
	GtkWidget *cam_width_spin;
	GtkWidget *cam_height_spin;
	GtkWidget *cam_sz1_button;
	GtkWidget *cam_sz2_button;
	GtkWidget *cam_sz4_button;
	GtkWidget *cam_sz8_button;
	GtkWidget *cam_subframe_checkbox;

	GtkWidget *cam_delay_first_spin;
	GtkWidget *cam_delay_every_spin;
	GtkWidget *cam_delay_checkbox;

	GtkWidget *cam_dither_method_combotext;
	GtkWidget *cam_dither_pixels_spin;
	GtkWidget *cam_dither_checkbox;

	GtkWidget *cam_refocus_temp_spin;
	GtkWidget *cam_refocus_temp_checkbox;
	GtkWidget *cam_refocus_time_spin;
	GtkWidget *cam_refocus_time_checkbox;
	GtkWidget *cam_refocus_checkbox;
};

G_DEFINE_TYPE_WITH_PRIVATE (GcxExposureDialog, gcx_exposure_dialog, GTK_TYPE_WINDOW);

static void
gcx_exposure_dialog_init (GcxExposureDialog *self)
{
	self->priv = gcx_exposure_dialog_get_instance_private (self);

	gtk_widget_init_template (GTK_WIDGET (self));
}

static void
gcx_exposure_dialog_class_init (GcxExposureDialogClass *class)
{
	gtk_widget_class_set_template_from_resource
		(GTK_WIDGET_CLASS (class), "/org/gcx/ui/exposuredialog.ui");

	/* widgets */
//	gtk_widget_class_bind_template_child_private
//		(GTK_WIDGET_CLASS (class), GcxExposureDialog, par_treemodel);

	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_binx_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_biny_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_filter_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_frametype_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_readout_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_iso_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_exposure_time_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_exposure_count_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_exposure_time_progress);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_exposure_count_progress);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_autosave_entry);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_get_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_multiple_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_abort_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_xskip_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_yskip_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_width_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_height_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_sz1_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_sz2_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_sz4_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_sz8_button);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_subframe_checkbox);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_delay_first_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_delay_every_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_delay_checkbox);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_dither_method_combotext);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_dither_pixels_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_dither_checkbox);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_refocus_temp_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_refocus_temp_checkbox);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_refocus_time_spin);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_refocus_time_checkbox);
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxExposureDialog, cam_refocus_checkbox);



#if 0
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
#endif
}

GtkWidget *
gcx_exposure_dialog_new (const gchar *title,
			 GtkWindow   *parent)
{
	return g_object_new (GCX_TYPE_EXPOSURE_DIALOG,
			     "transient-for", parent,
			     "title", title ? title : _("Camera Control"),
			     NULL);
}


// compat
void
act_control_camera (GtkAction *action, gpointer window)
{
	GtkWidget *camwin;

	camwin = gcx_exposure_dialog_new (_("Exposure Control"), window);
	gtk_window_present (GTK_WINDOW (camwin));
}
