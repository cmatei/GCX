
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "interface.h"
#include "params.h"
#include "sourcesdraw.h"
#include "obsdata.h"
#include "recipy.h"
#include "symbols.h"
#include "wcs.h"
#include "multiband.h"
#include "filegui.h"
#include "plots.h"
#include "psf.h"
#include "misc.h"
#include "demosaic.h"

#include "gcximageview.h"

enum {
	FRAME_CHANGED,
	MAPPING_CHANGED,

	LAST_SIGNAL
};

static guint image_view_signals[LAST_SIGNAL] = { 0 };


/* image display parameters */
#define MAX_ZOOM 16

#define LUT_SIZE 4096
#define LUT_IDX_MASK 0x0fff

struct frame_map {
	struct ccd_frame *fr;	/* the image */

	double zoom;		/* zoom level */
	int width;		/* size of drawing area at zoom=1 */
	int height;

	double lcut;		/* the low cut */
	double hcut;		/* the high cut */
	unsigned int invert:1;  /* if 1, the image is displayed in reverse video */
	double avg_at;		/* position of average between cuts */
	double gamma;		/* gamma setting for image */
	double toe;		/* toe setting for image */
	double offset;		/* offset setting for image */
	unsigned short lut[LUT_SIZE];
	double dsigma;		/* sigma used for cut calculation */
	double davg;	        /* average used for cut calculations */
	unsigned int color:1;   /* display a color image */

	unsigned int changed:1; /* when anyhting is changed in the map, setting */
				/* this flag will ask for the map cache to be redrawn */

	/* FIXME: not used */
	int flip_h;             /* flag for horisontal flip */
	int flip_v;             /* flag for vertical flip */
	int zoom_mode;		/* zooming algorithm */

};


/* we keep a cache of the already trasformed image for quick expose
 * redraws.
 */
struct map_cache {
	int valid;		/* the cache is valid */
	int type;		/* type of cache: gray or rgb */
	double zoom;		/* zoom level of the cache */
	int x; /* coordinate of top-left corner of cache (in display space) */
	int y;
	int w; /* width of cache (in display pixels) */
	int h; /* height of cache (in display pixels) */

	cairo_surface_t *data;
};


static struct map_cache *new_map_cache(int width, int height);
static void free_map_cache(struct map_cache *cache);

static struct frame_map *new_frame_map();
static void free_frame_map(struct frame_map *map);


struct _GcxImageView {
	GtkScrolledWindow parent;

	GtkWidget *darea;

	struct map_cache *cache;
	struct frame_map *map;
	struct wcs *wcs;

	struct gui_star_list *stars;
};


struct _GcxImageViewClass {
	GtkScrolledWindowClass parent_class;

	void (*frame_changed)(GcxImageView *);
	void (*mapping_changed)(GcxImageView *);
};

G_DEFINE_TYPE(GcxImageView, gcx_image_view, GTK_TYPE_SCROLLED_WINDOW);

static gboolean gcx_image_view_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer user);
static gboolean gcx_image_view_motion_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer user);

static void
gcx_image_view_init(GcxImageView *view)
{
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(view),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);


	view->darea = gtk_drawing_area_new ();
	gtk_widget_set_halign (GTK_WIDGET(view->darea), GTK_ALIGN_CENTER);
	gtk_widget_set_valign (GTK_WIDGET(view->darea), GTK_ALIGN_CENTER);

	gtk_container_add (GTK_CONTAINER(view), GTK_WIDGET(view->darea));

	gtk_widget_set_events (view->darea, 0
			       | GDK_BUTTON_PRESS_MASK
			       | GDK_POINTER_MOTION_MASK
			       | GDK_POINTER_MOTION_HINT_MASK);

	g_signal_connect (G_OBJECT (view->darea), "draw",
			  G_CALLBACK (gcx_image_view_draw_cb), view);

	g_signal_connect (G_OBJECT (view), "motion-notify-event",
			  G_CALLBACK (gcx_image_view_motion_event_cb), view);

	view->cache = new_map_cache (0, 0);
	view->map = new_frame_map ();
	view->wcs = wcs_new();
}

static void
gcx_image_view_finalize(GObject *object)
{
	GcxImageView *view = (GcxImageView *) object;

	free_map_cache (view->cache);
	free_frame_map (view->map);
	wcs_release(view->wcs);

	G_OBJECT_CLASS (gcx_image_view_parent_class)->finalize (object);
}

static void
gcx_image_view_class_init(GcxImageViewClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;

	gobject_class->finalize = gcx_image_view_finalize;

	image_view_signals[FRAME_CHANGED] =
		g_signal_new ("frame-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GcxImageViewClass, frame_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	image_view_signals[MAPPING_CHANGED] =
		g_signal_new ("mapping-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GcxImageViewClass, mapping_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

}


GtkWidget *
gcx_image_view_new()
{
	return GTK_WIDGET(g_object_new (GCX_TYPE_IMAGE_VIEW, NULL));
}




/* create a map cache */
static struct map_cache *
new_map_cache(int width, int height)
{
	struct map_cache *cache;

	cache = g_new0 (struct map_cache, 1);

	cache->data = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);

	return cache;
}

static void
free_map_cache(struct map_cache *cache)
{
	if (!cache) return;

	cairo_surface_destroy (cache->data);
	g_free (cache);
}


/* create a frame mapping */
static struct frame_map *
new_frame_map()
{
	struct frame_map *map;
	int i;

	map = g_new0 (struct frame_map, 1);

	map->zoom = 1.0;

	map->lcut = 0.0;
	map->hcut = 255.0;
	map->avg_at = 0.5;
	map->davg = 127.0;
	map->dsigma = 12.0;
	map->gamma = 1.0;
	map->toe = 0.0;
	map->offset = 0.0;
	for (i=0; i<LUT_SIZE; i++) {
		map->lut[i] = i * 65535 / LUT_SIZE;
	}
	map->changed = 1;
	map->fr = NULL;
	map->color = 0;

	return map;
}

static void
free_frame_map(struct frame_map *map)
{
	if (map->fr)
		release_frame(map->fr);

	g_free (map);
}

static void set_default_channel_cuts(struct frame_map *map);
static void set_view_size(GcxImageView *view, double xc, double yc);


/*
 * return 1 if the requested area is inside the cache
 */
static int area_in_cache(cairo_rectangle_int_t *area, struct map_cache *cache)
{
	if (area->x < cache->x)
		return 0;
	if (area->y < cache->y)
		return 0;
	if (area->x + area->width > cache->x + cache->w)
		return 0;
	if (area->y + area->height > cache->y + cache->h)
		return 0;
	return 1;
}


/* render a float frame to the cache at zoom >= 1*/
static void cache_render_rgb_float_zi(struct map_cache *cache, struct frame_map *map,
				      int zoom, int fx, int fy, int fw, int fh)
{
	struct ccd_frame *fr = map->fr;
	int fjump = fr->w - fw;
	int cjump;
	int x, y, z;
	float *fdat_r, *fdat_g, *fdat_b;
	int lndx;
	float gain = LUT_SIZE / (map->hcut - map->lcut);
	float flr = map->lcut;
	guint8  pix;
	guint32 val;


	guint32 *cdat = (guint32 *) cairo_image_surface_get_data (cache->data);
	int stride = cairo_image_surface_get_stride (cache->data) / sizeof(guint32); /* in 32 bit ints */

	cache->x = fx * zoom;
	cache->y = fy * zoom;

	cache->w = fw * zoom;
	cache->h = fh * zoom;

	cjump = stride - cache->w;

	fdat_r = fr->rdat + fy * fr->w + fx;
	fdat_g = fr->gdat + fy * fr->w + fx;
	fdat_b = fr->bdat + fy * fr->w + fx;

	for (y = 0; y < fh; y++) {
		for (x = 0; x < fw; x++) {
			lndx = gain * (*fdat_r++ - flr);
			clamp_int (&lndx, 0, LUT_SIZE - 1);
			pix = (map->lut[lndx] >> 8);
			val = (pix << 16);

			lndx = gain * (*fdat_g++ - flr);
			clamp_int (&lndx, 0, LUT_SIZE - 1);
			pix = (map->lut[lndx] >> 8);
			val += (pix << 8);

			lndx = gain * (*fdat_b++ - flr);
			clamp_int (&lndx, 0, LUT_SIZE - 1);
			pix = (map->lut[lndx] >> 8);
			val += pix;

			for (z = 0; z < zoom; z++) {
				*cdat++ = val;
			}
		}
		fdat_r += fjump;
		fdat_g += fjump;
		fdat_b += fjump;
		cdat   += cjump;

		for (z = 0; z < zoom - 1; z++) {
			memcpy(cdat, cdat - stride, cache->w * sizeof(guint32));
			cdat += stride;
		}
	}
	cache->valid = 1;
}

static void cache_render_rgb_float_zo(struct map_cache *cache, struct frame_map *map,
				      int zoom, int fx, int fy, int fw, int fh)
{
	struct ccd_frame *fr = map->fr;
	int fjump;
	int x, y;
	float *fdat_r, *fdat_g, *fdat_b;
	float *fd0_r, *fd0_g, *fd0_b;
	int lndx;
	float gain = LUT_SIZE / (map->hcut - map->lcut);
	float flr = map->lcut;

	guint32 *cdat = (guint32 *) cairo_image_surface_get_data (cache->data);
	int stride = cairo_image_surface_get_stride (cache->data) / sizeof(guint32); /* in 32 bit ints */

	guint32 val;

	float avgf = 1.0 / (zoom * zoom);
	float pixf_r, pixf_g, pixf_b;

	int fj2, xx, yy;
	int cjump;

	cache->x = fx / zoom;
	cache->y = fy / zoom;

	cache->w = fw / zoom;
	cache->h = fh / zoom;

	fdat_r = fr->rdat + fy * fr->w + fx;
	fdat_g = fr->gdat + fy * fr->w + fx;
	fdat_b = fr->bdat + fy * fr->w + fx;

        fjump = fr->w - fw + (zoom - 1) * fr->w;
        fj2 = fr->w - zoom; /* jump from last pixel in zoom row to first one in the next row */

        cjump = stride - cache->w;

	for (y = 0; y < cache->h; y++) {
		for (x = 0; x < cache->w; x++) {
			fd0_r = fdat_r;
			fd0_g = fdat_g;
			fd0_b = fdat_b;

			pixf_r = pixf_g = pixf_b = 0.0;

			for (yy = 0; yy < zoom; yy++) {
				for (xx = 0; xx < zoom; xx++) {
					pixf_r += *fdat_r++;
					pixf_g += *fdat_g++;
					pixf_b += *fdat_b++;
				}

				fdat_r += fj2;
				fdat_g += fj2;
				fdat_b += fj2;
			}

			fdat_r = fd0_r + zoom;
			fdat_g = fd0_g + zoom;
			fdat_b = fd0_b + zoom;

			lndx = floor(gain * (pixf_r * avgf - flr));
			clamp_int(&lndx, 0, LUT_SIZE - 1);
			val = (map->lut[lndx] >> 8);

                        lndx = floor(gain * (pixf_g * avgf - flr));
                        clamp_int(&lndx, 0, LUT_SIZE - 1);
                        val += (map->lut[lndx] >> 8) << 8;

                        lndx = floor(gain * (pixf_b * avgf - flr));
                        clamp_int(&lndx, 0, LUT_SIZE - 1);
                        val += (map->lut[lndx] >> 8);

                        *cdat++ = val;
		}
		fdat_r += fjump;
		fdat_g += fjump;
		fdat_b += fjump;
		cdat   += cjump;
	}

	cache->valid = 1;
}

/* render a float frame to the cache at zoom >= 1*/
static void cache_render_float_zi(struct map_cache *cache, struct frame_map *map,
			  int zoom, int fx, int fy, int fw, int fh)
{
	struct ccd_frame *fr = map->fr;
	int fjump = fr->w - fw;
	int cjump;
	int x, y, z;
	float *fdat;
	int lndx;
	float gain = LUT_SIZE / (map->hcut - map->lcut);
	float flr = map->lcut;

	guint32 *cdat = (guint32 *) cairo_image_surface_get_data(cache->data);
        int stride = cairo_image_surface_get_stride(cache->data) / sizeof(guint32); /* in 32bit ints */
        unsigned char pix;
	guint32 val;

	if (map->color) {
		cache_render_rgb_float_zi(cache, map, zoom, fx, fy, fw, fh);
		return;
	}

	cache->x = fx * zoom;
	cache->y = fy * zoom;

	cache->w = fw * zoom;
	cache->h = fh * zoom;

	cjump = stride - cache->w;

	fdat = fr->dat + fy * fr->w + fx;

	for (y = 0; y < fh; y++) {
		for (x = 0; x < fw; x++) {
			lndx = (gain * (*fdat++ - flr));
			clamp_int (&lndx, 0, LUT_SIZE - 1);
			pix = (map->lut[lndx] >> 8);

			val = (pix << 16) + (pix << 8) + pix;

			for (z = 0; z < zoom; z++) {
				*cdat++ = val;
			}
		}
		fdat += fjump;
		cdat += cjump;
		for (z = 0; z < zoom - 1; z++) {
			memcpy(cdat, cdat - stride, cache->w * sizeof(guint32));
			cdat += stride;
		}
	}

	cache->valid = 1;
}

static void cache_render_float_zo(struct map_cache *cache, struct frame_map *map,
			  int zoom, int fx, int fy, int fw, int fh)
{
	struct ccd_frame *fr = map->fr;
	int fjump;
	int x, y;
	float *fdat, *fd0;
	int lndx;
	float gain = LUT_SIZE / (map->hcut - map->lcut);
	float flr = map->lcut;

	guint32 *cdat = (guint32 *) cairo_image_surface_get_data(cache->data);
        int stride = cairo_image_surface_get_stride(cache->data) / sizeof(guint32); /* in 32bit ints */
        unsigned char pix;
	guint32 val;

	float avgf = 1.0 / (zoom * zoom);
	float pixf;

	int fj2, xx, yy;
	int cjump;

	if (map->color) {
		cache_render_rgb_float_zo(cache, map, zoom, fx, fy, fw, fh);
		return;
	}

	cache->x = fx / zoom;
	cache->y = fy / zoom;

	cache->w = fw / zoom;
	cache->h = fh / zoom;

	fdat = fr->dat + fy * fr->w + fx;
	fd0 = fdat;

	fjump = fr->w - fw + (zoom - 1) * fr->w;
	fj2 = fr->w - zoom; /* jump from last pixel in zoom row to first one in the next row */

	cjump = stride - cache->w;

	for (y = 0; y < cache->h; y++) {
		for (x = 0; x < cache->w; x++) {
			pixf = 0;
			fd0 = fdat;
			for (yy = 0; yy < zoom; yy++) {
				for (xx = 0; xx < zoom; xx++)
					pixf += *fdat++;
				fdat += fj2;
			}
			fdat = fd0 + zoom;

			lndx = floor(gain * (pixf * avgf - flr));
			clamp_int(&lndx, 0, LUT_SIZE - 1);

			pix = map->lut[lndx] >> 8;
			val = (pix << 16) + (pix << 8) + pix;

			*cdat++ = val;
		}
		fdat += fjump;
		cdat += cjump;
	}
	cache->valid = 1;
}


/*
 * compute the size a cache needs to be so that the give area can fit
 */
static unsigned area_fits_in_cache(cairo_rectangle_int_t *area, struct map_cache *cache)
{
	return (area->width + MAX_ZOOM <= cairo_image_surface_get_width(cache->data)) &&
                (area->height + MAX_ZOOM <= cairo_image_surface_get_height(cache->data));
}


/*
 * update the cache so that it contains a representation of the given area
 */
static void update_cache(struct map_cache *cache, struct frame_map *map,
			 cairo_rectangle_int_t *area)
{
	struct ccd_frame *fr;
	int fx, fy, fw, fh;
	int zoom_in = 1;
	int zoom_out = 1;

	if (map->zoom > 1.0 && map->zoom <= 16.0) {
		zoom_in = floor(map->zoom + 0.5);
	} else if (map->zoom < 1.0 && map->zoom >= (1.0 / 16.0)) {
		zoom_out = floor(1.0 / map->zoom + 0.5);
	}
	cache->zoom = map->zoom;

	fr = map->fr;
	if (! area_fits_in_cache (area, cache)) {
		/* free the cache and realloc */
		d3_printf("freeing old cache\n");
		cairo_surface_destroy (cache->data);
		cache->data = NULL;
	}

	if (cache->data == NULL) { /* we need to alloc (new) data area */
		cache->data = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
							  area->width + 2 * MAX_ZOOM,
							  area->height + 2 * MAX_ZOOM);
	}

	d3_printf("expose area is %d by %d starting at %d, %d\n",
		  area->width, area->height, area->x, area->y);

	/* calculate the frame coords for the exposed area */
	fx = area->x * zoom_out / zoom_in;
	fy = area->y * zoom_out / zoom_in;
	fw = area->width * zoom_out / zoom_in + (zoom_in > 1 ? 2 : 0);
	fh = area->height * zoom_out / zoom_in + (zoom_in > 1 ? 2 : 0);
	if (fx > fr->w - 1)
		fx = fr->w - 1;
	if (fx + fw > fr->w - 1)
		fw = fr->w - fx;
	if (fy > fr->h - 1)
		fy = fr->h - 1;
	if (fy + fh > fr->h - 1)
		fh = fr->h - fy;
	d3_printf("frame region: %d by %d at %d, %d\n", fw, fh, fx, fy);

	/* and now to the actual cache rendering. We call different
	   functions for each frame format / zoom mode combination */
	if (zoom_out > 1)
		cache_render_float_zo(cache, map, zoom_out, fx, fy, fw, fh);
	else
		cache_render_float_zi(cache, map, zoom_in, fx, fy, fw, fh);

	d3_printf("cache area is %d by %d starting at %d, %d\n",
		  cache->w, cache->h, cache->x, cache->y);
}

/* attach a frame to the given view for display
 * it ref's the frame, so the frame should be released after
 * frame_to_window is called
 */
int gcx_image_view_set_frame(GcxImageView *view, struct ccd_frame *fr)
{
	struct map_cache *cache = view->cache;
	struct frame_map *map = view->map;

	//if ((fr->magic & FRAME_VALID_RGB) == 0)
	//	bayer_interpolate(fr);

	cache->valid = 0;

	if (map->fr)
		release_frame(map->fr);

	get_frame(fr);
	map->fr = fr;

	if (fr->magic & FRAME_VALID_RGB)
		map->color = 1;

	/* set default cuts */
	if (!fr->stats.statsok)
		frame_stats(fr);

	set_default_channel_cuts(map);

	map->changed = 1;

	wcs_from_frame(fr, view->wcs);
	//wcsedit_refresh(window);

	map->width = fr->w;
	map->height = fr->h;

	set_view_size(view, 0.5, 0.5);

	//gtk_window_set_title (GTK_WINDOW (window), fr->name);
	//remove_stars(window, TYPE_MASK_FRSTAR, 0);

	//stats_cb(window, 0);
	//show_zoom_cuts(window);

	g_signal_emit (view, image_view_signals[FRAME_CHANGED], 0);
	g_signal_emit (view, image_view_signals[MAPPING_CHANGED], 0);

	gtk_widget_queue_draw(GTK_WIDGET(view));

	return 0;
}

/* return the mapped frame
 */
struct ccd_frame *
gcx_image_view_get_frame (GcxImageView *view)
{
	g_return_val_if_fail (GCX_IS_IMAGE_VIEW(view), 0);
	return view->map->fr;
}





/*
 * paint screen from cache
 */
static void
paint_from_cache (cairo_t *cr, struct map_cache *cache, cairo_rectangle_int_t *area)
{
        if (!area_in_cache(area, cache)) {
                err_printf("paint_from_cache: oops - area not in cache\n");
//              d3_printf("area is %d by %d starting at %d,%d\n",
//                        area->width, area->height, area->x, area->y);
//              d3_printf("cache is %d by %d starting at %d,%d\n",
//                        cache->w, cache->h, cache->x, cache->y);
//              return;
        }

	cairo_set_source_surface (cr, cache->data, (double) area->x, (double) area->y);
	cairo_paint (cr);
}


/*
 * a draw event to our image window
 */

static gboolean
gcx_image_view_draw_cb(GtkWidget *darea, cairo_t *cr, gpointer user)
{
	GcxImageView *iv = GCX_IMAGE_VIEW(user);
	struct map_cache *cache = iv->cache;
	struct frame_map *map = iv->map;
	cairo_rectangle_int_t area;

	if (map->fr == NULL) /* no frame */
		return TRUE;

	/* no clip rectangle means all is clipped */
	if (! gdk_cairo_get_clip_rectangle (cr, &area))
		return TRUE;

	/* invalidate cache if needed */
	if (cache->zoom != map->zoom)
		cache->valid = 0;

	if (map->changed)
		cache->valid = 0;

	if (!area_in_cache(&area, cache))
		cache->valid = 0;

	if (!cache->valid)
		update_cache(cache, map, &area);

	if (cache->valid) {
		d3_printf("cache valid, area is %d by %d starting at %d, %d\n",
			  cache->w, cache->h,
			  cache->x, cache->y);
		d3_printf("expose: from cache\n");

		paint_from_cache (cr, cache, &area);
	}

	draw_sources_hook(iv, cr, &area);

	map->changed = 0;

	return FALSE;
}

/* save a mapped image to a 8/16-bit pnm file. The image is curved to the
 * current cuts/lut
 * if fname is null, the file is output on stdout
 * return 0 for success */

int gcx_image_view_to_pnm_file(GcxImageView *iv, char *fn, int is_16bit)
{
	struct ccd_frame *fr = gcx_image_view_get_frame (iv);
	FILE *pnmf;
	int i;
	float *fdat;
	unsigned short pix;
	int lndx, all;
	float gain = LUT_SIZE / (iv->map->hcut - iv->map->lcut);
	float flr = iv->map->lcut;

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

		pix = iv->map->lut[lndx];
		fdat ++;
		putc(pix >> 8, pnmf);
		if (is_16bit)
			putc(pix & 0xff, pnmf);
	}

	if (pnmf != stdout)
		fclose(pnmf);

	return 0;
}


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

/* action values for view (zoom/pan) callback */
#define VIEW_ZOOM_IN 0x100
#define VIEW_ZOOM_OUT 0x200
#define VIEW_ZOOM_FIT 0x300
#define VIEW_PIXELS 0x400
#define VIEW_PAN_CENTER 0x500
#define VIEW_PAN_CURSOR 0x600


static void channel_set_lut_from_gamma(struct frame_map *map);


static void drag_adjust_cuts(GtkWidget *widget, int dx, int dy)
{
	GcxImageView *iv = GCX_IMAGE_VIEW (widget);
	struct frame_map *map = iv->map;
	double base;
	double span;

	if (iv->map->fr == NULL) {
		err_printf("drag_adjust_cuts: no frame in i_channel\n");
		return ;
	}

	span = map->hcut - map->lcut;
	base = map->lcut + map->avg_at * span;

	if (map->invert)
		dx = -dx;

	if (dy > 30)
		dy = 30;
	if (dy < -30)
		dy = -30;
	if (dx > 30)
		dx = 30;
	if (dx < -30)
		dx = -30;

	if (dy > 0 || span > MIN_SPAN)
		span = span * (1.0 + dy * CONTRAST_STEP_DRAG);
	map->lcut = base - span * map->avg_at - dx * BRIGHT_STEP_DRAG * span;
	map->hcut = base + span * (1 - map->avg_at)- dx * BRIGHT_STEP_DRAG * span;

//	d3_printf("new cuts: lcut:%.0f hcut:%.0f\n", map->lcut, map->hcut);
	map->changed = 1;
	//show_zoom_cuts(window);
	gtk_widget_queue_draw(iv->darea);
}

#define DRAG_MIN_MOVE 2
static gboolean gcx_image_view_motion_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer window)
{
	int x, y, dx, dy;
	GdkModifierType state;
	static int ox = 0, oy = 0;

	if (event->is_hint)
		gdk_window_get_pointer (event->window, &x, &y, &state);
	else
	{
		x = event->x;
		y = event->y;
		state = event->state;
	}
//	d3_printf("motion %d %d state %d\n", x - ox, y - oy, state);
//	show_xy_status(window, 1.0 * x, 1.0 * y);
	if (state & GDK_BUTTON1_MASK) {
		dx = x - ox;
		dy = y - oy;
//		printf("moving by %d %d\n", x - ox, y - oy);
		if (abs(dx) > DRAG_MIN_MOVE || abs(dy) > DRAG_MIN_MOVE) {
			if (dx > DRAG_MIN_MOVE)
				dx -= DRAG_MIN_MOVE;
			else if (dx < -DRAG_MIN_MOVE)
				dx += DRAG_MIN_MOVE;
			else
				dx = 0;
			if (dy > DRAG_MIN_MOVE)
				dy -= DRAG_MIN_MOVE;
			else if (dy < -DRAG_MIN_MOVE)
				dy += DRAG_MIN_MOVE;
			else
				dy = 0;
			drag_adjust_cuts(widget, dx, dy);
			ox = x;
			oy = y;
		}
	} else {
		ox = x;
		oy = y;
	}

	return TRUE;
}


/*
 * change channel cuts according to gui action
 */
static void channel_cuts_action(struct frame_map *map, int action)
{
	int act;
	int val;
	double base;
	double span;
	struct ccd_frame *fr = map->fr;

	span = map->hcut - map->lcut;
	base = map->lcut + map->avg_at * span;

	act = action & CUTS_ACT_MASK;
	val = action & CUTS_VAL_MASK;

	switch(act) {
	case CUTS_MINMAX:
		if (!fr->stats.statsok) {
			frame_stats(fr);
		}
		map->lcut = fr->stats.min;
		map->hcut = fr->stats.max;
		break;
	case CUTS_FLATTER:
		span = span * CONTRAST_STEP;
		map->lcut = base - span * map->avg_at;
		map->hcut = base + span * (1 - map->avg_at);
		break;
	case CUTS_SHARPER:
		span = span / CONTRAST_STEP;
		map->lcut = base - span * map->avg_at;
		map->hcut = base + span * (1 - map->avg_at);
		break;
	case CUTS_BRIGHTER:
		if (!map->invert) {
			map->lcut -= span * BRIGHT_STEP;
			map->hcut -= span * BRIGHT_STEP;
		} else {
			map->lcut += span * BRIGHT_STEP;
			map->hcut += span * BRIGHT_STEP;
		}
		break;
	case CUTS_DARKER:
		if (!map->invert) {
			map->lcut += span * BRIGHT_STEP;
			map->hcut += span * BRIGHT_STEP;
		} else {
			map->lcut -= span * BRIGHT_STEP;
			map->hcut -= span * BRIGHT_STEP;
		}
		break;
	case CUTS_INVERT:
		map->invert = !map->invert;
		channel_set_lut_from_gamma(map);
		d3_printf("invert is %d\n", map->invert);
		break;
	case CUTS_AUTO:
		val = DEFAULT_SIGMAS;
		/* fallthrough */
	case CUTS_CONTRAST:
		map->avg_at = DEFAULT_AVG_AT;
		if (!fr->stats.statsok) {
			frame_stats(fr);
		}
		map->davg = fr->stats.cavg;
		map->dsigma = fr->stats.csigma * 2;
		if (val < 0)
			val = 0;
		if (val >= SIGMAS_VALS)
			val = SIGMAS_VALS - 1;
		map->lcut = map->davg - map->avg_at
			* sigmas[val] * map->dsigma;
		map->hcut = map->davg + (1.0 - map->avg_at)
			* sigmas[val] * map->dsigma;
		break;
	default:
		err_printf("unknown cuts action %d, ignoring\n", action);
		return;
	}
	d3_printf("new cuts: lcut:%.0f hcut:%.0f\n", map->lcut, map->hcut);
	map->changed = 1;
}

static void set_default_channel_cuts(struct frame_map* map)
{
	channel_cuts_action(map, CUTS_AUTO);
}

/* pan the image in the scrolled window
 * so that the center of the viewable area is at xc and yc
 * of the image's width/height
 */

static void
__set_scroll(GtkAdjustment *adjustment, gdouble position)
{
	gdouble value, lower, upper, page_size;

	g_object_get (G_OBJECT(adjustment),
		      "lower",     &lower,
		      "upper",     &upper,
		      "page-size", &page_size,
		      NULL);

	value = (upper - lower) * position + lower - page_size / 2;
	clamp_double(&value, lower, upper - page_size);

	g_object_set (G_OBJECT(adjustment), "value", value, NULL);
}

static void
gcx_image_view_set_scrolls(GcxImageView *view, double xc, double yc)
{
	__set_scroll (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(view)), xc);
	__set_scroll (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(view)), yc);
}


/* return the position of the scrollbars as
 * the fraction of the image's dimension the center of the
 * visible area is at
 */
static void
__get_scroll(GtkAdjustment *adjustment, gdouble *position)
{
	gdouble value, lower, upper, page_size;

	g_object_get (G_OBJECT(adjustment),
		      "value",     &value,
		      "lower",     &lower,
		      "upper",     &upper,
		      "page-size", &page_size,
		      NULL);

	*position = (value + page_size / 2) / (upper - lower);
}

static void
gcx_image_view_get_scrolls(GcxImageView *view, double *xc, double *yc)
{
	__get_scroll (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(view)), xc);
	__get_scroll (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(view)), yc);
}


double
gcx_image_view_get_zoom (GcxImageView *iv)
{
	return iv->map->zoom;
}

struct gui_star_list *
gcx_image_view_get_stars (GcxImageView *iv)
{
	return iv->stars;
}

void
gcx_image_view_set_stars (GcxImageView *iv, struct gui_star_list *stars)
{
	iv->stars = stars;
}

cairo_t *
gcx_image_view_cairo_surface (GcxImageView *iv)
{
	GdkWindow *gdkw = gtk_widget_get_window (iv->darea);
	cairo_t *cr;

	cr = gdk_cairo_create (gdkw);

	return cr;
}

static void set_view_size(GcxImageView *view, double xc, double yc)
{
	GdkWindow *window;
	GtkAdjustment *adj;
	gdouble value, lower, upper, page_size;
	int zi, zo;
	int w, h;

	if (!gtk_widget_get_realized (GTK_WIDGET(view)))
		return;

	if (view->map->zoom > 1) {
		zo = 1;
		zi = floor(view->map->zoom + 0.5);
	} else {
		zi = 1;
		zo = floor(1.0 / view->map->zoom + 0.5);
	}

	window = gtk_widget_get_window (GTK_WIDGET(view->darea));
	//gdk_drawable_get_size (window, &w, &h);
	w = gdk_window_get_width (window);
	h = gdk_window_get_height (window);

	d3_printf("scw size id %d %d\n", w, h);

	gdk_window_freeze_updates(window);

	gtk_widget_set_size_request(GTK_WIDGET(view->darea), view->map->width * zi / zo,
				    view->map->height * zi / zo);

/* we need this to make sure the drawing area contracts properly
 * when we go from scrollbars to no scrollbars, we have to be sure
 * everything happens in sequnece */

	if (w >= view->map->width * zi / zo || h >= view->map->height * zi / zo) {
		gtk_widget_queue_resize (GTK_WIDGET(view));
		while (gtk_events_pending ())
			gtk_main_iteration ();
	}

	adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(view));
	g_object_get (G_OBJECT(adj), "value", &value, "lower", &lower, "upper", &upper,
		      "page-size", &page_size, NULL);

	if (page_size < view->map->width * zi / zo) {
		upper = view->map->width * zi / zo;
		if (upper < page_size)
			upper = page_size;

		value = (upper - lower) * xc + lower - page_size / 2;
		clamp_double(&value, lower, upper - page_size);

		g_object_set (G_OBJECT(adj), "upper", upper, "value", value, NULL);
	}

	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(view));
	if (page_size < view->map->height * zi / zo) {
		upper = view->map->height * zi / zo;
		if (upper < page_size)
			upper = page_size;

		value = (upper - lower) * yc + lower - page_size / 2;
		clamp_double(&value, lower, upper - page_size);

		g_object_set (G_OBJECT(adj), "upper", upper, "value", value, NULL);
	}

	gtk_widget_queue_draw(GTK_WIDGET(view));

	gdk_window_thaw_updates(window);
//	d3_printf("at end of darea_size\n");
}

/*
 * step the zoom up/down
 */
static void step_zoom(struct frame_map *map, int step)
{
	double zoom = map->zoom;

	if (zoom < 1) {
		zoom = floor(1.0 / zoom + 0.5);
	} else {
		zoom = floor(zoom + 0.5);
	}

	if (zoom == 1.0) {
		if (step > 0)
			map->zoom = 2.0;
		else
			map->zoom = 0.5;
		return;
	}

	if (map->zoom < 1)
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
	if (zoom > MAX_ZOOM)
		zoom = MAX_ZOOM;

	if (map->zoom < 1) {
		map->zoom = 1.0 / zoom;
	} else {
		map->zoom = zoom;
	}
}

/*
 * pan window so that the pixel pointed by the cursor is centered
 */
void pan_cursor(GtkWidget *window)
{
	GcxImageView *iv;
	int x, y, w, h;
	GdkModifierType mask;
	GdkWindow *gdkw;

	iv = GCX_IMAGE_VIEW(g_object_get_data(G_OBJECT(window), "image_view"));

	gdkw = gtk_widget_get_window (iv->darea);

	gdk_window_get_pointer (gdkw, &x, &y, &mask);
	w = gdk_window_get_width (gdkw);
	h = gdk_window_get_height (gdkw);
	//gdk_drawable_get_size (gtk_widget_get_window (iv->darea), &w, &h);

	d3_printf("mouse x=%d, y=%d drawable w=%d, h=%d\n", x, y, w, h);

	gcx_image_view_set_scrolls(iv, 1.0 * x / w, 1.0 * y / h);
}




/**********************
 * callbacks from gui
 **********************/

/*
 * changing cuts (brightness/contrast)
 */
static void cuts_option_cb(gpointer data, guint action)
{
	GtkWidget *window = data;
	GcxImageView *view;

	view = g_object_get_data(G_OBJECT(window), "image_view");

	channel_cuts_action(view->map, action);
	show_zoom_cuts(window);

	gtk_widget_queue_draw(GTK_WIDGET(view->darea));
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
	view_option_cb (data, VIEW_PIXELS);
}

void act_view_pan_center (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_PAN_CENTER);
}

void act_view_pan_cursor (GtkAction *action, gpointer data)
{
	view_option_cb (data, VIEW_PAN_CURSOR);
}

/*
 * display image stats in status bar
 * does not use action or menu_item
 */
void stats_cb(gpointer data, guint action)
{
#if 0
	gpointer ret;
	struct image_channel *i_channel;

	ret = g_object_get_data(G_OBJECT(data), "i_channel");
	if (ret == NULL) /* no channel */
		return;
	i_channel = ret;
	if (i_channel->fr == NULL) /* no frame */
		return;
	if (!(i_channel->fr)->stats.statsok)
		frame_stats(i_channel->fr);

	info_printf_sb2(data, "Frame: %dx%d   avg:%.0f sigma:%.1f min:%.0f max:%.0f",
		i_channel->fr->w, i_channel->fr->h,
		i_channel->fr->stats.cavg, i_channel->fr->stats.csigma,
		i_channel->fr->stats.min, i_channel->fr->stats.max );
#endif
}

/*
 * display stats in region around cursor
 * does not use action or menu_item
 */
void show_region_stats(GtkWidget *window, double x, double y)
{
#if 0
	char buf[180];
	gpointer ret;
	struct image_channel *i_channel;
	struct map_geometry *geom;
	struct rstats rs;
	int xi, yi;
	float val;

	ret = g_object_get_data(G_OBJECT(window), "i_channel");
	if (ret == NULL) /* no channel */
		return;
	i_channel = ret;
	if (i_channel->fr == NULL) /* no frame */
		return;

	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;

	x /= geom->zoom;
	y /= geom->zoom;

	xi = x;
	yi = y;

	val = get_pixel_luminence(i_channel->fr, xi, yi);
	ring_stats(i_channel->fr, x, y, 0, 10, 0xf, &rs, -HUGE, HUGE);

	if (i_channel->fr->magic & FRAME_VALID_RGB) {
		sprintf(buf, "[%d,%d]=%.1f (%.1f, %.1f, %.1f)  Region: Avg:%.0f  Sigma:%.1f  Min:%.0f  Max:%.0f",
			xi, yi, val,
			*(((float *) i_channel->fr->rdat) + xi + yi * i_channel->fr->w),
			*(((float *) i_channel->fr->gdat) + xi + yi * i_channel->fr->w),
			*(((float *) i_channel->fr->bdat) + xi + yi * i_channel->fr->w),
			rs.avg, rs.sigma, rs.min, rs.max );
	} else {
		sprintf(buf, "[%d,%d]=%.1f  Region: Avg:%.0f  Sigma:%.1f  Min:%.0f  Max:%.0f",
			xi, yi, val, rs.avg, rs.sigma, rs.min, rs.max );
	}

	info_printf_sb2(window, "%s\n", buf);
#endif
}

/*
 * show zoom/cuts in statusbar
 */
void show_zoom_cuts(GtkWidget * window)
{
#if 0
	char buf[180];
	GtkWidget *statuslabel, *dialog;
	gpointer ret;
	struct image_channel *i_channel;
	struct map_geometry *geom;
	void imadj_dialog_update(GtkWidget *dialog);

	statuslabel = g_object_get_data(G_OBJECT(window), "statuslabel1");
	if (statuslabel == NULL)
		return;
	ret = g_object_get_data(G_OBJECT(window), "i_channel");
	if (ret == NULL) /* no channel */
		return;
	i_channel = ret;

	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;

	sprintf(buf, " Z:%.2f  Lcut:%.0f  Hcut:%.0f",
		geom->zoom, i_channel->lcut, i_channel->hcut);

	gtk_label_set_text(GTK_LABEL(statuslabel), buf);
/* see if we have a imadjust dialog and update it, too */
	dialog = g_object_get_data(G_OBJECT(window), "imadj_dialog");
	if (dialog == NULL)
		return;
	imadj_dialog_update(dialog);
#endif
}

#define T_START_GAMMA 0.2

/* set LUT from the gamma and toe */
static void channel_set_lut_from_gamma(struct frame_map *map)
{
	double x, y;
	int i;

	d3_printf("set lut from gamma\n");
	for (i=0; i<LUT_SIZE; i++) {
		double x0, x1, g;
		x = 1.0 * (i) / LUT_SIZE;
		x0 = map->toe;
		x1 = map->gamma * map->toe;
//		d3_printf("channel offset is %f\n", map->offset);
		if (x0 < 0.001) {
			y = 65535 * (map->offset + (1 - map->offset) *
				     pow(x, 1.0 / map->gamma));
		} else {
			g = (x + x1) * x0 / ((x + x0) * x1);
			y = 65535 * (map->offset + (1 - map->offset) * pow(x, g));
		}
		if (!map->invert)
			map->lut[i] = y;
		else
			map->lut[LUT_SIZE - i - 1] = y;
	}
}

/* draw the histogram/curve */
#define CUTS_FACTOR 0.8 /* how much of the histogram is between the cuts */
/* rebin a histogram region from low to high into dbins bins */
void rebin_histogram(struct im_histogram *hist, double *rbh, int dbins,
		double low, double high, double *max)
{
	int i, hp, dp;
	int leader = 0;
	double hbinsize, dbinsize;
	double v, ri, dri;

	d3_printf("rebin between %.3f -> %.3f\n", low, high);

	hbinsize = (hist->end - hist->st) / hist->hsize;
	dbinsize = (high - low) / dbins;

	*max = 0.0;
	if (low < hist->st) {
		leader = floor(dbins * ((hist->st - low) / (high - low)));
		d3_printf("leader is %d\n", leader);
		for (i=0; i<leader && i < dbins; i++)
			rbh[i] = 0.0;
		dbins -= i;
		rbh += i;
		low = hist->st;
	}
	hp = floor(hist->hsize * ((low - hist->st) / (hist->end - hist->st)));
	dp = 0;
	ri = 0.0;
	d3_printf("hist_hsize = %d, dbinsize = %.3f, hbinsize = %.3f hp=%d\n", hist->hsize,
		  dbinsize, hbinsize, hp);
	while (hp < hist->hsize && dp < dbins) {
		dri = dbinsize;
//		d3_printf("hp = %d, hist = %d\n", hp, hist->hdat[hp]);
		v = hist->hdat[hp] * ri / hbinsize; /* remainder from last bin */
		dri -= ri;
		while (dri > hbinsize && hp < hist->hsize) { /* whole bins */
			hp ++;
			v += hist->hdat[hp];
			dri -= hbinsize;
		}
		if (hp == hist->hsize) {
			rbh[dp++] = v;
			if (v > *max)
				*max = v;
			break;
		}
		/* last (partial) bin */
		hp ++;
		ri = hbinsize - dri;
		v += hist->hdat[hp] * (1 - ri) / hbinsize;
		if (v > *max)
			*max = v;
		rbh[dp++] = v;
	}
	for (; dp < dbins; dp++) /* fill the end with zeros */
		rbh[dp] = 0.0;
}

/* scale the histogram so all vales are between 0 and 1
 */
#define LOG_FACTOR 5.0
void scale_histogram(double *rbh, int dbins, double max, int logh)
{
	double lv, lm;
	int i;
//	d3_printf("max is %f, log is %d\n", max, logh);
	if (logh) {
		if (max > 1) {
			lm = (1 + LOG_FACTOR * log(max));
		} else {
			lm = 1;
		}
		for (i = 0; i < dbins; i++) {
			if (rbh[i] < 1) {
				lv = 0;
			} else {
				lv = (1 + LOG_FACTOR * log(rbh[i])) / lm;
			}
			rbh[i] = lv;
		}
	} else {
		for (i = 0; i < dbins; i++) {
			rbh[i] = rbh[i] / max;
		}
	}
}

void plot_histogram(GtkWidget *darea, GdkRectangle *area,
		    int dskip, double *rbh, int dbins)
{
	GtkAllocation allocation;
	int firstx, lcx, hcx;
	int i, h;
	cairo_t *cr;

	cr = gdk_cairo_create (gtk_widget_get_window (GTK_WIDGET(darea)));

	gtk_widget_get_allocation (GTK_WIDGET(darea), &allocation);

	firstx = area->x / dskip;
	lcx = allocation.width * (1 - CUTS_FACTOR) / 2;
	hcx = allocation.width - allocation.width * (1 - CUTS_FACTOR) / 2;

	d3_printf("aw=%d lc=%d hc=%d\n", allocation.width, lcx, hcx);

        /* clear the area */
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_rectangle (cr, area->x, 0, area->width, allocation.height);
	cairo_fill (cr);

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

	for (i = firstx; i < (area->x + area->width) / dskip + 1 && i < dbins; i++) {
		h = allocation.height - rbh[i] * allocation.height;
//		d3_printf("hist line v=%.3f x=%d  h=%d\n", rbh[i], i, h);
		cairo_move_to (cr, i * dskip, allocation.height);
		cairo_line_to (cr, i * dskip, h);
		cairo_stroke (cr);
	}

	cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);

	cairo_move_to (cr, lcx, allocation.height);
	cairo_line_to (cr, lcx, 0);
	cairo_stroke (cr);

	cairo_move_to (cr, hcx, allocation.height);
	cairo_line_to (cr, hcx, 0);
	cairo_stroke (cr);

	cairo_destroy (cr);
}

/* draw the curve over the histogram area */
void plot_curve(GtkWidget *darea, GdkRectangle *area, struct frame_map *map)
{
	GtkAllocation allocation;
	int lcx, hcx, span;
	int i, ci;
	cairo_t *cr;

	cr = gdk_cairo_create (gtk_widget_get_window (GTK_WIDGET(darea)));

	gtk_widget_get_allocation (GTK_WIDGET(darea), &allocation);

	cairo_set_source_rgb (cr, 0.15, 0.60, 0.15);
	cairo_set_line_width (cr, 2.0);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

	lcx = allocation.width * (1 - CUTS_FACTOR) / 2;
	hcx = allocation.width - allocation.width * (1 - CUTS_FACTOR) / 2;
	span = hcx - lcx;

	cairo_move_to (cr, 0, allocation.height -
		       allocation.height * map->lut[0] / 65536);
	for (i = 0; i < span; i++) {
		ci = LUT_SIZE * i / span;

		cairo_line_to (cr, lcx + i, allocation.height -
			       allocation.height * map->lut[ci] / 65536);
	}
	cairo_stroke (cr);

	cairo_destroy (cr);
}


/* draw a piece of a histogram */
void draw_histogram(GtkWidget *darea, GdkRectangle *area, struct frame_map *map,
		    double low, double high, int logh)
{
	GtkAllocation allocation;
	struct im_histogram *hist;
	double *rbh, max = 0.0;
	double hbinsize, dbinsize;
	int dskip, dbins;

	if (!map->fr->stats.statsok) {
		err_printf("draw_histogram: no stats\n");
		return;
	}

	gtk_widget_get_allocation (GTK_WIDGET(darea), &allocation);

	hist = &(map->fr->stats.hist);
	hbinsize = (hist->end - hist->st) / hist->hsize;
	dbinsize = (high - low) / allocation.width;
	if (dbinsize < hbinsize) {
		dskip = 1 + floor(hbinsize / dbinsize);
	} else {
		dskip = 1;
	}
	dbins = allocation.width / dskip + 1;
	rbh = calloc(dbins, sizeof(double));
	if (rbh == NULL)
		return;
	rebin_histogram(hist, rbh, dbins, low, high, &max);
	scale_histogram(rbh, dbins, max, logh);
	plot_histogram(darea, area, dskip, rbh, dbins);
	plot_curve(darea, area, map);
}


gboolean histogram_expose_cb(GtkWidget *darea, GdkEventExpose *event, gpointer dialog)
{
	struct frame_map *map;
	GtkWidget *logckb;
	double low, high;
	int logh = 0;

	map = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (map == NULL) /* no map */
		return 0 ;

	logckb = g_object_get_data(G_OBJECT(dialog), "log_hist_check");
	if (logckb != NULL) {
		logh = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logckb));
	}

	low = map->lcut - (map->hcut - map->lcut) * (1 / CUTS_FACTOR - 1) / 2;
	high = map->hcut + (map->hcut - map->lcut) * (1 / CUTS_FACTOR - 1) / 2;;

	draw_histogram(darea, &event->area, map, low, high, logh);

	return 0 ;
}

void update_histogram(GtkWidget *dialog)
{
	GtkWidget *darea;

	darea = g_object_get_data(G_OBJECT(dialog), "hist_area");
	if (darea == NULL)
		return;
	gtk_widget_queue_draw(darea);
}


/* set the value in a named spinbutton */
void spin_set_value(GtkWidget *dialog, char *name, float val)
{
	GtkWidget *spin;
	spin = g_object_get_data(G_OBJECT(dialog), name);
	if (spin == NULL) {
		g_warning("cannot find spin button named %s\n", name);
		return;
	}
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), val);
}

/* get the value in a named spinbutton */
double spin_get_value(GtkWidget *dialog, char *name)
{
	GtkWidget *spin;
	spin = g_object_get_data(G_OBJECT(dialog), name);
	if (spin == NULL) {
		g_warning("cannot find spin button named %s\n", name);
		return 0.0;
	}
	return gtk_spin_button_get_value (GTK_SPIN_BUTTON(spin));
}

void imadj_cuts_updated (GtkWidget *spinbutton, gpointer dialog)
{
	struct frame_map *map;
	GtkWidget *window;

	map = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (map == NULL) /* no channel */
		return;
	window = g_object_get_data(G_OBJECT(dialog), "image_window");

	map->lcut = spin_get_value(dialog, "low_cut_spin");
	map->hcut = spin_get_value(dialog, "high_cut_spin");

	if (map->hcut <= map->lcut) {
		map->hcut = map->lcut + 1;
	}

	map->changed = 1;
	show_zoom_cuts(window);
	update_histogram(dialog);
	gtk_widget_queue_draw(window);
}

void log_toggled (GtkWidget *spinbutton, gpointer dialog)
{
	update_histogram(dialog);
}

void imadj_lut_updated (GtkWidget *spinbutton, gpointer dialog)
{
	struct frame_map *map;
	GtkWidget *window;

	map = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (map == NULL) /* no map */
		return;
	window = g_object_get_data(G_OBJECT(dialog), "image_window");

	map->gamma = spin_get_value(dialog, "gamma_spin");
	map->toe = spin_get_value(dialog, "toe_spin");
	map->offset = spin_get_value(dialog, "offset_spin");

	channel_set_lut_from_gamma(map);

	map->changed = 1;
	show_zoom_cuts(window);
	update_histogram(dialog);
	gtk_widget_queue_draw(window);

}


void imadj_set_callbacks(GtkWidget *dialog)
{
	GtkWidget *spin, *logckb, *button, *window;
	GtkAdjustment *adj;

	spin = g_object_get_data(G_OBJECT(dialog), "low_cut_spin");
	if (spin == NULL) return;
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			   G_CALLBACK (imadj_cuts_updated), dialog);

	spin = g_object_get_data(G_OBJECT(dialog), "high_cut_spin");
	if (spin == NULL) return;
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			   G_CALLBACK (imadj_cuts_updated), dialog);

	spin = g_object_get_data(G_OBJECT(dialog), "gamma_spin");
	if (spin == NULL) return;
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			   G_CALLBACK (imadj_lut_updated), dialog);

	spin = g_object_get_data(G_OBJECT(dialog), "toe_spin");
	if (spin == NULL) return;
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			   G_CALLBACK (imadj_lut_updated), dialog);

	spin = g_object_get_data(G_OBJECT(dialog), "offset_spin");
	if (spin == NULL) return;
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin));
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
	g_signal_connect(G_OBJECT(adj), "value_changed",
			   G_CALLBACK (imadj_lut_updated), dialog);

	logckb = g_object_get_data(G_OBJECT(dialog), "log_hist_check");
	if (logckb != NULL) {
		g_signal_connect(G_OBJECT(logckb), "toggled",
			   G_CALLBACK (log_toggled), dialog);
	}

	window = g_object_get_data(G_OBJECT(dialog), "image_window");

	/* FIXME: we use the action functions; they're ignoring the
	   first argument, which will be a GtkButton in this case.

	   This should be handled by gtk_action_connect_proxy^H^H^H^H
	   gtk_activatable_set_related_action, but I don't see a
	   convenient way to get to the actual GtkAction.
	*/
	button = g_object_get_data(G_OBJECT(dialog), "cuts_darker");
	if (button != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK (act_view_cuts_darker), window);
	}
	button = g_object_get_data(G_OBJECT(dialog), "cuts_sharper");
	if (button != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
				 G_CALLBACK (act_view_cuts_sharper), window);
	}
	button = g_object_get_data(G_OBJECT(dialog), "cuts_duller");
	if (button != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
			   G_CALLBACK (act_view_cuts_flatter), window);
	}
	button = g_object_get_data(G_OBJECT(dialog), "cuts_auto");
	if (button != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
			   G_CALLBACK (act_view_cuts_auto), window);
	}
	button = g_object_get_data(G_OBJECT(dialog), "cuts_min_max");
	if (button != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
			   G_CALLBACK (act_view_cuts_minmax), window);
	}
	button = g_object_get_data(G_OBJECT(dialog), "cuts_brighter");
	if (button != NULL) {
		g_signal_connect(G_OBJECT(button), "clicked",
			   G_CALLBACK (act_view_cuts_brighter), window);
	}

}

void imadj_dialog_update(GtkWidget *dialog)
{
	struct frame_map *map;
	double lcut, hcut, gamma, toe, offset;

	map = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (map == NULL) /* no channel */
		return;
	lcut = map->lcut;
	hcut = map->hcut;
	gamma = map->gamma;
	toe = map->toe;
	offset = map->offset;
	spin_set_value(dialog, "low_cut_spin", lcut);
	spin_set_value(dialog, "offset_spin", offset);
	spin_set_value(dialog, "high_cut_spin", hcut);
	spin_set_value(dialog, "gamma_spin", gamma);
	spin_set_value(dialog, "toe_spin", toe);
	update_histogram(dialog);
}

void imadj_dialog_edit(GtkWidget *dialog)
{

}

void close_imadj_dialog( GtkWidget *widget, gpointer data )
{
	g_object_set_data(G_OBJECT(data), "imadj_dialog", NULL);
}


void act_control_histogram(GtkAction *action, gpointer window)
{
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
		gdk_window_raise (gtk_widget_get_window(GTK_WIDGET(dialog)));
	}
	//ref_image_channel(i_channel);
	//g_object_set_data_full(G_OBJECT(dialog), "i_channel", i_channel,
	//			 (GDestroyNotify)release_image_channel);
	imadj_dialog_update(dialog);
	imadj_dialog_edit(dialog);
}
