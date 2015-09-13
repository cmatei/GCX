#ifndef __GCX_APPLICATION_H
#define __GCX_APPLICATION_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxApplication          GcxApplication;
typedef struct _GcxApplicationClass     GcxApplicationClass;

#define GCX_TYPE_APPLICATION            (gcx_application_get_type())
#define GCX_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_APPLICATION, GcxApplication))
#define GCX_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_APPLICATION, GcxApplicationClass))
#define GCX_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_APPLICATION))
#define GCX_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_APPLICATION))


GType gcx_application_get_type (void);

GcxApplication * gcx_application_new();

G_END_DECLS

#endif
