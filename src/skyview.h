#ifndef __GCX_SKYVIEW_H
#define __GCX_SKYVIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxSkyviewQuery          GcxSkyviewQuery;
typedef struct _GcxSkyviewQueryPrivate   GcxSkyviewQueryPrivate;
typedef struct _GcxSkyviewQueryClass     GcxSkyviewQueryClass;

#define GCX_TYPE_SKYVIEW_QUERY            (gcx_skyview_query_get_type())
#define GCX_SKYVIEW_QUERY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_SKYVIEW_QUERY, GcxSkyviewQuery))
#define GCX_SKYVIEW_QUERY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_SKYVIEW_QUERY, GcxSkyviewQueryClass))
#define GCX_IS_SKYVIEW_QUERY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_SKYVIEW_QUERY))
#define GCX_IS_SKYVIEW_QUERY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_SKYVIEW_QUERY))


GType gcx_skyview_query_get_type (void);

GtkWidget * gcx_skyview_query_new(GcxImageView *parent);



G_END_DECLS

#endif
