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

/* functions that manage 'gui_stars', star and object markers
 * that are overlaid on the images
 *
 * all the sources are linked in the list attached to the window as
 * "gui_star_list"
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "sourcesdraw.h"
#include "params.h"
#include "wcs.h"
#include "psf.h"
#include "filegui.h"

static unsigned int colors[PAR_COLOR_ALL] = {
	0xff0000,
	0xff8000,
	0xffff00,
	0x00c000,
	0x00ffff,
	0x0000ff,
	0x8080ff,
	0x808080,
	0xffffff,
};

void get_color_par(int p, GdkColor *color, GdkColormap *cmap)
{
	unsigned int crgb;
	crgb = colors[P_INT(p)];
	color->red = (crgb >> 8) & 0xff00;
	color->green = (crgb) & 0xff00;
	color->blue = (crgb << 8) & 0xff00;
	if (!gdk_colormap_alloc_color(cmap, color, FALSE, TRUE)) {
		g_error("couldn't allocate color");
	}
}

void gui_star_list_update_colors(struct gui_star_list *gsl)
{
	GdkColormap *cmap;
	GdkColor color;
	cmap = gdk_colormap_get_system();

	get_color_par(SDISP_SELECTED_COLOR, &color, cmap);
	memcpy(&(gsl->selected_color), &color, sizeof(GdkColor));
	get_color_par(SDISP_SIMPLE_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_SIMPLE]), &color, sizeof(GdkColor));
	get_color_par(SDISP_SREF_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_SREF]), &color, sizeof(GdkColor));
	get_color_par(SDISP_APSTD_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_APSTD]), &color, sizeof(GdkColor));
	get_color_par(SDISP_APSTAR_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_APSTAR]), &color, sizeof(GdkColor));
	get_color_par(SDISP_CAT_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_CAT]), &color, sizeof(GdkColor));
	get_color_par(SDISP_USEL_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_USEL]), &color, sizeof(GdkColor));
	get_color_par(SDISP_ALIGN_COLOR, &color, cmap);
	memcpy(&(gsl->color[STAR_TYPE_ALIGN]), &color, sizeof(GdkColor));
	gsl->shape[STAR_TYPE_SIMPLE] = P_INT(SDISP_SIMPLE_SHAPE);
	gsl->shape[STAR_TYPE_SREF] = P_INT(SDISP_SREF_SHAPE);
	gsl->shape[STAR_TYPE_APSTD] = P_INT(SDISP_APSTD_SHAPE);
	gsl->shape[STAR_TYPE_APSTAR] = P_INT(SDISP_APSTAR_SHAPE);
	gsl->shape[STAR_TYPE_CAT] = P_INT(SDISP_CAT_SHAPE);
	gsl->shape[STAR_TYPE_USEL] = P_INT(SDISP_USEL_SHAPE);
	gsl->shape[STAR_TYPE_ALIGN] = P_INT(SDISP_ALIGN_SHAPE);
}

void attach_star_list(struct gui_star_list *gsl, GtkWidget *window)
{
	g_object_set_data_full(G_OBJECT(window), "gui_star_list",
				 gsl, (GDestroyNotify)gui_star_list_release);
}

/* we draw stars that are this much outside the requested
 * area so we are sure we don't have bits left out */
static int star_near_area(int x, int y, GdkRectangle *area, int margin)
{
	if (x < area->x - margin)
		return 0;
	if (y < area->y - margin)
		return 0;
	if (x > area->x + area->width + margin)
		return 0;
	if (y > area->y + area->height + margin)
		return 0;
	return 1;
}

/* draw the star shape on screen */
static void draw_a_star(GdkWindow *drawable, GdkGC *gc, int x, int y, int size, int shape,
			char *label, int ox, int oy, GdkFont *font)
{
	GdkPoint point[5];
	GdkSegment seg[5];
	int r;

	switch(shape) {
	case STAR_SHAPE_CIRCLE:
		gdk_draw_arc(drawable, gc, FALSE, x-size, y-size,
			     2*size, 2*size, 0, 360*64);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+size+ox+2, y+oy+2,
				      label, strlen(label));
		}
		break;
	case STAR_SHAPE_SQUARE:
		gdk_draw_rectangle(drawable, gc, FALSE, x-size, y-size,
				   2*size, 2*size);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+size+ox+2, y+oy+2,
				      label, strlen(label));
		}
		break;
	case STAR_SHAPE_DIAMOND:
		size *= 2;
		point[0].x = x;
		point[0].y = y + size;
		point[1].x = x - size;
		point[1].y = y;
		point[2].x = x;
		point[2].y = y - size;
		point[3].x = x + size;
		point[3].y = y;
		point[4].x = x;
		point[4].y = y + size;
		gdk_draw_lines(drawable, gc, point, 5);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+size+ox+2, y+oy+2,
				      label, strlen(label));
		}
		break;
	case STAR_SHAPE_BLOB:
//		if (size > 1)
//			size --;
		gdk_draw_arc(drawable, gc, TRUE, x-size, y-size,
			     2*size, 2*size, 0, 360*64);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+size+ox+2, y+oy+2,
				      label, strlen(label));
		}
		break;
	case STAR_SHAPE_APHOT:
/* size is the zoom here. for x1, size = 2
   (so we never make the rings too small */
		r = P_DBL(AP_R1) * size / 2;
		gdk_draw_arc(drawable, gc, FALSE, x-r, y-r,
			     2*r, 2*r, 0, 360*64);
		r = P_DBL(AP_R2) * size / 2;
		gdk_draw_arc(drawable, gc, FALSE, x-r, y-r,
			     2*r, 2*r, 0, 360*64);
		r = P_DBL(AP_R3) * size / 2;
		gdk_draw_arc(drawable, gc, FALSE, x-r, y-r,
			     2*r, 2*r, 0, 360*64);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+r+ox+2, y+oy+2,
				      label, strlen(label));
		}
		break;
	case STAR_SHAPE_CROSS:
		seg[0].x1 = x+size;
		seg[0].x2 = x+5*size;
		seg[0].y1 = y;
		seg[0].y2 = y;
		seg[1].x1 = x-size;
		seg[1].x2 = x-5*size;
		seg[1].y1 = y;
		seg[1].y2 = y;
		seg[2].x1 = x;
		seg[2].x2 = x;
		seg[2].y1 = y-size;
		seg[2].y2 = y-5*size;
		seg[3].x1 = x;
		seg[3].x2 = x;
		seg[3].y1 = y+size;
		seg[3].y2 = y+5*size;
		gdk_draw_segments(drawable, gc, seg, 4);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+size+ox+2, y+oy-2,
				      label, strlen(label));
		}
		break;
	default:
		gdk_draw_arc(drawable, gc, FALSE, x-size, y-size,
			     2*size, 2*size, 0, 360*64);
		if (label != NULL && font != NULL) {
			gdk_draw_text(drawable, font, gc, x+size+ox+2, y+oy+2,
				      label, strlen(label));
		}
	}
/* TODO add label drawing here */
}

/* get the proper drawing type for the star */
/*
static int draw_type (struct gui_star *gs)
{
	if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK(STAR_TYPE_CAT)) && (gs->s != NULL)) {
		if (CAT_STAR(gs->s)->flags & CAT_VARIABLE)
			return STAR_TYPE_APSTAR;
		if (CAT_STAR(gs->s)->flags & CAT_PHOTOMET)
			return STAR_TYPE_APSTD;
		if (CAT_STAR(gs->s)->flags & CAT_ASTROMET)
			return STAR_TYPE_SREF;
		return STAR_TYPE_CAT;
	}
	return gs->flags & STAR_TYPE_M;
}
*/

static void draw_star_helper(struct gui_star *gs, GdkWindow *drawable, struct gui_star_list *gsl,
			GdkGC *gc, double zoom, GdkFont *font)
{
	int type;
	int ix, iy, isz;

	type = gs->flags & STAR_TYPE_M;

	if (type > STAR_TYPES - 1)
		type = STAR_TYPES - 1;
	if (gs->flags & STAR_SELECTED)
		gdk_gc_set_foreground(gc, &(gsl->selected_color));
	else
		gdk_gc_set_foreground(gc, &(gsl->color[type]));
	ix = (gs->x + 0.5) * zoom;
	iy = (gs->y + 0.5) * zoom;
	if (gsl->shape[type] == STAR_SHAPE_APHOT) {
		if (zoom < 1)
			isz = 1;
		else
			isz = 2 * zoom;
	} else if (gsl->shape[type] == STAR_SHAPE_BLOB) {
		isz = gs->size;
	} else {
		if (P_INT(DO_ZOOM_SYMBOLS)) {
			if (zoom <= P_INT(DO_ZOOM_LIMIT))
				isz = gs->size * zoom;
			else
				isz = gs->size * P_INT(DO_ZOOM_LIMIT);
		} else {
			isz = gs->size;
		}
	}
	clamp_int(&isz, P_INT(DO_MIN_STAR_SZ), 100);
	if (gs->flags & STAR_HIDDEN)
		return;
	draw_a_star(drawable, gc, ix, iy, isz, gsl->shape[type],
		    gs->label.label, gs->label.ox, gs->label.oy, font);
	if ((gs->pair != NULL) && (gs->pair->flags & STAR_HAS_PAIR)) {
		int pix, piy;
		gint8 dash_list[]={1, 1};
		GdkGC *gc_pline;
		gc_pline = gdk_gc_new(drawable);
		gdk_gc_copy(gc_pline, gc);
		gdk_gc_set_line_attributes (gc_pline, 0, GDK_LINE_ON_OFF_DASH,
					    GDK_CAP_BUTT, GDK_JOIN_BEVEL);
		gdk_gc_set_dashes(gc_pline, 0, dash_list, 2);
		pix = (gs->pair->x + 0.5) * zoom;
		piy = (gs->pair->y + 0.5) * zoom;
		gdk_draw_line(drawable, gc_pline, ix, iy, pix, piy);
		g_object_unref (gc_pline);
	}
	if (P_INT(DO_SHOW_DELTAS) &&
	    (gs->s != NULL) && (CAT_STAR(gs->s)->flags & INFO_POS)) {
		int pix, piy;
		pix = (P_DBL(WCS_PLOT_ERR_SCALE) *
		       CAT_STAR(gs->s)->pos[POS_DX] + 0.5) * zoom;
		piy = (P_DBL(WCS_PLOT_ERR_SCALE) *
		       CAT_STAR(gs->s)->pos[POS_DY] + 0.5) * zoom;
		if ((pix < -4) || (pix > 4) || (piy < -4) || (piy > 4)) {
			gdk_draw_line(drawable, gc, ix, iy, ix+pix, iy+piy);
			gdk_draw_arc(drawable, gc, TRUE, ix+pix-2, iy+piy-2,
				     4, 4, 0, 360*64);
		}
	}

}

/* compute display size for a cat_star */
double cat_star_size(struct cat_star *cats)
{
	double size;
	if (cats == NULL)
		return 1.0 * P_INT(DO_DEFAULT_STAR_SZ);
	if (cats->mag == 0.0)
		return 1.0 * P_INT(DO_DEFAULT_STAR_SZ);

	size = 1.0 * P_INT(DO_MIN_STAR_SZ) +
		P_DBL(DO_PIXELS_PER_MAG) * (P_DBL(DO_DLIM_MAG) - cats->mag);
	clamp_double(&(size), 1.0 * P_INT(DO_MIN_STAR_SZ),
		     1.0 * P_INT(DO_MAX_STAR_SZ));
	return size;
}


/* draw a single star; this is not very efficient, use
 * draw_star_helper for expose redraws */
void draw_gui_star(struct gui_star *gs, GtkWidget *window)
{
	GdkGC *gc;
	GtkWidget *darea;
	struct map_geometry *geom;
	struct gui_star_list *gsl;

	darea = g_object_get_data(G_OBJECT(window), "image");
	if (darea == NULL)
		return;
	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return;
	gc = gdk_gc_new(darea->window);
	draw_star_helper(gs, darea->window, gsl, gc, geom->zoom, gtk_style_get_font(darea->style));
	g_object_unref(gc);
}

/* draw a GSList of gui_stars
 * mostly used for selection changes
 */
void draw_star_list(GSList *stars, GtkWidget *window)
{
	struct gui_star *gs;
	GSList *sl = NULL;
	GdkGC *gc;
	GtkWidget *darea;
	struct map_geometry *geom;
	struct gui_star_list *gsl;

	darea = g_object_get_data(G_OBJECT(window), "image");
	if (darea == NULL)
		return;
	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return;
	gc = gdk_gc_new(darea->window);
	sl = stars;
	while(sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		draw_star_helper(gs, darea->window, gsl, gc, geom->zoom, gtk_style_get_font(darea->style));
	}
	g_object_unref(gc);
}


/* hook function for sources drawing on expose */
void draw_sources_hook(GtkWidget *darea, GtkWidget *window, GdkRectangle *area)
{
	GdkGC *gc;
	GSList *sl = NULL;
	struct map_geometry *geom;
	struct gui_star_list *gsl;
	struct gui_star *gs;
	int ix, iy, isz;

	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return;
	gui_star_list_update_colors(gsl);
	gc = gdk_gc_new(darea->window);

/*  	d3_printf("expose area is %d by %d starting at %d, %d\n", */
/*  		  area->width, area->height, area->x, area->y); */
	sl = gsl->sl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (!(TYPE_MASK_GSTAR(gs) & gsl->display_mask))
			continue;
		ix = (gs->x + 0.5) * geom->zoom;
		iy = (gs->y + 0.5) * geom->zoom;
		isz = gs->size * geom->zoom + gsl->max_size;
		if (star_near_area(ix, iy, area, isz)) {
			draw_star_helper(gs, darea->window, gsl, gc,
					 geom->zoom, gtk_style_get_font(darea->style));
		}
		if (gs->pair != NULL) {
			ix = (gs->pair->x + 0.5) * geom->zoom;
			iy = (gs->pair->y + 0.5) * geom->zoom;
			isz = gs->pair->size * geom->zoom + gsl->max_size;
			if (star_near_area(ix, iy, area, isz)) {
				draw_star_helper(gs, darea->window, gsl,
						 gc, geom->zoom, gtk_style_get_font(darea->style));
			}
		}
	}
	g_object_unref(gc);
}


/*
 * compute a display size for a star given it's flux and
 * a reference flux and fwhm
 */
static double star_size_flux(double flux, double ref_flux, double fwhm)
{
	double size;
//	size = fwhm * P_DBL(DO_STAR_SZ_FWHM_FACTOR);
//	clamp_double(&size, 0, 1.0 * P_INT(DO_MAX_STAR_SZ));
	size = 1.0 * P_INT(DO_MAX_STAR_SZ) / 1.5 + 2.5 * P_DBL(DO_PIXELS_PER_MAG)
		* log10(flux / ref_flux);
	clamp_double(&size, 1.0 * P_INT(DO_MIN_STAR_SZ) + 0.01,
		     1.0 * P_INT(DO_MAX_STAR_SZ));
	return size;
}



/* find stars in frame and add them to the current star list
 * if the list exists, remove all the stars of type SIMPLE
 * from it first
 */
void find_stars_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	struct sources *src;
	struct gui_star_list *gsl;
	struct gui_star *gs;
	struct cat_star **csl;
	int i,n;
	struct catalog *cat;
	struct image_channel *i_ch;
	double ref_flux = 0.0, ref_fwhm = 0.0;
	double radius;
	struct wcs *wcs;

	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");

//	d3_printf("find_stars_cb action %d\n", action);

	switch(action) {
	case ADD_STARS_GSC:
		if (i_ch == NULL || i_ch->fr == NULL)
			return;
		wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
		if (wcs == NULL || wcs->wcsset == WCS_INVALID) {
			err_printf_sb2(window, "Need an Initial WCS to Load GSC Stars");
			error_beep();
			return;
		}
		gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
		if (gsl == NULL) {
			gsl = gui_star_list_new();
			attach_star_list(gsl, window);
		}
		radius = 60.0*fabs(i_ch->fr->w * wcs->xinc);
		clamp_double(&radius, 1.0, P_DBL(SD_GSC_MAX_RADIUS));
		add_stars_from_gsc(gsl, wcs, radius,
				   P_DBL(SD_GSC_MAX_MAG),
				   P_INT(SD_GSC_MAX_STARS));
		gsl->display_mask |= TYPE_MASK_CATREF;
		gsl->select_mask |= TYPE_MASK_CATREF;
		break;
	case ADD_STARS_TYCHO2:
		if (i_ch == NULL || i_ch->fr == NULL)
			return;
		wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
		if (wcs == NULL || wcs->wcsset == WCS_INVALID) {
			err_printf_sb2(window, "Need an initial WCS to load Tycho2 stars");
			error_beep();
			return;
		}
		gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
		if (gsl == NULL) {
			gsl = gui_star_list_new();
			attach_star_list(gsl, window);
		}
		radius = 60.0*fabs(i_ch->fr->w * wcs->xinc);
		clamp_double(&radius, 1.0, P_DBL(SD_GSC_MAX_RADIUS));

		cat = open_catalog("tycho2");

		if (cat == NULL || cat->cat_search == NULL)
			return;

		d3_printf("tycho2 opened\n");

		csl = calloc(P_INT(SD_GSC_MAX_STARS),
			sizeof(struct cat_star *));
		if (csl == NULL)
			return;

		n = (* cat->cat_search)(csl, cat, wcs->xref, wcs->yref, radius,
					P_INT(SD_GSC_MAX_STARS));

		d3_printf ("got %d from cat_search\n", n);

		merge_cat_stars(csl, n, gsl, wcs);

		free(csl);

		gsl->display_mask |= TYPE_MASK_CATREF;
		gsl->select_mask |= TYPE_MASK_CATREF;
		break;
	case ADD_STARS_DETECT:
		if (i_ch == NULL || i_ch->fr == NULL)
			return;
		gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
		if (gsl == NULL) {
			gsl = gui_star_list_new();
			attach_star_list(gsl, window);
		}
		src = new_sources(P_INT(SD_MAX_STARS));
		if (src == NULL) {
			err_printf("find_stars_cb: cannot create sources\n");
			return;
		}
		info_printf_sb2(window, "Searching for stars...");
//		d3_printf("Star det SNR: %f(%d)\n", P_DBL(SD_SNR), SD_SNR);
/* give the mainloop a change to redraw under the popup */
		while (gtk_events_pending ())
			gtk_main_iteration ();
		extract_stars(i_ch->fr, NULL, 0, P_DBL(SD_SNR), src);
		remove_stars_of_type(gsl, TYPE_MASK(STAR_TYPE_SIMPLE), 0);
/* now add to the list */
		for (i = 0; i < src->ns; i++) {
			if (src->s[i].peak > P_DBL(AP_SATURATION))
				continue;
			ref_flux = src->s[i].flux;
			ref_fwhm = src->s[i].fwhm;
			break;
		}
		for (i = 0; i < src->ns; i++) {
			if (src->s[i].peak > P_DBL(AP_SATURATION))
				continue;
			gs = gui_star_new();
			gs->x = src->s[i].x;
			gs->y = src->s[i].y;
			if (src->s[i].datavalid) {
				gs->size = star_size_flux(src->s[i].flux, ref_flux, ref_fwhm);
			} else {
				gs->size = 1.0 * P_INT(DO_DEFAULT_STAR_SZ);
			}
			gs->flags = STAR_TYPE_SIMPLE;
			gsl->sl = g_slist_prepend(gsl->sl, gs);
		}
		gsl->display_mask |= TYPE_MASK(STAR_TYPE_SIMPLE);
		gsl->select_mask |= TYPE_MASK(STAR_TYPE_SIMPLE);

		info_printf_sb2(window, "Found %d stars (SNR > %.1f)", src->ns, P_DBL(SD_SNR));
		release_sources(src);
		break;
	case ADD_STARS_OBJECT:
		if (i_ch == NULL || i_ch->fr == NULL)
			return;
		wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
		if (wcs == NULL || wcs->wcsset == WCS_INVALID) {
			err_printf_sb2(window, "Need an Initial WCS to Load objects");
			error_beep();
			return;
		}
		gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
		if (gsl == NULL) {
			gsl = gui_star_list_new();
			attach_star_list(gsl, window);
		}
		add_star_from_frame_header(i_ch->fr, gsl, wcs);
		gsl->display_mask |= TYPE_MASK_CATREF;
		gsl->select_mask |= TYPE_MASK_CATREF;
		break;
	case ADD_FROM_CATALOG:
		add_star_from_catalog(window);
		break;
	default:
		err_printf("find_stars_cb: unknown action %d \n", action);
		break;
	}
	gtk_widget_queue_draw(window);
}



/*
 * check that the distance between the star and (bx, by) is less than maxd
 */
int star_near_point(struct gui_star *gs, double bx, double by)
{
	double d;
	d = sqr(bx - gs->x) + sqr(by - gs->y);
	return (d < sqr((gs->size < 3) ? 3 : gs->size));
}

/*
 * search for stars near a given point. Return a newly-created
 * list of stars whose symbols cover the point, given in frame coords.
 * the supplied mask tells which star types to match; a null
 * mask will use the selection mask from the gsl structure.
 * if stars are not displayed, they will not be selected in any case.
 * The returned stars' reference count are _not_ increased, anyone
 * looking to hold on to the pointers, should ref them.
 */
GSList *search_stars_near_point(struct gui_star_list *gsl, double x, double y, int mask)
{
	GSList *ret_sl = NULL;
	GSList *sl;
	struct gui_star * gs;

	sl = gsl->sl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (!(TYPE_MASK_GSTAR(gs) & gsl->display_mask))
			continue;
		if (gs->flags & STAR_HIDDEN)
			continue;
		if (mask) {
			if (!(TYPE_MASK_GSTAR(gs) & mask))
				continue;
		} else {
			if (!(TYPE_MASK_GSTAR(gs) & gsl->select_mask))
				continue;
		}
		if (star_near_point(gs, x, y)) {
			ret_sl = g_slist_prepend(ret_sl, gs);
		}
	}
	return ret_sl;
}

/*
 * search for selected stars matching the type_mask. Return a newly-created
 * list of stars. A null mask will use the selection mask from the gsl structure.
 * If stars are not displayed, they will not be returned in any case.
 * The returned stars' reference count are _not_ increased, anyone
 * looking to hold on to the pointers, should ref them.
 */
GSList *gui_stars_selection(struct gui_star_list *gsl, int type_mask)
{
	GSList *ret_sl = NULL;
	GSList *sl;
	struct gui_star * gs;

	sl = gsl->sl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (!(TYPE_MASK_GSTAR(gs) & gsl->display_mask))
			continue;
		if (type_mask) {
			if (!(TYPE_MASK_GSTAR(gs) & type_mask))
				continue;
		} else {
			if (!(TYPE_MASK_GSTAR(gs) & gsl->select_mask))
				continue;
		}
		if (gs->flags & STAR_SELECTED) {
			ret_sl = g_slist_prepend(ret_sl, gs);
		}
	}
	return ret_sl;
}

/* get a list of gui stars of the given type */
GSList *gui_stars_of_type(struct gui_star_list *gsl, int type_mask)
{
	GSList *ret_sl = NULL;
	GSList *sl;
	struct gui_star * gs;

	sl = gsl->sl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (!(TYPE_MASK_GSTAR(gs) & type_mask))
			continue;
		ret_sl = g_slist_prepend(ret_sl, gs);
	}
	return ret_sl;
}


/*
 * search for selected stars matching the type_mask. Return a newly-created
 * list of stars. A null mask will use the selection mask from the gsl structure.
 * If stars are not displayed, they will not be returned in any case.
 * The returned stars' reference count are _not_ increased, anyone
 * looking to hold on to the pointers, should ref them.
 */
GSList *get_selection_from_window(GtkWidget *window, int type_mask)
{
	struct gui_star_list *gsl;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return NULL;
	return gui_stars_selection(gsl, type_mask);
}

/* look for stars under the mouse click
 * see search_stars_near_point for result description
 */
GSList * stars_under_click(GtkWidget *window, GdkEventButton *event)
{
	struct map_geometry *geom;
	struct gui_star_list *gsl;
	double x, y;
	GSList *found=NULL;

	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return NULL;
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return NULL;
	x = event->x / geom->zoom;
	y = event->y / geom->zoom;
	found = search_stars_near_point(gsl, x, y, 0);
	return found;
}



/* process button presses in star select mode
 */
void select_stars(GtkWidget *window, GdkEventButton *event, int multiple)
{
	struct map_geometry *geom;
	struct gui_star_list *gsl;
	struct gui_star * gs;
	double x, y;
	GSList *found, *sl;

	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return;
	x = event->x / geom->zoom;
	y = event->y / geom->zoom;
	found = search_stars_near_point(gsl, x, y, 0);
	d3_printf("looking for stars near %.0f, %.0f\n", x, y);

	sl = found;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		d3_printf("found [%08p] %.2f,%.2f  size %.0f\n",
			  gs, gs->x, gs->y, gs->size) ;
		gs->flags ^= STAR_SELECTED;
	}
	draw_star_list(found, window);
	g_slist_free(found);
}





/* re-generate and redraw catalog stars in window */
void redraw_cat_stars(GtkWidget *window)
{
	struct gui_star_list *gsl;
	struct wcs *wcs;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL)
		return;
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (gsl == NULL)
		return;

	cat_change_wcs(gsl->sl, wcs);

	star_list_update_size(window);
	star_list_update_labels(window);
}


/*
 * make the given star the only selected star matching the type mask
 */
void select_star_single(struct gui_star_list *gsl, struct gui_star *gsi, int type_mask)
{
	GSList *sl;
	struct gui_star *gs;

	sl = gsl->sl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (TYPE_MASK_GSTAR(gs) & type_mask) {
			gs->flags &= ~STAR_SELECTED;
		}
		if (gs == gsi) {
			gs->flags |= STAR_SELECTED;
		}
	}
}

struct gui_star *get_pairable_star(GSList *sl)
{
	struct gui_star *gs=NULL;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF)
		    && (!(gs->flags & STAR_HAS_PAIR)))
			return gs;
		sl = g_slist_next(sl);
	}
	return NULL;
}

/* look for the first paired star in found and remove it's pairing
 */
static void try_remove_pair(GtkWidget *window, GSList *found)
{
	GSList *sl;
	struct gui_star *gs;
	struct gui_star_list *gsl;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		g_warning("try_remove_pair: window really should have a star list\n");
		return;
	}
	sl = found;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		if (gs->flags & STAR_HAS_PAIR) {
			remove_pair_from(gs);
			break;
		}
		sl = g_slist_next(sl);
	}
	gtk_widget_queue_draw(window);
}

/* look for the first 'FR' type star and remove it from the star list
 */
static void try_unmark_star(GtkWidget *window, GSList *found)
{
	GSList *sl;
	struct gui_star *gs;
	struct gui_star_list *gsl;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		g_warning("try_unmark_pair: window really should have a star list\n");
		return;
	}
	sl = found;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
//		if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_FRSTAR) {
			remove_star(gsl, gs);
			break;
//		}
		sl = g_slist_next(sl);
	}
	if (sl == NULL) {
		err_printf_sb2(window, "No star to unmark");
		error_beep();
	}
	gtk_widget_queue_draw(window);
}

/*
 * try to create a star pair between stars from the window's selection
 * and the found list
 */
static void try_attach_pair(GtkWidget *window, GSList *found)
{
	GSList *selection = NULL, *pair = NULL, *sl;
	struct gui_star *gs = NULL, *cat_gs;
	struct gui_star_list *gsl;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		g_warning("try_attach_pair: window really should have a star list\n");
		return;
	}

	selection = gui_stars_selection(gsl, TYPE_MASK_ALL);

	sl = found;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF) {
			/* try to find possible unpaired match first */
			pair = filter_selection(selection, TYPE_MASK_FRSTAR, 0, STAR_HAS_PAIR);
			if (pair != NULL)
				break;
			pair = filter_selection(selection, TYPE_MASK_FRSTAR, 0, 0);
			if (pair != NULL)
				break;
		}
		if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_FRSTAR) {
			/* try to find possible unpaired match first */
			pair = filter_selection(selection, TYPE_MASK_CATREF, 0, STAR_HAS_PAIR);
			if (pair != NULL)
				break;
			pair = filter_selection(selection, TYPE_MASK_CATREF, 0, 0);
			if (pair != NULL)
				break;
		}
	}
	g_slist_free(selection);
	if (pair == NULL) {
		err_printf_sb2(window, "Cannot find suitable pair.");
		error_beep();
		return;
	}

	if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF) {
		cat_gs = gs;
		gs = GUI_STAR(pair->data);
		g_slist_free(pair);
	} else {
		cat_gs = GUI_STAR(pair->data);
		g_slist_free(pair);
	}

	info_printf_sb2(window, "Creating Star Pair");

	remove_pair_from(gs);
	search_remove_pair_from(cat_gs, gsl);
	gui_star_ref(cat_gs);
	cat_gs->flags |= STAR_HAS_PAIR;
	gs->flags |= STAR_HAS_PAIR;
	gs->pair = cat_gs;
	gtk_widget_queue_draw(window);
	return;
}


static void move_star(GtkWidget *window, GSList *found)
{
	GSList *selection = NULL, *sl;
	struct gui_star *gs = NULL, *cat_gs;
	struct gui_star_list *gsl;
	struct wcs *wcs;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		g_warning("try_attach_pair: window really should have a star list\n");
		return;
	}

	selection = gui_stars_selection(gsl, TYPE_MASK_ALL);

	if (g_slist_length(selection) != 1) {
		error_beep();
		info_printf_sb2(window,
				"Please select exactly one star as move destination\n");
		if (selection)
			g_slist_free(selection);
		return;
	}

	cat_gs = NULL;
	sl = found;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF) {
			if (gs->s == NULL)
				continue;
			if (CAT_STAR(gs->s)->flags & CAT_ASTROMET)
				continue;
			cat_gs = gs;
		}
	}
	gs = selection->data;
	g_slist_free(selection);
	if (cat_gs == NULL) {
		err_printf_sb2(window, "Only non-astrometric stars can be moved\n");
		error_beep();
		return;
	}

	cat_gs->x = gs->x;
	cat_gs->y = gs->y;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs != NULL)
		w_worldpos(wcs, gs->x, gs->y,
			   &CAT_STAR(cat_gs->s)->ra,
			   &CAT_STAR(cat_gs->s)->dec);

	gtk_widget_queue_draw(window);
	return;
}



/* temporary star printing */
static void print_star(struct gui_star *gs)
{
	d3_printf("gui_star x:%.1f y:%.1f size:%.1f flags %x\n",
		  gs->x, gs->y, gs->size, gs->flags);
	if (TYPE_MASK_GSTAR(gs) & TYPE_MASK(STAR_TYPE_CAT)) {
		d3_printf("         ra:%.5f dec:%.5f mag:%.1f name %16s\n",
			  CAT_STAR(gs->s)->ra, CAT_STAR(gs->s)->dec,
			  CAT_STAR(gs->s)->mag, CAT_STAR(gs->s)->name);
	}
}

/* temporary star printing */
static void print_stars(GtkWidget *window, GSList *found)
{
	GSList *sl;
	struct gui_star *gs;

	sl = found;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		print_star(gs);
		sl = g_slist_next(sl);
	}
}

void plot_profile(GtkWidget *window, GSList *found)
{
	struct gui_star_list *gsl;
	struct image_channel *i_ch;
	GSList *selection;
	int ret = 0;

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		err_printf("No gui stars\n");
		return;
	}
	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL) {
		err_printf("No image\n");
		return;
	}
	selection = gui_stars_selection(gsl, TYPE_MASK_ALL);
	if (found) {
		ret = do_plot_profile(i_ch->fr, found);
	} else if (selection) {
		ret = do_plot_profile(i_ch->fr, selection);
		g_slist_free(selection);
	} else {
		err_printf_sb2(window, "No stars selected");
		error_beep();
	}
	if (ret < 0) {
		err_printf_sb2(window, "%s", last_err());
		error_beep();
	}
}


/*
 * the item factory's data containg the list of stars under the cursor,
 * data is the image window. If selected stars are found under the cursor,
 * only those will be reach here.
 * The list must be copied and the stars must be ref'd
 * before holding a pointer to them.
 */
void star_popup_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *window = data;
	GSList *found, *sl;
	found = gtk_item_factory_popup_data_from_widget (menu_item);
	sl = found;
	switch(action) {
	case STARP_UNMARK_STAR:
		try_unmark_star(window, found);
		break;
	case STARP_PAIR_RM:
		try_remove_pair(window, found);
		break;
	case STARP_INFO:
		print_stars(window, found);
		break;
	case STARP_PAIR:
		try_attach_pair(window, found);
		break;
	case STARP_MOVE:
		move_star(window, found);
		break;
	case STARP_EDIT_AP:
		star_edit_dialog(window, found);
		break;
	case STARP_PROFILE:
		plot_profile(window, found);
		break;
	case STARP_MEASURE:
		print_star_measures(window, found);
		break;
	case STARP_SKYHIST:
		plot_sky_histogram(window, found);
		break;
	case STARP_FIT_PSF:
		do_fit_psf(window, found);
		break;
	default:
		g_warning("star_popup_cb: unknown action %d\n", action);
	}
}

/*
 * get a good position for the given itemfactory menu
 * (near the pointer, but visible if at the edge of screen)
 */
void popup_position (gpointer ifac, int *x, int *y)
{
	gint screen_width;
	gint screen_height;
	gint width = 100; /* wire the menu sizes; ugly,
			     but we have no way of knowing the size until
			     the menu is shown */
	gint height = 150;
	gdk_window_get_pointer (NULL, x, y, NULL);

	screen_width = gdk_screen_width ();
	screen_height = gdk_screen_height ();

	*x = CLAMP (*x - 2, 0, MAX (0, screen_width - width));
	*y = CLAMP (*y - 2, 0, MAX (0, screen_height - height));
}

/*
 * filter a selection to include only stars matching the star mask,
 * and_mask and or_mask. Return the narrowed list in a newly allocated
 * gslist. and_mask contains the bits we want to be 1, or_mask - the bits
 * we want to be 0.
 */
GSList *filter_selection(GSList *sl, int type_mask, guint and_mask, guint or_mask)
{
	GSList *filter = NULL;
	struct gui_star *gs;

	or_mask = ~or_mask;

	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if ( ((TYPE_MASK_GSTAR(gs) & type_mask) != 0) &&
		     ((and_mask & gs->flags) == and_mask) &&
		     ((or_mask | gs->flags) == or_mask) ) {
			filter = g_slist_prepend(filter, gs);
		}
	}
	return filter;
}

/*
 * fix and show the sources popup
 */
static void do_sources_popup(GtkWidget *window, GtkItemFactory *star_if,
			     GSList *found, GdkEventButton *event)
{
	gint cx, cy;
	GSList *selection = NULL, *pair = NULL, *sl, *push = NULL;
	GtkWidget *mi;
	struct gui_star *gs;
	struct wcs *wcs;

	int delp = 0, pairp = 0, unmarkp = 0, editp = 0, mkstdp = 0;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");

	popup_position(star_if, &cx, &cy);
	selection = get_selection_from_window(GTK_WIDGET(window), TYPE_MASK_ALL);

/* see if any stars under cursor are selected - if so, keep them in 'push'*/
	sl = found;
	while (sl != NULL) {
		if ((g_slist_find(selection, sl->data)) != NULL) {
			push = g_slist_prepend(push, sl->data);
//			d3_printf("intestected one!\n");
			unmarkp = 1;
		}
		sl = g_slist_next(sl);
	}

	if (push == NULL)
		push = found;
	else
		g_slist_free(found);

	sl = push;
	while (sl != NULL) {
//		d3_printf("push exam\n");
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (gs->flags & STAR_HAS_PAIR) {
		/* possible pair deletion */
			delp = 1;
		}
		if ((wcs != NULL) && (wcs->wcsset & WCS_VALID)) {
			mkstdp = 1;
			editp = 1;
		}
		if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF) {
			editp = 1;
			mkstdp = 1;
			if (TYPE_MASK_GSTAR(gs) & TYPE_MASK(STAR_TYPE_APSTD))
				mkstdp = 0;

			/* see if we have a possible pair in the selection */
			pair = filter_selection(selection, TYPE_MASK_FRSTAR, 0, 0);
			if (pair != NULL)
				pairp = 1;
			g_slist_free(pair);
		}
		if (TYPE_MASK_GSTAR(gs) & TYPE_MASK_FRSTAR) {
			unmarkp = 1;
			/* see if we have a possible pair in the selection */
			print_stars(window, selection);
			pair = filter_selection(selection, TYPE_MASK_CATREF, 0, 0);
			if (pair != NULL)
				pairp = 1;
			g_slist_free(pair);
		}
	}
	g_slist_free(selection);
/* fix the menu */
	mi = gtk_item_factory_get_widget(star_if, "/Create Pair");
	if (mi != NULL)
		gtk_widget_set_sensitive(mi, pairp);

	mi = gtk_item_factory_get_widget(star_if, "/Move Star");
	if (mi != NULL)
		gtk_widget_set_sensitive(mi, pairp);

	mi = gtk_item_factory_get_widget(star_if, "/Remove Pair");
		if (mi != NULL)
			gtk_widget_set_sensitive(mi, delp);

	mi = gtk_item_factory_get_widget(star_if, "/Remove Star");
		if (mi != NULL)
			gtk_widget_set_sensitive(mi, unmarkp);

	mi = gtk_item_factory_get_widget(star_if, "/Make Std Star");
		if (mi != NULL)
			gtk_widget_set_sensitive(mi, mkstdp);

	mi = gtk_item_factory_get_widget(star_if, "/Edit Star");
		if (mi != NULL)
			gtk_widget_set_sensitive(mi, editp);

	gtk_item_factory_popup_with_data( star_if, push,
					  (GDestroyNotify)g_slist_free,
					  cx, cy, event->button,
					  event->time);
}

/* toggle selection of the given star list */
void toggle_selection(GtkWidget *window, GSList *stars)
{
	GSList *sl;
	struct gui_star *gs;

	sl = stars;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		gs->flags ^= STAR_SELECTED;
		draw_gui_star(gs, window);
		sl = g_slist_next(sl);
	}

}

/* make the given star the only one slected */
void single_selection_gs(GtkWidget *window, struct gui_star *gs)
{
	GSList *selection = NULL;

	selection = get_selection_from_window(GTK_WIDGET(window), TYPE_MASK_ALL);
	if (selection != NULL) {
		toggle_selection(window, selection);
		g_slist_free(selection);
	}
	gs->flags |= STAR_SELECTED;
	draw_gui_star(gs, window);
}


/* make the given star the only one slected */
void gsl_unselect_all(GtkWidget *window)
{
	GSList *selection = NULL;

	selection = get_selection_from_window(GTK_WIDGET(window), TYPE_MASK_ALL);
	if (selection != NULL) {
		toggle_selection(window, selection);
		g_slist_free(selection);
	}
}


static void edit_star_hook(struct gui_star *gs, GtkWidget *window);

/* make one star from list the only one selected from the window */
void single_selection(GtkWidget *window, GSList *stars)
{
	GSList *sl, *selection = NULL;
	struct gui_star *gs = NULL;

	if (stars == NULL)
		return;
	sl = stars;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (gs->flags & STAR_SELECTED) {
			gs->flags &= ~STAR_SELECTED;
			draw_gui_star(gs, window);
			break;
		}
	}
	if (sl == NULL) {
		gs = GUI_STAR(stars->data);
	} else {
		gs = GUI_STAR(sl->data);
	}
//	gs->flags &= ~STAR_SELECTED;
	selection = get_selection_from_window(GTK_WIDGET(window), TYPE_MASK_ALL);
	toggle_selection(window, selection);
	g_slist_free(selection);
	gs->flags |= STAR_SELECTED;
	draw_gui_star(gs, window);
	edit_star_hook(gs, window);
}

/* detect a star under the given position */
void detect_add_star(GtkWidget *window, double x, double y)
{
	struct image_channel *i_ch;
	struct map_geometry *geom;
	struct gui_star_list *gsl;
	struct gui_star *gs;
	struct star s;
	int ix, iy;
	int ret;

	d3_printf("det_add_star\n");
	geom = g_object_get_data(G_OBJECT(window), "geometry");
	if (geom == NULL)
		return;
	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL)
		return;
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		gsl = gui_star_list_new();
		attach_star_list(gsl, window);
	}

	ix = (x) / geom->zoom;
	iy = (y) / geom->zoom;

	d3_printf("trying to get star near %d,%d\n", ix, iy);
	ret = get_star_near(i_ch->fr, ix, iy, 0, &s);
	if (!ret) {
		d3_printf("s %.1f %.1f\n", s.x, s.y);
		gs = gui_star_new();
		gs->x = s.x;
		gs->y = s.y;
		gs->size = 1.0 * P_INT(DO_DEFAULT_STAR_SZ);

		gs->flags = STAR_TYPE_USEL;
		gsl->sl = g_slist_prepend(gsl->sl, gs);

		gsl->display_mask |= TYPE_MASK(STAR_TYPE_USEL);
		gsl->select_mask |= TYPE_MASK(STAR_TYPE_USEL);
		single_selection_gs(window, gs);
	}
}

/* print a short info line about the star */
static void sprint_star(char *buf, int len, struct gui_star *gs, struct wcs *wcs)
{
	int pos = 0;
	char ras[32];
	char decs[32];
	double xpos, ypos;
	if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF) && (gs->s)) {
		degrees_to_dms_pr(ras, CAT_STAR(gs->s)->ra / 15.0, 2);
		degrees_to_dms_pr(decs, CAT_STAR(gs->s)->dec, 1);
		if (CAT_STAR(gs->s) == NULL || CAT_STAR(gs->s)->smags == NULL)
			snprintf(buf, len, "%s [%.1f,%.1f] Ra:%s Dec:%s Mag:%.1f",
				CAT_STAR(gs->s)->name, gs->x, gs->y, ras, decs,
				CAT_STAR(gs->s)->mag);
		else
			snprintf(buf, len, "%s [%.1f,%.1f] Ra:%s Dec:%s %s",
				CAT_STAR(gs->s)->name, gs->x, gs->y, ras, decs,
				CAT_STAR(gs->s)->smags);
		return;
	}
	if (wcs == NULL || wcs->wcsset == WCS_INVALID) {
		pos = snprintf(buf, len, "Field Star at x:%.1f y:%.1f size:%.1f",
			      gs->x, gs->y, gs->size);
	} else {
		w_worldpos(wcs, gs->x, gs->y, &xpos, &ypos);
		degrees_to_dms_pr(ras, xpos / 15.0, 2);
		degrees_to_dms_pr(decs, ypos, 1);
		pos = snprintf(buf, len, "Field Star [%.1f,%.1f] Ra:%s Dec:%s %s size:%.1f",
			      gs->x, gs->y, ras, decs,
			      (wcs->wcsset & WCS_VALID) ? "" : "(uncertain)",
			      gs->size);
	}
	return;
}

/* if the star edit dialog is open, pust the selected star into it */
static void edit_star_hook(struct gui_star *gs, GtkWidget *window)
{
	GtkWidget *dialog;

	dialog = g_object_get_data(G_OBJECT(window), "star_edit_dialog");
	if (dialog == NULL || gs->s == NULL)
		return;
	star_edit_star(window, CAT_STAR(gs->s));
}


static void show_star_data(GSList *found, GtkWidget *window)
{
	GSList *filter;
	char buf[128];
	struct wcs *wcs;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");

	filter = filter_selection(found, TYPE_MASK_ALL, STAR_SELECTED, 0);
	if (filter != NULL) {
		sprint_star(buf, 127, GUI_STAR(filter->data), wcs);
		g_slist_free(filter);
	} else {
		sprint_star(buf, 127, GUI_STAR(found->data), wcs);
	}
//	d3_printf("star data %s\n", buf);
	info_printf_sb2(window, buf);
}


/*
 * the callback for sources ops. If we don't have a source op to do,
 * we return FALSE, so the next callback can handle image zooms/pans
 */
gint sources_clicked_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	GSList *found, *filt;
	GtkItemFactory *star_if;

	d3_printf("star_popup\n");
	switch(event->button) {
	case 3:
		found = stars_under_click(GTK_WIDGET(data), event);
		if (found != NULL) {
			star_if = g_object_get_data(G_OBJECT(data), "star_popup_if");
			if (star_if == NULL) {
				g_slist_free(found);
				return FALSE;
			}
			do_sources_popup(data, star_if, found, event);
//			d3_printf("star_popup returning TRUE\n");
			return TRUE;
		}
		return FALSE;
	case 1:
		found = stars_under_click(GTK_WIDGET(data), event);
		if (event->state & GDK_CONTROL_MASK) {
			d3_printf("ctrl-1\n");
			filt = filter_selection(found, TYPE_MASK_FRSTAR, 0, 0);
			if (filt == NULL)
				detect_add_star(data, event->x, event->y);
			g_slist_free(filt);
			return TRUE;
		}
		if (found == NULL)
			return FALSE;
		if (event->state & GDK_SHIFT_MASK)
			toggle_selection(data, found);
		else
			single_selection(data, found);
		show_star_data(found, data);
		return TRUE;
	default :
		break;
	}
	return FALSE;
}

/* adjust star display options */
void star_display_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	switch(action) {
	case STAR_FAINTER:
		if (P_DBL(DO_DLIM_MAG) < 25) {
			P_DBL(DO_DLIM_MAG) += 1.0;
			par_touch(DO_DLIM_MAG);
		}
		info_printf_sb2(window, "Display limiting magnitude is %.1f", P_DBL(DO_DLIM_MAG));
		break;
	case STAR_BRIGHTER:
		if (P_DBL(DO_DLIM_MAG) > 0) {
			P_DBL(DO_DLIM_MAG) -= 1.0;
			par_touch(DO_DLIM_MAG);
		}
		info_printf_sb2(window, "Display limiting magnitude is %.1f", P_DBL(DO_DLIM_MAG));
		break;
	case STAR_REDRAW:
		info_printf_sb2(window, "Display limiting magnitude is %.1f", P_DBL(DO_DLIM_MAG));
		break;
	}
	star_list_update_size(window);
	star_list_update_labels(window);
	gtk_widget_queue_draw(window);
}

void star_edit_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	GSList *sl = NULL;

	sl = get_selection_from_window(window, TYPE_MASK_ALL);
	if (sl == NULL) {
		err_printf_sb2(window, "No stars selected");
		return;
	}
	do_edit_star(window, sl, 0);
	g_slist_free(sl);

}
