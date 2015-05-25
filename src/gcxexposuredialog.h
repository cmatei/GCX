#ifndef __GCXEXPOSUREDIALOG_H
#define __GCXEXPOSUREDIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxExposureDialog          GcxExposureDialog;
typedef struct _GcxExposureDialogClass     GcxExposureDialogClass;
typedef struct _GcxExposureDialogPrivate   GcxExposureDialogPrivate;

#define GCX_TYPE_EXPOSURE_DIALOG            (gcx_exposure_dialog_get_type())
#define GCX_EXPOSURE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_EXPOSURE_DIALOG, GcxExposureDialog))
#define GCX_EXPOSURE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_EXPOSURE_DIALOG, GcxExposureDialogClass))
#define GCX_IS_EXPOSURE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_EXPOSURE_DIALOG))
#define GCX_IS_EXPOSURE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_EXPOSURE_DIALOG))


struct _GcxExposureDialog
{
	GtkDialog parent_instance;

	GcxExposureDialogPrivate *priv;
};


struct _GcxExposureDialogClass
{
	GtkDialogClass parent_class;
};


GType                gcx_exposure_dialog_get_type                (void) G_GNUC_CONST;

GtkWidget *          gcx_exposure_dialog_new                     (const gchar *title,
								  GtkWindow   *parent);


G_END_DECLS


#endif
