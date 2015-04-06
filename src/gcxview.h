#ifndef __GCXVIEW_H
#define __GCXVIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxView          GcxView;
typedef struct _GcxViewClass     GcxViewClass;

#define GCX_VIEW_TYPE            (gcx_view_get_type())
#define GCX_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_VIEW_TYPE, GcxView))
#define GCX_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GCX_VIEW_TYPE, GcxViewClass))
#define IS_GCX_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_VIEW_TYPE))
#define IS_GCX_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GCX_VIEW_TYPE))


GType gcx_view_get_type (void);

GtkWidget * gcx_view_new();

int               gcx_view_set_frame  (GcxView *view, struct ccd_frame *fr);
struct ccd_frame *gcx_view_get_frame  (GcxView *view);

void              gcx_view_set_scrolls(GcxView *view, double xc, double yc);
void              gcx_view_get_scrolls(GcxView *view, double *xc, double *yc);

int               gcx_view_to_pnm_file(GcxView *view, char *fn, int is_16bit);


G_END_DECLS

#endif
