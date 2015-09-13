#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "gcx.h"
#include "gcx-imageview.h"

#define LUT_SIZE 4096

struct _GcxImageViewPrivate
{
	GtkAdjustment *hadjust;
	GtkAdjustment *vadjust;

	struct ccd_frame *fr;

	guint16 lut[LUT_SIZE];
	double hcut;
	double lcut;

	double zoom;

	struct gui_star_list *stars;
};

G_DEFINE_TYPE_WITH_PRIVATE (GcxImageView, gcx_image_view, GTK_TYPE_DRAWING_AREA);

static gboolean gcx_image_view_draw (GtkWidget *self, cairo_t *cr);
static void     gcx_image_view_size_allocate (GtkWidget *self, GtkAllocation *allocation);
static void     gcx_image_view_get_preferred_width (GtkWidget *self, gint *minimum, gint *natural);
static void     gcx_image_view_get_preferred_height (GtkWidget *self, gint *minimum, gint *natural);

#define ZOOM_STEP_LARGE 2
#define ZOOM_STEP_TRSHD 4

#define ZOOM_MAX 16

/* action values for zoom/pan */
#define VIEW_ZOOM_IN 0x100
#define VIEW_ZOOM_OUT 0x200
#define VIEW_ZOOM_FIT 0x300
#define VIEW_ZOOM_PIXELS 0x400
#define VIEW_PAN_CENTER 0x500
#define VIEW_PAN_CURSOR 0x600

/* preset contrast values in sigmas */
#define CONTRAST_STEP 1.4
#define SIGMAS_VALS 10
#define DEFAULT_SIGMAS 7 /* index in table */
static float sigmas[SIGMAS_VALS] = {
	2.8, 4, 5.6, 8, 11, 16, 22, 45, 90, 180
};
#define BRIGHT_STEP 0.05 /* of the cuts span */
#define DEFAULT_AVG_AT 0.15

#define BRIGHT_STEP_DRAG 0.003 /* adjustments steps for drag adjusts */
#define CONTRAST_STEP_DRAG 0.01

#define ZOOM_STEP_LARGE 2
#define ZOOM_STEP_TRSHD 4

#define MIN_SPAN 32

/* action values for cuts callback */
#define CUTS_AUTO 0x100
#define CUTS_MINMAX 0x200
#define CUTS_FLATTER 0x300
#define CUTS_SHARPER 0x400
#define CUTS_BRIGHTER 0x500
#define CUTS_DARKER 0x600
#define CUTS_CONTRAST 0x700
#define CUTS_INVERT 0x800
#define CUTS_VAL_MASK 0x000000ff
#define CUTS_ACT_MASK 0x0000ff00

static void
gcx_image_view_init (GcxImageView *self)
{
	GcxImageViewPrivate *priv;

        priv = self->priv = gcx_image_view_get_instance_private (self);

        gtk_widget_init_template (GTK_WIDGET (self));

	int i;
	for (i = 0; i < LUT_SIZE; i++) {
		priv->lut[i] = i << 4;
	}

	priv->zoom = 1.0;

	priv->stars = NULL;
}


static void
gcx_image_view_class_init (GcxImageViewClass *class)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

        gtk_widget_class_set_template_from_resource
                (GTK_WIDGET_CLASS (class), "/org/gcx/ui/imageview.ui");

        /* widgets */
	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxImageView, hadjust);

	gtk_widget_class_bind_template_child_private
		(GTK_WIDGET_CLASS (class), GcxImageView, vadjust);


	widget_class->draw = gcx_image_view_draw;

	widget_class->get_preferred_width = gcx_image_view_get_preferred_width;
	widget_class->get_preferred_height = gcx_image_view_get_preferred_height;

	widget_class->size_allocate = gcx_image_view_size_allocate;
}

/* gtk_widget_get_pointer (widget, &x, &y) was too easy, so it got deprecated. */
static gboolean
get_pointer(GtkWidget *widget, gint *x, gint *y, GdkModifierType *mask)
{
	GdkDisplay *display;
	GdkDeviceManager *device_manager;
	GdkDevice *device;

	if (!gtk_widget_get_realized (widget))
		return FALSE;

	display = gtk_widget_get_display (widget);
	device_manager = gdk_display_get_device_manager (display);
	device = gdk_device_manager_get_client_pointer (device_manager);

	gdk_window_get_device_position (
		gtk_widget_get_window (widget), device, x, y, mask);

	return TRUE;
}

static inline float
pixel_at(float *data, int stride, int x, int y)
{
	return data[y * stride + x];
}

static cairo_surface_t *
render_gray_zi (GcxImageView *view, int zoom, int x, int y, int w, int h)
{
	GcxImageViewPrivate *priv = view->priv;
	cairo_surface_t *dst;
	guint32 *pdst;
	float gain = LUT_SIZE / (priv->hcut - priv->lcut);
	float flr = priv->lcut;
	int lndx;
	guint8 pix;
	guint32 val;
	int xx, yy, jump;

	fprintf(stderr, "iv::render_zo x %d, y %d, w %d, h %d, zoom %d, hcut %.f, lcut %.f\n",
		x, y, w, h, zoom, priv->hcut, priv->lcut);

	dst = cairo_image_surface_create (CAIRO_FORMAT_RGB24, w, h);
	pdst = (guint32 *) cairo_image_surface_get_data (dst);
	jump = cairo_image_surface_get_stride (dst) / sizeof(guint32) - w;

	for (yy = y; yy < y + h; yy++) {
		for (xx = x; xx < x + w; xx++) {
			lndx = gain * (pixel_at(priv->fr->dat, priv->fr->w, xx / zoom, yy / zoom) - flr);
			clamp_int (&lndx, 0, LUT_SIZE - 1);
			pix = (priv->lut[lndx] >> 8);

			val = (pix << 16) + (pix << 8) + pix;

			*pdst++ = val;
		}

		pdst += jump;
	}

	return dst;
}

static cairo_surface_t *
render_gray_zo (GcxImageView *view, int zoom, int x, int y, int w, int h)
{
	GcxImageViewPrivate *priv = view->priv;
	cairo_surface_t *dst;
	guint32 *pdst;
	float gain = LUT_SIZE / (priv->hcut - priv->lcut);
	float flr = priv->lcut;
	int lndx;
	guint8 pix;
	guint32 val;
	int xx, yy, jump;

	fprintf(stderr, "iv::render_zo x %d, y %d, w %d, h %d, zoom %d, hcut %.f, lcut %.f\n",
		x, y, w, h, zoom, priv->hcut, priv->lcut);

	dst = cairo_image_surface_create (CAIRO_FORMAT_RGB24, w, h);
	pdst = (guint32 *) cairo_image_surface_get_data (dst);
	jump = cairo_image_surface_get_stride (dst) / sizeof(guint32) - w;

	for (yy = y; yy < y + h; yy++) {
		for (xx = x; xx < x + w; xx++) {
			lndx = gain * (pixel_at(priv->fr->dat, priv->fr->w, xx * zoom, yy * zoom) - flr);
			clamp_int (&lndx, 0, LUT_SIZE - 1);
			pix = (priv->lut[lndx] >> 8);

			val = (pix << 16) + (pix << 8) + pix;

			*pdst++ = val;
		}

		pdst += jump;
	}

	return dst;
}

static gboolean
gcx_image_view_draw (GtkWidget *self, cairo_t *cr)
{
	GcxImageViewPrivate *priv = GCX_IMAGE_VIEW (self)->priv;
	GdkRectangle rect;
	cairo_surface_t *surface;
        int zoom_in = 1;
        int zoom_out = 1;
	int fx, fy, fw, fh;
	struct timespec t1, t2;

	if (!priv->fr)
		return FALSE;

	if (!gdk_cairo_get_clip_rectangle (cr, &rect)) {
		fprintf(stderr, "iv::draw CLIPPED\n");
		return FALSE;
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);

	if (priv->zoom > 1.0)
		zoom_in = floor(priv->zoom + 0.5);
	else
		zoom_out = floor(1.0 / priv->zoom + 0.5);

	fprintf(stderr, "iv::draw x %d, y %d, w %d, h %d, zi %d, zo %d\n",
		rect.x, rect.y, rect.width, rect.height, zoom_in, zoom_out);

	/* calculate the frame coords for the exposed area */
        fx = rect.x * zoom_out / zoom_in;
        fy = rect.y * zoom_out / zoom_in;
        fw = rect.width * zoom_out / zoom_in;// + (zoom_in > 1 ? 2 : 0);
        fh = rect.height * zoom_out / zoom_in;// + (zoom_in > 1 ? 2 : 0);

        if (fx > priv->fr->w - 1)
                fx = priv->fr->w - 1;

        if (fx + fw > priv->fr->w - 1)
                fw = priv->fr->w - fx;

        if (fy > priv->fr->h - 1)
                fy = priv->fr->h - 1;

        if (fy + fh > priv->fr->h - 1)
                fh = priv->fr->h - fy;

        if (zoom_out > 1)
		surface = render_gray_zo (GCX_IMAGE_VIEW (self), zoom_out, rect.x, rect.y, rect.width, rect.height);
        else
		surface = render_gray_zi (GCX_IMAGE_VIEW (self), zoom_in, rect.x, rect.y, rect.width, rect.height);

	cairo_set_source_surface (cr, surface, rect.x, rect.y);
	cairo_paint (cr);

	clock_gettime(CLOCK_MONOTONIC, &t2);

	long delta = (t2.tv_nsec - t1.tv_nsec) + (t2.tv_sec - t1.tv_sec) * 1000000000;
	fprintf(stderr, "draw took %ld us\n", delta / 1000);

	return FALSE;
}

// called after show(), I can get my allocated size here
static void
gcx_image_view_size_allocate (GtkWidget *self, GtkAllocation *allocation)
{
	GcxImageViewPrivate *priv = GCX_IMAGE_VIEW (self)->priv;


	fprintf(stderr, "iv::size_allocate w %d, h %d\n", allocation->width, allocation->height);

	if (priv->fr) {
		clamp_int(&allocation->width, 0, priv->zoom * priv->fr->w);
		clamp_int(&allocation->height, 0, priv->zoom * priv->fr->h);
	}

	GTK_WIDGET_CLASS (gcx_image_view_parent_class)->size_allocate (self, allocation);
}

static void
gcx_image_view_get_preferred_width (GtkWidget *self, gint *minimum, gint *natural)
{
	GcxImageViewPrivate *priv = GCX_IMAGE_VIEW (self)->priv;
	int w, h;

	if (priv->fr) {
		w = priv->fr->w;
		h = priv->fr->h;
	} else {
		w = 0;
		h = 0;
	}

	*minimum = priv->zoom * w;
	*natural = priv->zoom * h;

	fprintf(stderr, "iv::get_preferred_width min %d, nat %d\n", *minimum, *natural);
}

static void
gcx_image_view_get_preferred_height (GtkWidget *self, gint *minimum, gint *natural)
{
	GcxImageViewPrivate *priv = GCX_IMAGE_VIEW (self)->priv;
	int w, h;

	if (priv->fr) {
		w = priv->fr->w;
		h = priv->fr->h;
	} else {
		w = 0;
		h = 0;
	}

	*minimum = priv->zoom * w;
	*natural = priv->zoom * h;

	fprintf(stderr, "iv::get_preferred_height min %d, nat %d\n", *minimum, *natural);
}

GtkAdjustment *
gcx_image_view_get_hadjustment (GcxImageView *self)
{
	GcxImageViewPrivate *priv = GCX_IMAGE_VIEW (self)->priv;

	return priv->hadjust;
}

GtkAdjustment *
gcx_image_view_get_vadjustment (GcxImageView *self)
{
	GcxImageViewPrivate *priv = GCX_IMAGE_VIEW (self)->priv;

	return priv->vadjust;
}

GtkWidget *
gcx_image_view_new ()
{
        return g_object_new (GCX_TYPE_IMAGE_VIEW, NULL);
}


void
gcx_image_view_set_frame (GcxImageView *self, struct ccd_frame *fr)
{
	GcxImageViewPrivate *priv = self->priv;

	if (priv->fr)
		release_frame (priv->fr);

	priv->fr = fr;
	get_frame (fr);

	priv->lcut = 10000;
	priv->hcut = 25000;

	gtk_widget_set_size_request (GTK_WIDGET (self), fr->w, fr->h);
	//gtk_widget_queue_resize (GTK_WIDGET (self));
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

struct ccd_frame *
gcx_image_view_get_frame (GcxImageView *self)
{
	GcxImageViewPrivate *priv = self->priv;

	return priv->fr;
}


struct gui_star_list *
gcx_image_view_get_stars (GcxImageView *self)
{
	GcxImageViewPrivate *priv = self->priv;
	return priv->stars;
}

void
gcx_image_view_set_stars (GcxImageView *self, struct gui_star_list *stars)
{
	GcxImageViewPrivate *priv = self->priv;
	priv->stars = stars;
}


static double
step_zoom(double current_zoom, int step)
{
	double zoom = current_zoom;

	if (zoom < 1) {
                zoom = floor(1.0 / zoom + 0.5);
        } else {
                zoom = floor(zoom + 0.5);
        }

        if (zoom == 1.0)
		return (step > 0) ? 2.0 : 0.5;

	if (current_zoom < 1)
		step = -step;

        if (step > 0) {
                if (zoom < ZOOM_STEP_TRSHD)
                        zoom += 1.0;
                else
                        zoom += ZOOM_STEP_LARGE;
        } else {
                if (zoom > ZOOM_STEP_TRSHD)
                        zoom -= ZOOM_STEP_LARGE;
                else
                        zoom -= 1.0;
        }

        if (zoom > ZOOM_MAX)
                zoom = ZOOM_MAX;

        if (current_zoom < 1.0)
		return 1.0 / zoom;
        else
		return zoom;
}

/* return the position of the scrollbars as
 * the fraction of the image's dimension the center of the
 * visible area is at
 */

static void
get_adjust_positions (GcxImageView *view, int *xc, int *yc)
{
	GcxImageViewPrivate *priv = view->priv;
	GtkAdjustment *adjust;
	gdouble value, page_size, lower, upper;

	adjust = priv->hadjust;

	value     = gtk_adjustment_get_value (adjust);
	page_size = gtk_adjustment_get_page_size (adjust);
	lower     = gtk_adjustment_get_lower (adjust);
	upper     = gtk_adjustment_get_upper (adjust);
        fprintf(stderr, "horiz v %.3f p %.3f l %.3f u %.3f\n", value, page_size, lower, upper);
        *xc = (value + page_size / 2) / (upper - lower);

	adjust = priv->vadjust;

	value     = gtk_adjustment_get_value (adjust);
	page_size = gtk_adjustment_get_page_size (adjust);
	lower     = gtk_adjustment_get_lower (adjust);
	upper     = gtk_adjustment_get_upper (adjust);
        fprintf(stderr, "vert v %.3f p %.3f l %.3f u %.3f\n", value, page_size, lower, upper);
        *yc = (value + page_size / 2) / (upper - lower);

	fprintf(stderr, "pos: %d %d\n", *xc, *yc);
}

/* pan the image in the scrolled window
 * so that the center of the viewable area is at xc and yc
 * of the image's width/height
 */

static void
set_adjust_positions (GcxImageView *view, double xc, double yc)
{
}

static void
set_darea_size (GcxImageView *view, double xc, double yc)
{
	GcxImageViewPrivate *priv = view->priv;
	GtkAdjustment *adj;
	GtkAllocation allocation;
        int zi, zo;
        int w, h, zw, zh;
	gdouble value, lower, upper, page_size;

	gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);

	w = allocation.width;
	h = allocation.height;

	if (priv->zoom > 1.0) {
		zo = 1;
		zi = floor (priv->zoom + 0.5);
	} else {
		zi = 1;
		zo = floor (1.0 / priv->zoom + 0.5);
	}

//	gtk_widget_set_size_request (GTK_WIDGET (view),
//				     priv->w * zi / zo,
//				     priv->h * zi / zo);


//        gdk_window_freeze_updates(window);

	zw = priv->fr->w * zi / zo;
	zh = priv->fr->h * zi / zo;

        gtk_widget_set_size_request (GTK_WIDGET (view), zw, zh);

        /* we need this to make sure the drawing area contracts properly
         * when we go from scrollbars to no scrollbars, we have to be sure
         * everything happens in sequnece */

        if (w >= zw || h >= zh) {
                gtk_widget_queue_resize (GTK_WIDGET(view));
                while (gtk_events_pending ())
                        gtk_main_iteration ();
        }

        adj = priv->hadjust;
        g_object_get (G_OBJECT(adj), "value", &value, "lower", &lower,
		      "upper", &upper, "page-size", &page_size, NULL);

        if (page_size < zw) {
                upper = zw;
                if (upper < page_size)
                        upper = page_size;

                value = (upper - lower) * xc + lower - page_size / 2;
                clamp_double(&value, lower, upper - page_size);

                g_object_set (G_OBJECT(adj), "upper", upper, "value", value, NULL);
        }

        adj = priv->vadjust;
        if (page_size < zh) {
                upper = zh;
                if (upper < page_size)
                        upper = page_size;

                value = (upper - lower) * yc + lower - page_size / 2;
                clamp_double(&value, lower, upper - page_size);

                g_object_set (G_OBJECT(adj), "upper", upper, "value", value, NULL);
        }

        gtk_widget_queue_draw (GTK_WIDGET(view));

        //gdk_window_thaw_updates(window);
}


static void
zoom_pan(GcxImageView *view, int action)
{
	GcxImageViewPrivate *priv = view->priv;
	gint x, y, w, h, xc, yc;

	w = priv->zoom * priv->fr->w;
	h = priv->zoom * priv->fr->h;

	get_pointer (GTK_WIDGET (view), &x, &y, NULL);
	fprintf(stderr, "view action %d at x:%d y:%d \n", action, x, y);

       switch(action) {
       case VIEW_ZOOM_IN:
	       //get_scrolls(window, &xc, &yc);
	       get_adjust_positions (view, &xc, &yc);

	       priv->zoom = step_zoom(priv->zoom, +1);

	       set_darea_size(view, 1.0 * x / w, 1.0 * y / h);
	       break;
       case VIEW_ZOOM_OUT:
	       //get_scrolls(window, &xc, &yc);
	       get_adjust_positions (view, &xc, &yc);

	       fprintf(stderr, "zooming out to %d %d\n", xc, yc);
	       //step_zoom(geom, -1);
	       priv->zoom = step_zoom(priv->zoom, -1);

	       set_darea_size(view, xc, yc);

	       set_adjust_positions (view, xc, yc);
	       break;
/*
        case VIEW_PIXELS:
                get_scrolls(window, &xc, &yc);
                if (geom->zoom > 1) {
                        geom->zoom = 1.0;
                        set_darea_size(window, geom, xc, yc);
                        gtk_widget_queue_draw(window);
                } else if (geom->zoom < 1.0) {
                        geom->zoom = 1.0;
                        set_darea_size(window, geom, 1.0 * x / w, 1.0 * y / h);
                        gtk_widget_queue_draw(window);
                }
                break;
        case VIEW_PAN_CENTER:
                set_scrolls(window, 0.5, 0.5);
                break;
        case VIEW_PAN_CURSOR:
                set_scrolls(window, 1.0 * x / w, 1.0 * y / h);
                break;

        default:
                err_printf("unknown view action %d, ignoring\n", action);
                return;
*/
        }
//        show_zoom_cuts(window);
}

void
gcx_image_view_zoom_in (GcxImageView *self)
{
	zoom_pan (self, VIEW_ZOOM_IN);
}

void
gcx_image_view_zoom_out (GcxImageView *self)
{
	zoom_pan (self, VIEW_ZOOM_OUT);
}

void
gcx_image_view_zoom_fit (GcxImageView *self)
{
	zoom_pan (self, VIEW_ZOOM_FIT);
}

void
gcx_image_view_zoom_pixels (GcxImageView *self)
{
	zoom_pan (self, VIEW_ZOOM_PIXELS);
}

void
gcx_image_view_pan_center (GcxImageView *self)
{
	GcxImageViewPrivate *priv = self->priv;

	priv->hcut = 20000;
	priv->lcut = 10000;

	gtk_widget_queue_draw (GTK_WIDGET (self));

	//zoom_pan (self, VIEW_PAN_CENTER);
}

void
gcx_image_view_pan_cursor (GcxImageView *self)
{
	GcxImageViewPrivate *priv = self->priv;

	priv->hcut = 30000;
	priv->lcut = 10000;

	gtk_widget_queue_draw (GTK_WIDGET (self));

	//zoom_pan (self, VIEW_PAN_CURSOR);
}


double
gcx_image_view_get_zoom (GcxImageView *view)
{
	GcxImageViewPrivate *priv = view->priv;

	return priv->zoom;
}

int
gcx_image_view_to_pnm_file(GcxImageView *view, char *fn, int is_16bit)
{
	GcxImageViewPrivate *priv = view->priv;
	struct ccd_frame *fr = priv->fr;
	FILE *pnmf;
	int i;
	float *fdat;
	unsigned short pix;
	int lndx, all;
	float gain = LUT_SIZE / (priv->hcut - priv->lcut);
	float flr = priv->lcut;

	if (fr == NULL) {
		err_printf("view_to_pnm: no frame\n");
		return -1;
	}

	if (fn != NULL) {
		pnmf = fopen(fn, "w");
		if (pnmf == NULL) {
			err_printf("view_to_pnm_file: "
				   "cannot open file %s for writing\n", fn);
			return -1;
		}
	} else {
		pnmf = stdout;
	}

	fprintf(pnmf, "P5 %d %d %d\n", fr->w, fr->h, is_16bit ? 65535 : 255);

	fdat = (float *)fr->dat;
	all = fr->w * fr->h;

	for (i = 0; i < all; i++) {
		lndx = (gain * (*fdat - flr));

		clamp_int(&lndx, 0, LUT_SIZE - 1);

		pix = priv->lut[lndx];
		fdat ++;
		putc(pix >> 8, pnmf);
		if (is_16bit)
			putc(pix & 0xff, pnmf);
	}

	if (pnmf != stdout)
		fclose(pnmf);

	return 0;
}


void act_control_histogram(GtkAction *action, gpointer window)
{
#if 0
	GtkWidget *dialog, *close, *darea;
	void *ret;
	GtkWidget* create_imadj_dialog (void);
	//struct image_channel *i_channel;

	ret = g_object_get_data(G_OBJECT(window), "i_channel");
	if (ret == NULL) /* no channel */
		return;
	//i_channel = ret;

	dialog = g_object_get_data(G_OBJECT(window), "imadj_dialog");
	if (dialog == NULL) {
		dialog = create_imadj_dialog();
		gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
		g_object_ref(dialog);
		g_object_set_data_full(G_OBJECT(window), "imadj_dialog", dialog,
					 (GDestroyNotify)gtk_widget_destroy);
		g_object_set_data(G_OBJECT(dialog), "image_window", window);
		close = g_object_get_data(G_OBJECT(dialog), "hist_close");
		g_signal_connect (G_OBJECT (dialog), "destroy",
				    G_CALLBACK (close_imadj_dialog), window);
		g_signal_connect (G_OBJECT (close), "clicked",
				    G_CALLBACK (close_imadj_dialog), window);
		darea = g_object_get_data(G_OBJECT(dialog), "hist_area");
		g_signal_connect (G_OBJECT (darea), "expose-event",
				    G_CALLBACK (histogram_expose_cb), dialog);

		imadj_set_callbacks(dialog);
		gtk_widget_show(dialog);
	} else {
		gtk_window_present (GTK_WINDOW (dialog));
	}
	//ref_image_channel(i_channel);
	//g_object_set_data_full(G_OBJECT(dialog), "i_channel", i_channel,
	//			 (GDestroyNotify)release_image_channel);
	imadj_dialog_update(dialog);
	imadj_dialog_edit(dialog);
#endif

}

/**********************
 * callbacks from gui
 **********************/

/*
 * changing cuts (brightness/contrast)
 */
static void cuts_option_cb(gpointer data, guint action)
{
#if 0
	GtkWidget *window = data;
	GcxImageView *view;

	view = g_object_get_data(G_OBJECT(window), "image_view");

	channel_cuts_action(view->map, action);
	show_zoom_cuts(window);

	gtk_widget_queue_draw(GTK_WIDGET(view->darea));
#endif
}

void act_view_cuts_brighter(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_BRIGHTER);
}

void act_view_cuts_auto(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_AUTO);
}

void act_view_cuts_minmax(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_MINMAX);
}

void act_view_cuts_darker(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_DARKER);
}

void act_view_cuts_invert(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_INVERT);
}

void act_view_cuts_flatter(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_FLATTER);
}

void act_view_cuts_sharper(GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_SHARPER);
}

void act_view_cuts_contrast_1 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|1);
}

void act_view_cuts_contrast_2 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|2);
}

void act_view_cuts_contrast_3 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|3);
}

void act_view_cuts_contrast_4 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|4);
}

void act_view_cuts_contrast_5 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|5);
}

void act_view_cuts_contrast_6 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|6);
}

void act_view_cuts_contrast_7 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|7);
}

void act_view_cuts_contrast_8 (GtkAction *action, gpointer window)
{
	cuts_option_cb(window, CUTS_CONTRAST|8);
}


/*
 * zoom/pan
 */
static void view_option_cb(gpointer window, guint action)
{
#if 0
	GcxImageView *view;
	int x, y, w, h;
	GdkModifierType mask;
	GdkWindow *gdkw;
	double xc, yc;

	view = g_object_get_data(G_OBJECT(window), "image_view");
	g_return_if_fail (GCX_IS_IMAGE_VIEW(view));

	gdkw = gtk_widget_get_window(GTK_WIDGET(view->darea));
	gdk_window_get_pointer(gdkw, &x, &y, &mask);
	//gdk_drawable_get_size(gtk_widget_get_window(GTK_WIDGET(view->darea)), &w, &h);
	w = gdk_window_get_width (gdkw);
	h = gdk_window_get_height (gdkw);

	d2_printf("view action %d at x:%d y:%d  (w:%d h:%d) mask: %d\n",
	       action, x, y, w, h, mask);

	switch(action) {
	case VIEW_ZOOM_IN:
		gcx_image_view_get_scrolls(view, &xc, &yc);
		step_zoom(view->map, +1);
		set_view_size(view, 1.0 * x / w, 1.0 * y / h);
		gtk_widget_queue_draw(window);
		break;
	case VIEW_ZOOM_OUT:
		gcx_image_view_get_scrolls(view, &xc, &yc);
		step_zoom(view->map, -1);
		set_view_size(view, xc, yc);
		gcx_image_view_set_scrolls(view, xc, yc);
		gtk_widget_queue_draw(window);
		break;
	case VIEW_PIXELS:
		gcx_image_view_get_scrolls(view, &xc, &yc);
		if (view->map->zoom > 1) {
			view->map->zoom = 1.0;
			set_view_size(view, xc, yc);
			gtk_widget_queue_draw(window);
		} else if (view->map->zoom < 1.0) {
			view->map->zoom = 1.0;
			set_view_size(view, 1.0 * x / w, 1.0 * y / h);
			gtk_widget_queue_draw(window);
		}
		break;
	case VIEW_PAN_CENTER:
		gcx_image_view_set_scrolls(view, 0.5, 0.5);
		break;
	case VIEW_PAN_CURSOR:
		gcx_image_view_set_scrolls(view, 1.0 * x / w, 1.0 * y / h);
		break;

	default:
		err_printf("unknown view action %d, ignoring\n", action);
		return;
	}
	show_zoom_cuts(window);
#endif
}

void act_view_zoom_in (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_ZOOM_IN);
}

void act_view_zoom_out (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_ZOOM_OUT);
}

void act_view_pixels (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_ZOOM_PIXELS);
}

void act_view_pan_center (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_PAN_CENTER);
}

void act_view_pan_cursor (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_PAN_CURSOR);
}
