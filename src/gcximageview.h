#ifndef __GCXVIEW_H
#define __GCXVIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxImageView          GcxImageView;
typedef struct _GcxImageViewClass     GcxImageViewClass;

#define GCX_IMAGE_VIEW_TYPE            (gcx_image_view_get_type())
#define GCX_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_IMAGE_VIEW_TYPE, GcxImageView))
#define GCX_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_IMAGE_VIEW_TYPE, GcxImageViewClass))
#define IS_GCX_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_IMAGE_VIEW_TYPE))
#define IS_GCX_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_IMAGE_VIEW_TYPE))


GType gcx_image_view_get_type (void);

GtkWidget * gcx_image_view_new();

int               gcx_image_view_set_frame  (GcxImageView *view, struct ccd_frame *fr);
struct ccd_frame *gcx_image_view_get_frame  (GcxImageView *view);

void              gcx_image_view_set_scrolls(GcxImageView *view, double xc, double yc);
void              gcx_image_view_get_scrolls(GcxImageView *view, double *xc, double *yc);

int               gcx_image_view_to_pnm_file(GcxImageView *view, char *fn, int is_16bit);


G_END_DECLS

#endif
