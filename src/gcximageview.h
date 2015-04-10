#ifndef __GCXIMAGEVIEW_H
#define __GCXIMAGEVIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxImageView          GcxImageView;
typedef struct _GcxImageViewClass     GcxImageViewClass;

#define GCX_TYPE_IMAGE_VIEW            (gcx_image_view_get_type())
#define GCX_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_IMAGE_VIEW, GcxImageView))
#define GCX_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_IMAGE_VIEW, GcxImageViewClass))
#define GCX_IS_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_IMAGE_VIEW))
#define GCX_IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_IMAGE_VIEW))


GType gcx_image_view_get_type (void);

GtkWidget * gcx_image_view_new();

int               gcx_image_view_set_frame  (GcxImageView *view, struct ccd_frame *fr);
struct ccd_frame *gcx_image_view_get_frame  (GcxImageView *view);

void              gcx_image_view_set_scrolls(GcxImageView *view, double xc, double yc);
void              gcx_image_view_get_scrolls(GcxImageView *view, double *xc, double *yc);

double            gcx_image_view_get_zoom   (GcxImageView *view);

struct gui_star_list *gcx_image_view_get_stars (GcxImageView *view);
void                  gcx_image_view_set_stars (GcxImageView *view, struct gui_star_list *stars);

cairo_t          *gcx_image_view_cairo_surface (GcxImageView *view);

int               gcx_image_view_to_pnm_file(GcxImageView *view, char *fn, int is_16bit);


G_END_DECLS

#endif
