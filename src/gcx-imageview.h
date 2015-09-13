#ifndef __GCX_IMAGEVIEW_H
#define __GCX_IMAGEVIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcxImageView           GcxImageView;
typedef struct _GcxImageViewClass      GcxImageViewClass;
typedef struct _GcxImageViewPrivate    GcxImageViewPrivate;

#define GCX_TYPE_IMAGE_VIEW            (gcx_image_view_get_type())
#define GCX_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_IMAGE_VIEW, GcxImageView))
#define GCX_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_IMAGE_VIEW, GcxImageViewClass))
#define GCX_IS_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_IMAGE_VIEW))
#define GCX_IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_IMAGE_VIEW))


struct _GcxImageView
{
        GtkDrawingArea parent_instance;
        GcxImageViewPrivate *priv;
};


struct _GcxImageViewClass
{
        GtkDrawingAreaClass parent_class;
};


GType                 gcx_image_view_get_type                (void) G_GNUC_CONST;

GtkWidget *           gcx_image_view_new                     ();

GtkAdjustment *       gcx_image_view_get_hadjustment (GcxImageView *view);
GtkAdjustment *       gcx_image_view_get_vadjustment (GcxImageView *view);


void                  gcx_image_view_set_frame (GcxImageView *view, struct ccd_frame *fr);
struct ccd_frame     *gcx_image_view_get_frame (GcxImageView *view);

struct gui_star_list *gcx_image_view_get_stars (GcxImageView *view);
void                  gcx_image_view_set_stars (GcxImageView *view, struct gui_star_list *stars);


void                  gcx_image_view_zoom_in (GcxImageView *view);
void                  gcx_image_view_zoom_out (GcxImageView *view);
void                  gcx_image_view_zoom_fit (GcxImageView *view);
void                  gcx_image_view_zoom_pixels (GcxImageView *view);

void                  gcx_image_view_pan_center (GcxImageView *self);
void                  gcx_image_view_pan_cursor (GcxImageView *self);


double                gcx_image_view_get_zoom (GcxImageView *view);

int                   gcx_image_view_to_pnm_file(GcxImageView *view, char *fn, int is_16bit);


G_END_DECLS


#endif
