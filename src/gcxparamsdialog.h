#ifndef __GCX_PARAMS_DIALOG_H
#define __GCX_PARAMS_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GCX_TYPE_PARAMS_DIALOG                  (gcx_params_dialog_get_type ())
#define GCX_PARAMS_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_PARAMS_DIALOG, GcxParamsDialog))
#define GCX_PARAMS_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_PARAMS_DIALOG, GcxParamsDialogClass))
#define GCX_IS_PARAMS_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_PARAMS_DIALOG))
#define GCX_IS_PARAMS_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_PARAMS_DIALOG))
#define GCX_PARAMS_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj),  GCX_TYPE_PARAMS_DIALOG, GcxParamsDialogClass))


typedef struct _GcxParamsDialog         GcxParamsDialog;
typedef struct _GcxParamsDialogClass    GcxParamsDialogClass;
typedef struct _GcxParamsDialogPrivate  GcxParamsDialogPrivate;

struct _GcxParamsDialog
{
	GtkDialog parent_instance;

	GcxParamsDialogPrivate *priv;
};

struct _GcxParamsDialogClass
{
	GtkDialogClass parent_class;
};

GType                gcx_params_dialog_get_type                (void) G_GNUC_CONST;

GtkWidget *          gcx_params_dialog_new                     (const gchar *title,
								GtkWindow   *parent);


G_END_DECLS

#endif
