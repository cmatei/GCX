/*******************************************************************************
  Copyright(c) 2000 - 2003 Radu Corlan. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Contact Information: radu@corlan.net
*******************************************************************************/

/* functions that adjust an image's pan, zoom and display brightness */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"

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


void channel_set_lut_from_gamma(struct image_channel *channel);


void drag_adjust_cuts(GtkWidget *window, int dx, int dy)
{
	struct image_channel* channel;
	double base;
	double span;

	channel = g_object_get_data(G_OBJECT(window), "i_channel");
	if (channel == NULL) {
		err_printf("drag_adjust_cuts: no i_channel\n");
		return ;
	}
	if (channel->fr == NULL) {
		err_printf("drag_adjust_cuts: no frame in i_channel\n");
		return ;
	}

	span = channel->hcut - channel->lcut;
	base = channel->lcut + channel->avg_at * span;

	if (channel->invert)
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
	channel->lcut = base - span * channel->avg_at - dx * BRIGHT_STEP_DRAG * span;
	channel->hcut = base + span * (1 - channel->avg_at)- dx * BRIGHT_STEP_DRAG * span;

//	d3_printf("new cuts: lcut:%.0f hcut:%.0f\n", channel->lcut, channel->hcut);
	channel->channel_changed = 1;
	show_zoom_cuts(window);
	gtk_widget_queue_draw(window);
}




/*
 * change channel cuts according to gui action
 */
static void channel_cuts_action(struct image_channel *channel, int action)
{
	int act;
	int val;
	double base;
	double span;
	struct ccd_frame *fr = channel->fr;

	span = channel->hcut - channel->lcut;
	base = channel->lcut + channel->avg_at * span;

	act = action & CUTS_ACT_MASK;
	val = action & CUTS_VAL_MASK;

	switch(act) {
	case CUTS_MINMAX:
		if (!fr->stats.statsok) {
			frame_stats(fr);
		}
		channel->lcut = fr->stats.min;
		channel->hcut = fr->stats.max;
		break;
	case CUTS_FLATTER:
		span = span * CONTRAST_STEP;
		channel->lcut = base - span * channel->avg_at;
		channel->hcut = base + span * (1 - channel->avg_at);
		break;
	case CUTS_SHARPER:
		span = span / CONTRAST_STEP;
		channel->lcut = base - span * channel->avg_at;
		channel->hcut = base + span * (1 - channel->avg_at);
		break;
	case CUTS_BRIGHTER:
		if (!channel->invert) {
			channel->lcut -= span * BRIGHT_STEP;
			channel->hcut -= span * BRIGHT_STEP;
		} else {
			channel->lcut += span * BRIGHT_STEP;
			channel->hcut += span * BRIGHT_STEP;
		}
		break;
	case CUTS_DARKER:
		if (!channel->invert) {
			channel->lcut += span * BRIGHT_STEP;
			channel->hcut += span * BRIGHT_STEP;
		} else {
			channel->lcut -= span * BRIGHT_STEP;
			channel->hcut -= span * BRIGHT_STEP;
		}
		break;
	case CUTS_INVERT:
		channel->invert = !channel->invert;
		channel_set_lut_from_gamma(channel);
		d3_printf("invert is %d\n", channel->invert);
		break;
	case CUTS_AUTO:
		val = DEFAULT_SIGMAS;
		/* fallthrough */
	case CUTS_CONTRAST:
		channel->avg_at = DEFAULT_AVG_AT;
		if (!fr->stats.statsok) {
			frame_stats(fr);
		}
		channel->davg = fr->stats.cavg;
		channel->dsigma = fr->stats.csigma * 2;
		if (val < 0)
			val = 0;
		if (val >= SIGMAS_VALS)
			val = SIGMAS_VALS - 1;
		channel->lcut = channel->davg - channel->avg_at
			* sigmas[val] * channel->dsigma;
		channel->hcut = channel->davg + (1.0 - channel->avg_at)
			* sigmas[val] * channel->dsigma;
		break;
	default:
		err_printf("unknown cuts action %d, ignoring\n", action);
		return;
	}
	d3_printf("new cuts: lcut:%.0f hcut:%.0f\n", channel->lcut, channel->hcut);
	channel->channel_changed = 1;
}

void set_default_channel_cuts(struct image_channel* channel)
{
	channel_cuts_action(channel, CUTS_AUTO);
}


/* pan the image in the scrolled window
 * so that the center of the viewable area is at xc and yc
 * of the image's width/height
 */
int set_scrolls(GtkWidget *window, double xc, double yc)
{
	GtkScrolledWindow *scw;
	GtkAdjustment *hadj, *vadj;

	scw = g_object_get_data(G_OBJECT(window), "scrolled_window");
	if (scw == NULL) {
		err_printf("set_scroll: no scrolled window\n");
		return -1;
	}
	d3_printf("set scrolls at %.3f %.3f\n", xc, yc);
	hadj = gtk_scrolled_window_get_hadjustment(scw);
	vadj = gtk_scrolled_window_get_vadjustment(scw);
/* center the adjustments */
	hadj->value = (hadj->upper - hadj->lower) * xc + hadj->lower
		- hadj->page_size / 2;
	if (hadj->value < hadj->lower)
		hadj->value = hadj->lower;
	if (hadj->value > hadj->upper - hadj->page_size)
		hadj->value = hadj->upper - hadj->page_size;
	vadj->value = (vadj->upper - vadj->lower) * yc + vadj->lower
		- vadj->page_size / 2;
	if (vadj->value < vadj->lower)
		vadj->value = vadj->lower;
	if (vadj->value > vadj->upper - vadj->page_size)
		vadj->value = vadj->upper - vadj->page_size;
	gtk_adjustment_value_changed(hadj);
	gtk_adjustment_value_changed(vadj);
	return 0;
}

/* return the position of the scrollbars as
 * the fraction of the image's dimention the center of the
 * visible area is at
 */
int get_scrolls(GtkWidget *window, double *xc, double *yc)
{
	GtkScrolledWindow *scw;
	GtkAdjustment *hadj, *vadj;

	scw = g_object_get_data(G_OBJECT(window), "scrolled_window");
	if (scw == NULL) {
		err_printf("set_scroll: no scrolled window\n");
		return -1;
	}
	hadj = gtk_scrolled_window_get_hadjustment(scw);
	vadj = gtk_scrolled_window_get_vadjustment(scw);
/* center the adjustments */
	d3_printf("v %.3f p %.3f l %.3f u %.3f\n",
		  hadj->value, hadj->page_size, hadj->lower, hadj->upper);
	*xc = (hadj->value + hadj->page_size / 2) / (hadj->upper - hadj->lower);
	*yc = (vadj->value + vadj->page_size / 2) / (vadj->upper - vadj->lower);
	return 0;
}

void set_darea_size(GtkWidget *window, struct map_geometry *geom, double xc, double yc)
{
	int zi, zo;
	GtkWidget *darea;
	GtkScrolledWindow *scw;
	GtkAdjustment *hadj, *vadj;
	int w, h;

	darea = g_object_get_data(G_OBJECT(window), "image");

	if (geom->zoom > 1) {
		zo = 1;
		zi = floor(geom->zoom + 0.5);
	} else {
		zi = 1;
		zo = floor(1.0 / geom->zoom + 0.5);
	}

	scw = g_object_get_data(G_OBJECT(window), "scrolled_window");
	if (scw == NULL) {
		err_printf("set_darea_size: no scrolled window\n");
		return;
	}

	if (GTK_WIDGET(scw)->window == NULL)
		return;
	gdk_drawable_get_size(GTK_WIDGET(scw)->window, &w, &h);
	d3_printf("scw size id %d %d\n", w, h);

//	gdk_window_freeze_updates(GTK_WIDGET(darea)->window);

	gtk_widget_set_size_request(GTK_WIDGET(darea), geom->width * zi / zo,
				    geom->height * zi / zo);

/* we need this to make sure the drawing area contracts properly
 * when we go from scrollbars to no scrollbars, we have to be sure
 * everything happens in sequnece */

	if (w >= geom->width * zi / zo ||
	    h >= geom->height * zi / zo) {
		gtk_widget_queue_resize(window);
		while (gtk_events_pending ())
			gtk_main_iteration ();
	}

	hadj = gtk_scrolled_window_get_hadjustment(scw);
	vadj = gtk_scrolled_window_get_vadjustment(scw);

	if (hadj->page_size < geom->width * zi / zo) {
		hadj->upper = geom->width * zi / zo;
		if (hadj->upper < hadj->page_size)
			hadj->upper = hadj->page_size;
		hadj->value = (hadj->upper - hadj->lower) * xc + hadj->lower
			- hadj->page_size / 2;
		if (hadj->value < hadj->lower)
			hadj->value = hadj->lower;
		if (hadj->value > hadj->upper - hadj->page_size)
			hadj->value = hadj->upper - hadj->page_size;
	}

	if (vadj->page_size < geom->height * zi / zo) {
		vadj->upper = geom->height * zi / zo;
		if (vadj->upper < vadj->page_size)
			vadj->upper = vadj->page_size;
		vadj->value = (vadj->upper - vadj->lower) * yc + vadj->lower
			- vadj->page_size / 2;
		if (vadj->value < vadj->lower)
			vadj->value = vadj->lower;
		if (vadj->value > vadj->upper - vadj->page_size)
			vadj->value = vadj->upper - vadj->page_size;
	}
	gtk_adjustment_changed(vadj);
	gtk_adjustment_value_changed(vadj);
	gtk_adjustment_changed(hadj);
	gtk_adjustment_value_changed(hadj);
//	gtk_widget_queue_draw(window);
//	gdk_window_thaw_updates(darea->window);
//	d3_printf("at end of darea_size\n");
}

/*
 * step the zoom up/down
 */
void step_zoom(struct map_geometry *geom, int step)
{
	double zoom = geom->zoom;

	if (zoom < 1) {
		zoom = floor(1.0 / zoom + 0.5);
	} else {
		zoom = floor(zoom + 0.5);
	}

	if (zoom == 1.0) {
		if (step > 0)
			geom->zoom = 2.0;
		else
			geom->zoom = 0.5;
		return;
	}

	if (geom->zoom < 1)
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

	if (geom->zoom < 1) {
		geom->zoom = 1.0 / zoom;
	} else {
		geom->zoom = zoom;
	}
}

/*
 * pan window so that the pixel pointed by the cursor is centered
 */
void pan_cursor(GtkWidget *window)
{
	GtkWidget *image;
	int x, y, w, h;
	GdkModifierType mask;

	image = g_object_get_data(G_OBJECT(window), "image");

	if (image == NULL) {
		err_printf("no image\n");
		return;
	}
	if (image->window) {
		gdk_window_get_pointer(image->window, &x, &y, &mask);
		gdk_drawable_get_size(image->window, &w, &h);

		set_scrolls(window, 1.0 * x / w, 1.0 * y / h);
	}
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
	struct image_channel* channel;

	channel = g_object_get_data(G_OBJECT(window), "i_channel");
	if (channel == NULL) {
		err_printf("cuts_option_cb: no i_channel\n");
		return ;
	}
	if (channel->fr == NULL) {
		err_printf("cuts_option_cb: no frame in i_channel\n");
		return ;
	}
	channel_cuts_action(channel, action);
	show_zoom_cuts(window);
	gtk_widget_queue_draw(window);
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
	GtkWidget *image;
	int x, y, w, h;
	GdkModifierType mask;
	struct map_geometry *geom;
	double xc, yc;

	image = g_object_get_data(G_OBJECT(window), "image");
	geom = g_object_get_data(G_OBJECT(window), "geometry");

	if (image == NULL || geom == NULL) {
		err_printf("no image/geom\n");
		return;
	}

	gdk_window_get_pointer(image->window, &x, &y, &mask);
	gdk_drawable_get_size(image->window, &w, &h);

	d2_printf("view action %d at x:%d y:%d  (w:%d h:%d) mask: %d\n",
	       action, x, y, w, h, mask);

	switch(action) {
	case VIEW_ZOOM_IN:
		get_scrolls(window, &xc, &yc);
		step_zoom(geom, +1);
		set_darea_size(window, geom, 1.0 * x / w, 1.0 * y / h);
		gtk_widget_queue_draw(window);
		break;
	case VIEW_ZOOM_OUT:
		get_scrolls(window, &xc, &yc);
		step_zoom(geom, -1);
		set_darea_size(window, geom, xc, yc);
//		set_scrolls(window, xc, yc);
		gtk_widget_queue_draw(window);
		break;
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
}

/*
 * display stats in region around cursor
 * does not use action or menu_item
 */
void show_region_stats(GtkWidget *window, double x, double y)
{
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

	if (i_channel->fr->pix_format == PIX_FLOAT) {
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
	} else if (i_channel->fr->pix_format == PIX_BYTE) {
		/* FIXME: This won't work with RGB */
		val = 1.0 * *((unsigned char *)(i_channel->fr->dat) + xi + yi * i_channel->fr->w);
		sprintf(buf, "Pixel [%d,%d]=%.1f",
		xi, yi, val);
	} else {
		sprintf(buf, "Pixel [%d,%d] unknown format", xi, yi);
	}
	info_printf_sb2(window, "%s\n", buf);
}

/*
 * show zoom/cuts in statusbar
 */
void show_zoom_cuts(GtkWidget * window)
{
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
}

/* set a channel's lut from cuts, gamma and toe */
void channel_set_direct_lut(struct image_channel *channel)
{
}

#define T_START_GAMMA 0.2

/* set a channel's lut from the gamma and toe */
void channel_set_lut_from_gamma(struct image_channel *channel)
{
	double x, y;
	int i;
	if (channel->lut_mode == LUT_MODE_DIRECT) {
		channel_set_direct_lut(channel);
		return;
	}
	d3_printf("set lut from gamma\n");
	for (i=0; i<LUT_SIZE; i++) {
		double x0, x1, g;
		x = 1.0 * (i) / LUT_SIZE;
		x0 = channel->toe;
		x1 = channel->gamma * channel->toe;
//		d3_printf("channel offset is %f\n", channel->offset);
		if (x0 < 0.001) {
			y = 65535 * (channel->offset + (1 - channel->offset) *
				     pow(x, 1.0 / channel->gamma));
		} else {
			g = (x + x1) * x0 / ((x + x0) * x1);
			y = 65535 * (channel->offset + (1 - channel->offset) * pow(x, g));
		}
		if (!channel->invert)
			channel->lut[i] = y;
		else
			channel->lut[LUT_SIZE - i - 1] = y;
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
	int firstx, lcx, hcx;
	int i, h;
	GdkGC *fgc, *redgc;
	GdkColor red;
	GdkColormap *cmap;

	cmap = gdk_colormap_get_system();
	red.red = 65535;
	red.green = 0;
	red.blue = 0;
	gdk_colormap_alloc_color(cmap, &red, FALSE, TRUE);

	redgc = gdk_gc_new(darea->window);
	gdk_gc_set_foreground(redgc, &red);

	firstx = area->x / dskip;
	lcx = darea->allocation.width * (1 - CUTS_FACTOR) / 2;
	hcx = darea->allocation.width - darea->allocation.width * (1 - CUTS_FACTOR) / 2;

	d3_printf("aw=%d lc=%d hc=%d\n", darea->allocation.width, lcx, hcx);

/* clear the area */
	gdk_draw_rectangle(darea->window, darea->style->white_gc,
			   1, area->x, 0, area->width, darea->allocation.height);
	fgc = darea->style->black_gc;
//	gdk_gc_set_line_attributes(fgc, dskip, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
	for (i = firstx; i < (area->x + area->width) / dskip + 1 && i < dbins; i++) {
		h = darea->allocation.height - rbh[i] * darea->allocation.height;
//		d3_printf("hist line v=%.3f x=%d  h=%d\n", rbh[i], i, h);
		gdk_draw_line(darea->window, fgc, i*dskip, darea->allocation.height, i*dskip, h);
	}
	gdk_draw_line(darea->window, redgc, lcx, darea->allocation.height, lcx, 0);
	gdk_draw_line(darea->window, redgc, hcx, darea->allocation.height, hcx, 0);

	g_object_unref (redgc);
}

/* draw the curve over the histogram area */
void plot_curve(GtkWidget *darea, GdkRectangle *area, struct image_channel *channel)
{
	int lcx, hcx, span;
	int i, ci;
	GdkGC *greengc;
	GdkColor green;
	GdkColormap *cmap;
	GdkPoint *points;

	cmap = gdk_colormap_get_system();
	green.red = 10000;
	green.green = 40000;
	green.blue = 10000;
	gdk_colormap_alloc_color(cmap, &green, FALSE, TRUE);

	greengc = gdk_gc_new(darea->window);
	gdk_gc_set_foreground(greengc, &green);
	gdk_gc_set_line_attributes(greengc, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

	lcx = darea->allocation.width * (1 - CUTS_FACTOR) / 2;
	hcx = darea->allocation.width - darea->allocation.width * (1 - CUTS_FACTOR) / 2;
	span = hcx - lcx;
	points = calloc((span+1), sizeof(GdkPoint));
	points[0].x = 0;
	points[0].y = darea->allocation.height
			- darea->allocation.height * channel->lut[0] / 65536;
	for (i = 0; i < span; i++) {
		ci = LUT_SIZE * i / span;
		points[i+1].x = lcx+i;
		points[i+1].y = darea->allocation.height
			- darea->allocation.height * channel->lut[ci] / 65536;
	}
	gdk_draw_lines(darea->window, greengc, points, span+1);
	free(points);

	g_object_unref(greengc);
}


/* draw a piece of a histogram */
void draw_histogram(GtkWidget *darea, GdkRectangle *area, struct image_channel *channel,
		    double low, double high, int logh)
{
	struct im_histogram *hist;
	double *rbh, max = 0.0;
	double hbinsize, dbinsize;
	int dskip, dbins;

	if (!channel->fr->stats.statsok) {
		err_printf("draw_histogram: no stats\n");
		return;
	}
	hist = &(channel->fr->stats.hist);
	hbinsize = (hist->end - hist->st) / hist->hsize;
	dbinsize = (high - low) / darea->allocation.width;
	if (dbinsize < hbinsize) {
		dskip = 1 + floor(hbinsize / dbinsize);
	} else {
		dskip = 1;
	}
	dbins = darea->allocation.width / dskip + 1;
	rbh = calloc(dbins, sizeof(double));
	if (rbh == NULL)
		return;
	rebin_histogram(hist, rbh, dbins, low, high, &max);
	scale_histogram(rbh, dbins, max, logh);
	plot_histogram(darea, area, dskip, rbh, dbins);
	plot_curve(darea, area, channel);
}


gboolean histogram_expose_cb(GtkWidget *darea, GdkEventExpose *event, gpointer dialog)
{
	struct image_channel *channel;
	GtkWidget *logckb;
	double low, high;
	int logh = 0;

	channel = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (channel == NULL) /* no channel */
		return 0 ;

	logckb = g_object_get_data(G_OBJECT(dialog), "log_hist_check");
	if (logckb != NULL) {
		logh = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logckb));
	}

	low = channel->lcut - (channel->hcut - channel->lcut) * (1 / CUTS_FACTOR - 1) / 2;
	high = channel->hcut + (channel->hcut - channel->lcut) * (1 / CUTS_FACTOR - 1) / 2;;

	draw_histogram(darea, &event->area, channel, low, high, logh);

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
	struct image_channel *channel;
	GtkWidget *window;

	channel = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (channel == NULL) /* no channel */
		return;
	window = g_object_get_data(G_OBJECT(dialog), "image_window");

	channel->lcut = spin_get_value(dialog, "low_cut_spin");
	channel->hcut = spin_get_value(dialog, "high_cut_spin");

	if (channel->hcut <= channel->lcut) {
		channel->hcut = channel->lcut + 1;
	}

	channel->channel_changed = 1;
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
	struct image_channel *channel;
	GtkWidget *window;

	channel = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (channel == NULL) /* no channel */
		return;
	window = g_object_get_data(G_OBJECT(dialog), "image_window");

	channel->gamma = spin_get_value(dialog, "gamma_spin");
	channel->toe = spin_get_value(dialog, "toe_spin");
	channel->offset = spin_get_value(dialog, "offset_spin");

	channel_set_lut_from_gamma(channel);

	channel->channel_changed = 1;
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
	struct image_channel *i_channel;
	double lcut, hcut, gamma, toe, offset;

	i_channel = g_object_get_data(G_OBJECT(dialog), "i_channel");
	if (i_channel == NULL) /* no channel */
		return;
	lcut = i_channel->lcut;
	hcut = i_channel->hcut;
	gamma = i_channel->gamma;
	toe = i_channel->toe;
	offset = i_channel->offset;
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
	struct image_channel *i_channel;

	ret = g_object_get_data(G_OBJECT(window), "i_channel");
	if (ret == NULL) /* no channel */
		return;
	i_channel = ret;

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
		gdk_window_raise(dialog->window);
	}
	ref_image_channel(i_channel);
	g_object_set_data_full(G_OBJECT(dialog), "i_channel", i_channel,
				 (GDestroyNotify)release_image_channel);
	imadj_dialog_update(dialog);
	imadj_dialog_edit(dialog);
}


/* from Glade */

GtkWidget* create_imadj_dialog (void)
{
  GtkWidget *imadj_dialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox1;
  GtkWidget *hist_scrolled_win;
  GtkWidget *viewport1;
  GtkWidget *hist_area;
  GtkWidget *vbox2;
  GtkWidget *frame3;
  GtkWidget *hbox2;
  GtkWidget *channel_combo;
  GtkWidget *label3;
  GtkObject *gamma_spin_adj;
  GtkWidget *gamma_spin;
  GtkWidget *label4;
  GtkObject *toe_spin_adj;
  GtkWidget *toe_spin;
  GtkWidget *label5;
  GtkWidget *log_hist_check;
  GtkWidget *frame2;
  GtkWidget *table1;
  GtkWidget *hseparator1;
  GtkObject *low_cut_spin_adj;
  GtkWidget *low_cut_spin;
  GtkObject *high_cut_spin_adj;
  GtkWidget *high_cut_spin;
  GtkObject *offset_spin_adj;
  GtkWidget *offset_spin;
  GtkWidget *label2;
  GtkWidget *label1;
  GtkWidget *table2;
  GtkWidget *cuts_darker;
  GtkWidget *cuts_sharper;
  GtkWidget *cuts_duller;
  GtkWidget *cuts_auto;
  GtkWidget *cuts_min_max;
  GtkWidget *cuts_brighter;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbuttonbox1;
  GtkWidget *hist_close;
  GtkWidget *hist_apply;
  GtkWidget *hist_redraw;

  imadj_dialog = gtk_dialog_new ();
  g_object_set_data (G_OBJECT (imadj_dialog), "imadj_dialog", imadj_dialog);
  gtk_window_set_title (GTK_WINDOW (imadj_dialog), ("Curves / Histogram"));
//  GTK_WINDOW (imadj_dialog)->type = GTK_WINDOW_DIALOG;

  dialog_vbox1 = GTK_DIALOG (imadj_dialog)->vbox;
  g_object_set_data (G_OBJECT (imadj_dialog), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  vbox1 = gtk_vbox_new (FALSE, 0);
  g_object_ref (vbox1);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "vbox1", vbox1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

  hist_scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  g_object_ref (hist_scrolled_win);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_scrolled_win", hist_scrolled_win,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hist_scrolled_win);
  gtk_box_pack_start (GTK_BOX (vbox1), hist_scrolled_win, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hist_scrolled_win), 2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (hist_scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_range_set_update_policy (GTK_RANGE (GTK_SCROLLED_WINDOW (hist_scrolled_win)->hscrollbar), GTK_POLICY_NEVER);

  viewport1 = gtk_viewport_new (NULL, NULL);
  g_object_ref (viewport1);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "viewport1", viewport1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (viewport1);
  gtk_container_add (GTK_CONTAINER (hist_scrolled_win), viewport1);

  hist_area = gtk_drawing_area_new ();
  g_object_ref (hist_area);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_area", hist_area,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hist_area);
  gtk_container_add (GTK_CONTAINER (viewport1), hist_area);

  vbox2 = gtk_vbox_new (FALSE, 0);
  g_object_ref (vbox2);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "vbox2", vbox2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, TRUE, 0);

  frame3 = gtk_frame_new (("Curve/Histogram"));
  g_object_ref (frame3);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "frame3", frame3,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox2), frame3, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  g_object_ref (hbox2);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hbox2", hbox2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hbox2);
  gtk_container_add (GTK_CONTAINER (frame3), hbox2);

  channel_combo = gtk_combo_box_new_text ();
  g_object_ref (channel_combo);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "chanel_combo", channel_combo,
                            (GDestroyNotify) g_object_unref);
  gtk_combo_box_append_text (GTK_COMBO_BOX(channel_combo), "Channel");
  gtk_combo_box_set_active (GTK_COMBO_BOX(channel_combo), 0);
  gtk_widget_show (channel_combo);
  gtk_box_pack_start (GTK_BOX (hbox2), channel_combo, FALSE, FALSE, 0);

  label3 = gtk_label_new (("Gamma"));
  g_object_ref (label3);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "label3", label3,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label3);
  gtk_box_pack_start (GTK_BOX (hbox2), label3, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label3), 5, 0);

  gamma_spin_adj = gtk_adjustment_new (1, 0.1, 10, 0.1, 1, 0.0);
  gamma_spin = gtk_spin_button_new (GTK_ADJUSTMENT (gamma_spin_adj), 1, 1);
  g_object_ref (gamma_spin);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "gamma_spin", gamma_spin,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_set_size_request (GTK_WIDGET(&(GTK_SPIN_BUTTON(gamma_spin)->entry)), 60, -1);
  gtk_widget_show (gamma_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), gamma_spin, FALSE, TRUE, 0);

  label4 = gtk_label_new (("Toe"));
  g_object_ref (label4);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "label4", label4,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (hbox2), label4, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label4), 6, 0);

  toe_spin_adj = gtk_adjustment_new (0, 0, 0.4, 0.002, 0.1, 0);
  toe_spin = gtk_spin_button_new (GTK_ADJUSTMENT (toe_spin_adj), 0.002, 3);
  g_object_ref (toe_spin);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "toe_spin", toe_spin,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_set_size_request(GTK_WIDGET(&(GTK_SPIN_BUTTON(toe_spin)->entry)), 60, -1);
  gtk_widget_show (toe_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), toe_spin, FALSE, FALSE, 0);

  label5 = gtk_label_new (("Offset"));
  g_object_ref (label5);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "label5", label5,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label5);
  gtk_box_pack_start (GTK_BOX (hbox2), label5, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label5), 5, 0);

  offset_spin_adj = gtk_adjustment_new (0, 0, 1, 0.01, 1, 0.0);
  offset_spin = gtk_spin_button_new (GTK_ADJUSTMENT (offset_spin_adj), 0.01, 2);
  g_object_ref (offset_spin);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "offset_spin", offset_spin,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_set_size_request(GTK_WIDGET(&(GTK_SPIN_BUTTON(offset_spin)->entry)), 60, -1);
  gtk_widget_show (offset_spin);
  gtk_box_pack_start (GTK_BOX (hbox2), offset_spin, FALSE, TRUE, 0);

  log_hist_check = gtk_check_button_new_with_label (("Log"));
  g_object_ref (log_hist_check);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "log_hist_check", log_hist_check,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (log_hist_check);
  gtk_box_pack_start (GTK_BOX (hbox2), log_hist_check, FALSE, FALSE, 0);

  frame2 = gtk_frame_new (("Cuts"));
  g_object_ref (frame2);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "frame2", frame2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox2), frame2, TRUE, TRUE, 0);

  table1 = gtk_table_new (3, 3, FALSE);
  g_object_ref (table1);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "table1", table1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame2), table1);

  hseparator1 = gtk_hseparator_new ();
  g_object_ref (hseparator1);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hseparator1", hseparator1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 3, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  low_cut_spin_adj = gtk_adjustment_new (1, -65535, 65535, 2, 10, 0);
  low_cut_spin = gtk_spin_button_new (GTK_ADJUSTMENT (low_cut_spin_adj), 2, 0);
  g_object_ref (low_cut_spin);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "low_cut_spin", low_cut_spin,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (low_cut_spin);
  gtk_table_attach (GTK_TABLE (table1), low_cut_spin, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  high_cut_spin_adj = gtk_adjustment_new (1, -65535, 65535, 10, 10, 0);
  high_cut_spin = gtk_spin_button_new (GTK_ADJUSTMENT (high_cut_spin_adj), 10, 0);
  g_object_ref (high_cut_spin);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "high_cut_spin", high_cut_spin,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (high_cut_spin);
  gtk_table_attach (GTK_TABLE (table1), high_cut_spin, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label2 = gtk_label_new (("High"));
  g_object_ref (label2);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "label2", label2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label2), 6, 0);

  label1 = gtk_label_new (("Low"));
  g_object_ref (label1);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "label1", label1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label1), 6, 0);

  table2 = gtk_table_new (2, 3, TRUE);
  g_object_ref (table2);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "table2", table2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (table2);
  gtk_table_attach (GTK_TABLE (table1), table2, 2, 3, 1, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 3);

  cuts_darker = gtk_button_new_with_label (("Darker"));
  g_object_ref (cuts_darker);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_darker", cuts_darker,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (cuts_darker);
  gtk_table_attach (GTK_TABLE (table2), cuts_darker, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cuts_sharper = gtk_button_new_with_label (("Sharper"));
  g_object_ref (cuts_sharper);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_sharper", cuts_sharper,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (cuts_sharper);
  gtk_table_attach (GTK_TABLE (table2), cuts_sharper, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cuts_duller = gtk_button_new_with_label (("Flatter"));
  g_object_ref (cuts_duller);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_duller", cuts_duller,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (cuts_duller);
  gtk_table_attach (GTK_TABLE (table2), cuts_duller, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cuts_auto = gtk_button_new_with_label (("Auto Cuts"));
  g_object_ref (cuts_auto);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_auto", cuts_auto,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (cuts_auto);
  gtk_table_attach (GTK_TABLE (table2), cuts_auto, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cuts_min_max = gtk_button_new_with_label (("Min-Max"));
  g_object_ref (cuts_min_max);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_min_max", cuts_min_max,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (cuts_min_max);
  gtk_table_attach (GTK_TABLE (table2), cuts_min_max, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cuts_brighter = gtk_button_new_with_label (("Brighter"));
  g_object_ref (cuts_brighter);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_brighter", cuts_brighter,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (cuts_brighter);
  gtk_table_attach (GTK_TABLE (table2), cuts_brighter, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);

  dialog_action_area1 = GTK_DIALOG (imadj_dialog)->action_area;
  g_object_set_data (G_OBJECT (imadj_dialog), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 3);

  hbuttonbox1 = gtk_hbutton_box_new ();
  g_object_ref (hbuttonbox1);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hbuttonbox1", hbuttonbox1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbuttonbox1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbuttonbox1), 3);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_EDGE);
  gtk_box_set_spacing (GTK_BOX (hbuttonbox1), 4);

  //gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);
  //gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 15, -1);

  hist_close = gtk_button_new_with_label (("Close"));
  g_object_ref (hist_close);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_close", hist_close,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hist_close);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), hist_close);
  GTK_WIDGET_SET_FLAGS (hist_close, GTK_CAN_DEFAULT);

  hist_apply = gtk_button_new_with_label (("Apply"));
  g_object_ref (hist_apply);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_apply", hist_apply,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hist_apply);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), hist_apply);
  GTK_WIDGET_SET_FLAGS (hist_apply, GTK_CAN_DEFAULT);

  hist_redraw = gtk_button_new_with_label (("Redraw"));
  g_object_ref (hist_redraw);
  g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_redraw", hist_redraw,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hist_redraw);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), hist_redraw);
  GTK_WIDGET_SET_FLAGS (hist_redraw, GTK_CAN_DEFAULT);

  return imadj_dialog;
}

