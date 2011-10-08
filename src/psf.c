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

/* psf analysis and extraction */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

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
#include "dsimplex.h"

#define GROWTH_POINTS 128
#define growth_radius(s) (1.0 + 0.5 * (s))

static int grow_1_walk[] = {0,1, -1,0, 0,-1, 0,-1, 1,0, 1,0, 0,1, 0,1, 0,0 };
static int grow_2_walk[] = {0,1, -1,0, 0,-1, 0,-1, 1,0, 1,0, 0,1, 0,1,
			    0,1, -1,0, -1,0, -1,-1, 0,-1, 0,-1, 1,-1,
			    1,0, 1,0, 1,1, 0,1, 0,1, 0,0 };
static int grow_3_walk[] = {0,1, -1,0, 0,-1, 0,-1, 1,0, 1,0, 0,1, 0,1,
			    0,1, -1,0, -1,0, -1,-1, 0,-1, 0,-1, 1,-1,
			    1,0, 1,0, 1,1, 0,1, 0,1,
			    0,1, -1,1, -1,0, -1,0, -1,-1, -1,-1, 0,-1, 0,-1,
			    1,-1, 1,-1, 1,0, 1,0, 1,1, 1,1, 0,1, 0,1, 0,0 };

/* create a gaussian profile with a sigma of s, centered on
   xc, yc. the total generated volume is returned.
   the psf is oversampled by ovs */
static double gaussian_psf(struct psf *psf, double ovs, double s)
{
	int xi, yi;
	double rs, v, vol = 0.0;

	s *= ovs;
	d3_printf("gauss_psf: s=%.3f\n", s);

	for (xi = 0; xi < psf->w; xi ++)
		for (yi = 0; yi < psf->h; yi++) {
			rs = sqr(xi - psf->cx - psf->dx) + sqr(yi - psf->cy - psf->dy);
			v = exp( - rs / 2 / sqr(s));
			vol += v;
			psf->d[xi][yi] = v;
		}
	return vol;
}

/* create a gaussian profile with a sigma of s and beta of b, centered on
   xc, yc. the total generated volume is returned.
   the psf is oversampled by ovs */

double moffat_psf(struct psf *psf, double ovs, double s, double b)
{
	int xi, yi;
	double rs, v, vol = 0.0;


	s = s * ovs / (4 * (pow(2, 1/b) - 1));

	d3_printf("moffat_psf: s=%.3f b = %.1f\n", s, b);

	for (xi = 0; xi < psf->w; xi ++)
		for (yi = 0; yi < psf->h; yi++) {
			rs = sqr(xi - psf->cx - psf->dx) + sqr(yi - psf->cy - psf->dy);
			v = pow(1 + rs / sqr(s), -b);
			vol += v;
			psf->d[xi][yi] = v;
		}
	return vol;
}

/* create a new psf (or aperture) of given size. the reference pixel is set in
   the middle. the psf data is not initialised.*/
struct psf *psf_new(int w, int h)
{
	struct psf *psf;
	float *f;
	float **d;
	int i;

	psf = calloc(1, sizeof(struct psf));
	g_assert(psf != NULL);

	d = malloc(w * h * sizeof(float) + w * sizeof(float *));
	g_assert(d != NULL);

	f = (float *)(d + w);
	for (i = 0; i < w; i++) {
		d[i] = f;
		f += h;
	}

	psf->d = d;
	psf->w = w;
	psf->h = h;
	psf->cx = w/2;
	psf->cy = h/2;
	psf->dx = 1.0 * w / 2 - psf->cx;
	psf->dy = 1.0 * w / 2 - psf->cy;
	psf->ref_count = 1;
	return psf;
}

void psf_release(struct psf *psf)
{
	g_return_if_fail(psf != NULL);

	if (psf->ref_count > 1) {
		psf->ref_count--;
		return;
	}
	free(psf);
}

/* set the pixels in an annular aperture around the ref pixel to 1, the rest to 0 */
void make_annular_aperture(struct psf *apert, double r1, double r2)
{
	int x, y;
	double d, r1s, r2s;
	r1s = sqr(r1);
	r2s = sqr(r2);

	for (x = 0; x < apert->w; x++)
		for (y = 0; y < apert->h; y++) {
			d = sqr(x - apert->cx - apert->dx) + sqr(y - apert->cy - apert->dy);
			if (d >= r1s && d < r2s)
				apert->d[x][y] = 1;
			else
				apert->d[x][y] = 0;
		}
}

/* create a circular aperture centered on the reference pixel*/
void make_circular_aperture(struct psf *apert, double r1)
{
	int x, y;
	double d;

	for (x = 0; x < apert->w; x++)
		for (y = 0; y < apert->h; y++) {
			d = sqrt(sqr(x - apert->cx - apert->dx) +
				 sqr(y - apert->cy - apert->dy));
			if (d > r1 + 0.5)
				apert->d[x][y] = 0;
			else if (d < r1 - 0.5)
				apert->d[x][y] = 1.0;
			else
				apert->d[x][y] = r1 + 0.5 - d;
		}
}


/* compute stats for the in-aperture pixels */
void aperture_stats(struct ccd_frame *fr, struct psf *psf, double x, double y,
		    struct stats *rs)
{
	int xs, xe, ys, ye;
	float *vals;
	int nvals;
	int ix, iy;
	int rx, ry;
	int h_offset, w_offset;
	float d;
	rs->min = HUGE;
	rs->max = -HUGE;

	xs = ys = 0;
	xe = psf->w;
	ye = psf->h;
	rx = floor(x + 0.5);
	ry = floor(y + 0.5);

	if (rx < 1)
		rx = 1;
	if (rx > fr->w - 1)
		rx = fr->w -1;
	if (ry < 1)
		ry = 1;
	if (ry > fr->h - 1)
		rx = fr->h -1;

	if (rx < psf->cx)
		xs = psf->cx - rx;
	if (ry < psf->cy)
		ys = psf->cy - ry;

	if (rx + psf->w - psf->cx > fr->w)
		xe = fr->w + psf->cx - rx;
	if (ry + psf->h - psf->cy > fr->h)
		ye = fr->h + psf->cy - ry;

	if (xe - xs >= psf->w)
		xe = xs + psf->w;
	if (ye - ys >= psf->h)
		ye = ys + psf->h;

	d4_printf("xs:%d xe:%d ys:%d ys:%d\n", xs, xe, ys, ye);

	nvals = rs->sum = rs->sumsq = rs->all = 0;
	vals = malloc(psf->h * psf->w * sizeof(vals));
	g_assert(vals != NULL);

	w_offset = rx - psf->cx;
	h_offset = ry - psf->cy;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			d = get_pixel_luminence(fr, w_offset + ix, h_offset + iy);
			if (psf->d[ix][iy] >= 0.5) {
				vals[nvals++] = d;
				if (d > rs->max) {
					rs->max = d;
					rs->max_x = rx + ix - psf->cx;
					rs->max_y = ry + iy - psf->cy;
				}
				if (d < rs->min)
					rs->min = d;
			}
			rs->sum += d * psf->d[ix][iy];
			rs->sumsq += sqr(d) * psf->d[ix][iy];
			rs->all += psf->d[ix][iy];
		}
	}
	rs->median = fmedian(vals, nvals);
	if (rs->all != 0.0) {
		rs->avg = rs->sum/rs->all;
		rs->sigma = sqrt(rs->sumsq / rs->all - sqr(rs->sum / rs->all));
	} else {
		rs->avg = rs->sigma = 0.0;
	}
}



/* multiply the aperture by the pixel values */
void aperture_multiply(struct ccd_frame *fr, struct psf *psf, double x, double y)
{
	int xs, xe, ys, ye;
	int ix, iy;
	int rx, ry;
	int w_offset, h_offset;

	xs = ys = 0;
	xe = psf->w;
	ye = psf->h;
	rx = floor(x + 0.5);
	ry = floor(y + 0.5);

	if (rx < psf->cx)
		xs = psf->cx - rx;
	if (ry < psf->cy)
		ys = psf->cy - ry;

	if (rx + psf->w - psf->cx > fr->w)
		xe = fr->w + psf->cx - rx;
	if (ry + psf->h - psf->cy > fr->h)
		ye = fr->h + psf->cy - ry;

	d4_printf("xs:%d xe:%d ys:%d ys:%d\n", xs, xe, ys, ye);

	w_offset = rx - psf->cx;
	h_offset = ry - psf->cy;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			psf->d[ix][iy] *= get_pixel_luminence(fr, w_offset + ix, h_offset + iy);
		}
	}
}

/* extract a patch from the frame into the psf */
static void extract_patch(struct ccd_frame *fr, struct psf *psf, double x, double y)
{
	int xs, xe, ys, ye;
	int ix, iy;
	int rx, ry;
	float d;
	int w_offset, h_offset;

	xs = ys = 0;
	xe = psf->w;
	ye = psf->h;
	rx = floor(x + 0.5);
	ry = floor(y + 0.5);
	psf->dx = 0;
	psf->dx = 0;

	if (rx < psf->cx)
		xs = psf->cx - rx;
	if (ry < psf->cy)
		ys = psf->cy - ry;

	if (rx + psf->w - psf->cx > fr->w)
		xe = fr->w + psf->cx - rx;
	if (ry + psf->h - psf->cy > fr->h)
		ye = fr->h + psf->cy - ry;

	d4_printf("xs:%d xe:%d ys:%d ys:%d\n", xs, xe, ys, ye);

	w_offset = rx - psf->cx;
	h_offset = ry - psf->cy;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			psf->d[ix][iy] = get_pixel_luminence(fr, w_offset + ix, h_offset + iy);
			d++;
		}
	}
}


/* grow a region around the given pixel */
void pixel_grow(struct psf *psf, int ix, int iy, int grow)
{
	int *walk;
	int i;
	int dx, dy;
	float v;

	v = psf->d[ix][iy];


	switch(grow) {
	case 0:
		return;
	case 1:
		walk = grow_1_walk;
		break;
	case 2:
		walk = grow_2_walk;
		break;
	case 3:
		walk = grow_3_walk;
		break;
	default:
		walk = grow_3_walk;
		d1_printf("clamping grow to 3\n");
		break;
	}

	i = 0;
	do {
		dx = walk[i++];
		dy = walk[i++];
		ix += dx;
		iy += dy;
//		d4_printf("%d,%d ", ix, iy);
		if ((ix >= 0) && (ix < psf->w) & (iy >=0) && (iy < psf->h)) {
			psf->d[ix][iy] = v;
		}
	} while ((dx != 0) || (dy != 0));
//	d4_printf("\n");
}


/* dodge high points in the aperture by zeroing the aperture function for
 * points > ht. the points are region-grown by grow pixels. return the number of points dodged */
int dodge_high(struct ccd_frame *fr, struct psf *psf, double x, double y,
		double ht, int grow)
{
	int xs, xe, ys, ye;
	int ix, iy, n = 0;
	int w_offset, h_offset;
	int rx, ry;

	xs = ys = 0;
	xe = psf->w;
	ye = psf->h;

	rx = floor(x + 0.5);
	ry = floor(y + 0.5);

	if (rx < psf->cx)
		xs = psf->cx - rx;
	if (ry < psf->cy)
		ys = psf->cy - ry;

	if (rx + psf->w - psf->cx > fr->w)
		xe = fr->w + psf->cx - rx;
	if (ry + psf->h - psf->cy > fr->h)
		ye = fr->h + psf->cy - ry;

	w_offset = rx - psf->cx;
	h_offset = ry - psf->cy;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			if (get_pixel_luminence(fr, w_offset + ix, h_offset + iy) > ht) {
				if (psf->d[ix][iy] > 0)
					n++;
				psf->d[ix][iy] = 0;
				pixel_grow(psf, ix, iy, grow);
			}
		}
	}
	return n;
}





int growth_curve(struct ccd_frame *fr, double x, double y, double grc[], int n)
{
	struct ap_params apdef;
	struct star s;
	int i, ret;
	double mflux = 1;
	struct stats stats;
	double sky, flux;

	ap_params_from_par(&apdef);

	memset(&s, 0, sizeof(struct star));
	s.x = x;
	s.y = y;
	s.xerr = BIG_ERR;
	s.yerr = BIG_ERR;
	if (P_INT(AP_AUTO_CENTER)) {
		center_star(fr, &s, P_DBL(AP_MAX_CENTER_ERR));
	}
	apdef.center = P_INT(AP_AUTO_CENTER);
	ret = aperture_photometry(fr, &s, &apdef, NULL);
	if (ret)
		return ret;
	apdef.center = 0;

	sky = get_sky(fr, s.x, s.y, &apdef, NULL, &stats, NULL, NULL);

	for (i = 0; i < n; i++) {
		apdef.r1 = growth_radius(i);
		flux = get_star(fr, s.x, s.y, &apdef, &stats, NULL);
		grc[i] = flux - stats.all * sky;
		d4_printf("r:%.1f tflux:%.1f star_all:%.3f\n",
			  apdef.r1, flux, stats.all);
		if (apdef.r1 <= P_DBL(AP_R1))
			mflux = grc[i];
	}
	for (i = 0; i < n; i++) {
		grc[i] = grc[i] / mflux;
	}
	return 0;
}

/* fill up the rp_point table with radius/intensity pairs. return the number of points filled */
int radial_profile(struct ccd_frame *fr, double x, double y, double r,
		   struct rp_point rpp[], int n, double *peak, double *flux,
		   double *sky, double *err)
{
	struct ap_params apdef;
	struct star s;
	int ret, ns = 0;
	float d;
	int ri, i;
	int ix, iy, xs, ys;// xe, ye;
	double max = 1.0;

	ap_params_from_par(&apdef);
	memcpy(&apdef.exp, &fr->exp, sizeof(struct exp_data));

	memset(&s, 0, sizeof(struct star));
	s.x = x;
	s.y = y;
	s.xerr = BIG_ERR;
	s.yerr = BIG_ERR;
	if (P_INT(AP_AUTO_CENTER))
		center_star(fr, &s, P_DBL(AP_MAX_CENTER_ERR));
	ret = aphot_star(fr, &s, &apdef, NULL);
	if (ret) {
		err_printf("radial_profile: aphot err\n");
		return ret;
	}

	ri = 2 * ceil(r);
	xs = floor(x - r);
	ys = floor(y - r);
//	xe = xs + ri;
//	ye = ys + ri;
	g_return_val_if_fail(ri < fr->w, 0);
	for (iy = 0; iy < ri && ys + iy < fr->h; iy++) {
		for (ix = 0; ix < ri && xs + ix < fr->w; ix++) {
			double dist;
			dist = sqrt(sqr(ix + xs - x) + sqr(iy + ys - y));
			if (ns < n && dist <= r) {
				d = get_pixel_luminence(fr, xs + ix, ys + iy);
//				d4_printf("dist %8.1f v:%.1f\n", dist, d);
				rpp[ns].r = dist;
				rpp[ns].v = d;
				if (dist < r/2 && max < d)
					max = d;
				ns++;
			}
		}
	}
	for (i = 0; i < ns; i++) {
		if ((max - s.aph.sky) / max > 0.001)
			rpp[i].v = (rpp[i].v - s.aph.sky) / (max - s.aph.sky);
		else
			rpp[i].v = (rpp[i].v) / (max);
	}
	if (peak)
		*peak = max;
	if (sky)
		*sky = s.aph.sky;
	if (flux)
		*flux = s.aph.star;
	if (err)
		*err = s.aph.magerr;
	if (ns < n) {
		rpp[ns].v = -1;
		rpp[ns].r = -1;
		return ns;
	} else {
		rpp[ns].v = -1;
		rpp[ns].r = -1;
		return ns - 1;
	}
}

/* minimisation error function (gaussian model, looking for peak and sigma).
   p[1] is the peak, p[2] is sigma, data is a radial profile. The error is
   only calculated up to three sigmas. the profiles end with an entry with
   negative radius */
double gauss_1d_2_err_funk(double p[], void *rppi)
{
	double A = p[1];
	double s = p[2];
	int i;
	double err = 0;
	struct rp_point *rpp = rppi;

	if (s < 0.01)
		return 1000;

	i = 0;
	while (rpp[i].r >= 0) {
			err += sqr(A * exp(-0.5 * sqr((rpp[i].r) / s)) - rpp[i].v);
		i++;
	}
//	d4_printf("A:%.3f s:%.3f err:%.3f\n", A, s, err);
	return err;
}

/* fit an unidimensional gaussian or moffat profile. if B is NULL,
   the background is assumed 0 (not fitted). if b is NULL, a gaussian
   is fitted, else a moffat function. */
int fit_1d_profile(struct rp_point *rpp, double *A, double *B, double *s, double *b)
{
	double **p;		/* np+1 rows, np columns (+1 due to 1-based) */
	double *y;		/* np columns ( " ) */
	int iter=0;
	int i;
	int ret;
	int np;

	np = 2;

	g_return_val_if_fail(A != NULL, -1);
	g_return_val_if_fail(s != NULL, -1);

	p = (double **) malloc((np + 2) * sizeof(double *));
	y = (double *) malloc((np + 2) * sizeof(double));

	for (i = 1; i <= np + 1; i++) {
		p[i] = (double *) malloc((np + 1) * sizeof(double));
	}

	p[1][1] = *A;
	p[1][2] = *s;

	p[2][1] = *A * 1.1;
	p[2][2] = *s;

	p[3][1] = *A;
	p[3][2] = *s * 1.1;

	for (i = 1; i <= np+1; i++) {
		y[i] = gauss_1d_2_err_funk(p[i], rpp);
	}

	ret = amoeba(p, y, np, 0.01, gauss_1d_2_err_funk, &iter, rpp);

	d4_printf("it:%d evals:%d s:%.3f A:%.3f\n", iter, ret, p[1][2], p[1][1]);

	*A = p[1][1];
	*s = p[1][2];

	for (i = 1; i < np + 2; i++)
		free((void *) p[i]);
	free((void *) p);
	free((void *) y);

	return ret;
}

/* calculate the sky value in an area around x,y. Return the sky value
   Update the stats with the statistics of the pixels used, and err with
   the estimated error of the sky. if psf is non-null, use it to return the
   sky aperture function. */
double get_sky(struct ccd_frame *fr, double x, double y, struct ap_params *p,
	       double *err, struct stats *stats, struct psf **apert,
	       double *allpixels)
{
	struct psf *psf;
	int ret;
	double so, th;
	double s = 0.0, e = BIG_ERR;
	double ap;

	g_return_val_if_fail(stats != NULL, 0.0);

	psf = psf_new((int) (2 + 2 * ceil(p->r3)), (int) (1 + 2 * ceil(p->r3)) );
	make_annular_aperture(psf, p->r2, p->r3);

	aperture_stats(fr, psf, x, y, stats);
	ap = stats->all;
	switch(p->sky_method) {
	case APMET_KS:
	case APMET_SYMODE:
		do {
			so = stats->sigma;
			th = stats->median + p->sigmas * stats->sigma;
			ret = dodge_high(fr, psf, x, y, th, p->grow);
			aperture_stats(fr, psf, x, y, stats);
			d4_printf("dodged %d>%.1f, aperture avg:%.1f median:%.1f sigma:%.1f\n",
				  ret, th, stats->avg, stats->median, stats->sigma);
		} while (stats->sigma != so && stats->all > 10);
		break;
	}

	switch(p->sky_method) {
	case PAR_SKY_METHOD_AVERAGE:
		s = stats->avg;
		e = stats->sigma / sqrt(stats->all);
		break;
	case PAR_SKY_METHOD_MEDIAN:
		s = stats->median;
		e = stats->sigma / sqrt(stats->all) / 0.65;
		break;
	case PAR_SKY_METHOD_KAPPA_SIGMA:
		s = stats->avg;
		e = stats->sigma / sqrt(stats->all);
		break;
	case PAR_SKY_METHOD_SYNTHETIC_MODE:
		s = 3 * stats->median - 2 * stats->avg;
		e = stats->sigma / sqrt(stats->all);
		break;
	default:
		err_printf("get_sky: unknown method %d\n", p->sky_method);
		break;
	}
	if (apert)
		*apert = psf;
	else
		psf_release(psf);
	if (err)
		*err = e;
	if (allpixels)
		*allpixels = ap;
	return s;
}


/* return the flux in an area around x,y.
   Update the stats with the statistics of the pixels used.
   if psf is non-null, use it to return the
   aperture function. */
double get_star(struct ccd_frame *fr, double x, double y, struct ap_params *p,
		struct stats *stats, struct psf **apert)
{
	struct psf *psf;
	double f;

	psf = psf_new((int) (3 + 2 * ceil(p->r1)), (int) (3 + 2 * ceil(p->r1)));
	psf->dx = x - floor(x + 0.5);
	psf->dy = y - floor(y + 0.5);
	make_circular_aperture(psf, p->r1);
	aperture_stats(fr, psf, x, y, stats);

	f = stats->sum;

	if (apert)
		*apert = psf;
	else
		psf_release(psf);

	return f;
}

int aphot_star(struct ccd_frame *fr, struct star *s,
	       struct ap_params *p, struct bad_pix_map *bp)
{
	double sky, sky_err, flux;
	struct stats sky_stats;
	struct stats star_stats;
	double phn, flnsq;

	if (s->x < 0 || s->x > fr->w - 1 || s->y < 0 || s->y > fr->h - 1) {
		return -1;
	}

	sky = get_sky(fr, s->x, s->y, p, &sky_err, &sky_stats, NULL, NULL);
	flux = get_star(fr, s->x, s->y, p, &star_stats, NULL);

	s->aph.tflux = flux;
	s->aph.star_all = star_stats.all;
	s->aph.sky = sky;
	s->aph.sky_err = sky_err;
	s->aph.star_max = star_stats.max;

	s->aph.star = flux - star_stats.all * sky;

	phn = sqrt(fabs(s->aph.tflux - p->exp.bias * s->aph.star_all)
		   * p->exp.scale) / p->exp.scale; // total photon shot noise
	flnsq = sqr(p->exp.flat_noise) * (star_stats.sumsq);
	s->aph.flux_err = sqrt((sqr(p->exp.rdnoise) * s->aph.star_all + sqr(phn) + flnsq));
	s->aph.pshot_noise = phn;
	s->aph.rd_noise = fr->exp.rdnoise * sqrt(s->aph.star_all);
	d4_printf(">>>>peak: %.1f sat_limit: %.1f\n",
		  s->aph.star_max, p->sat_limit);
	if (s->aph.star_max > p->sat_limit)
		s->aph.flags |= AP_BURNOUT;

	s->aph.star_err = sqrt(sqr(s->aph.flux_err) + s->aph.star_all * sqr(s->aph.sky_err));
//	d3_printf("get star: star_err %.5g flux_err %.5g sky_err %.5g\n", s->aph.star_err,
//		  s->aph.flux_err, s->aph.sky_err);

	if (s->aph.star < 3 * s->aph.star_err)
		s->aph.flags |= AP_FAINT;
// clip to protect the logarithms
	if (s->aph.star < MIN_STAR)
		s->aph.star = MIN_STAR;
	if (s->aph.star_err < MIN_STAR)
		s->aph.star_err = MIN_STAR;

	s->aph.absmag = flux_to_absmag(s->aph.star);
	s->aph.magerr = 1.08 * fabs(s->aph.star_err / s->aph.star);
//	d3_printf("get star: absmag:%.4g magerr:%.4g\n", s->aph.absmag,
//		  s->aph.magerr);
	s->aph.flags |= AP_MEASURED;
	return 0;
}

/* 2d psf fitting */

/* return the flux in a "superpixel" sub pixels wide, with */
/* the center at x, y (referenced to the psf center). */

static float psf_subsample_pixel(struct psf *psf, double sub, double x, double y)
{
	double xc = x*sub + psf->cx + psf->dx - 0.5; /* center of superpixel in psf coords */
	double yc = y*sub + psf->cy + psf->dy - 0.5;

	int fx, lx;		/* first and last pixel considered */
	float fxw, lxw;		/* weight of the first and last
				   pixel (in-between the weight is 1) */
	int fy, ly;
	float fyw, lyw;

	double p;
	float v;
	int ix, iy, n = 0;

	p = xc - sub / 2;
	if (p < 0)
		p = 0;
	fx = floor(p);
	if (fx > psf->w - 1)
		return 0.0;
	fxw = 1.0 - (p - fx);

	p = xc + sub / 2;
	if (p > psf->w - 1)
		p = psf->w - 1;
	lx = floor(p);
	if (lx < 0)
		return 0.0;
	lxw = p - lx;

	p = yc - sub / 2;
	if (p < 0)
		p = 0;
	fy = floor(p);
	if (fy > psf->h - 1)
		return 0.0;
	fyw = 1.0 - (p - fy);

	p = yc + sub / 2;
	if (p > psf->h - 1)
		p = psf->h - 1;
	ly = floor(p);
	if (ly < 0)
		return 0.0;
	lyw = p - ly;


	v = psf->d[fx][fy] * fxw * fyw + /* corners */
		psf->d[fx][ly] * fxw * lyw +
		psf->d[lx][fy] * lxw * fyw +
		psf->d[lx][ly] * lxw * lyw;
	for (ix = fx + 1; ix < lx ; ix++) {
		v += psf->d[ix][fy] * fyw; /* top/bot sides */
		v += psf->d[ix][ly] * lyw;
	}
	for (iy = fy + 1; iy < ly ; iy ++) {
		v += psf->d[fx][iy] * fxw; /* left/right sides */
		v += psf->d[lx][iy] * lxw;
		for (ix = fx + 1; ix < lx ; ix++) {
			n++;
			v += psf->d[ix][iy]; /* middle pixels */
		}
	}
//	d4_printf("subsample: fx:%d fy:%d lx:%d ly:%d\n", fx, fy, lx, ly);
//	d4_printf("subsample: fxw:%.2f fyw:%.2f lxw:%.2f lyw:%.2f\n", fxw, fyw, lxw, lyw);
//	d4_printf("v is %.1f(%d)\n", v, n);
	return v;
}

float radial_weight(double dx, double dy, double r)
{
	float ds;

	if (dx < -r || dx > r || dy < -r || dy > r)
		return 0;

	ds = dx*dx + dy*dy;
	r = r*r;

	if (ds > r)
		return 0;
	if (ds < r / 2)
		return 1;
	return 2 * (r - ds) / r;
}

/* minimisation error function; compute the rms error of
   the model in the given region. The model parameters are as
   follows: p[1,2]  are the xy coordinates  and p[3] the amplitude
   of the first star, p[4,5] and p[6] the same for the second star,
   and so on. */
double multistar_funk(double p[], void *rmodel)
{
	int i, ix, iy;
	double err = 0;
	struct rmodel *mo = rmodel;

	for (i = 0; i < mo->n; i++) {
		d4_printf("star %d %.2f %.2f, %.2f ", i+1, p[3*i+1], p[3*i+2], p[3*i+3]);
	}

	for (ix = 0; ix < mo->patch->w; ix++) {
		for (iy = 0; iy < mo->patch->h; iy++) {
			float mv, w;
			mv = 0.0;
			for (i = 0; i < mo->n; i++) {
				int si;
				si = 3 * i + 1;
				w = radial_weight(1.0 * ix - p[si],
						  1.0 * iy - p[si+1], 2*mo->r);
//				w = 1.0;
				if (w > 0) {
					w *= psf_subsample_pixel(mo->psf, mo->ovsample,
								 1.0 * ix - p[si],
								 1.0 * iy - p[si+1]);
					mv += w * p[si+2];
				}
//				mo->patch->d[ix][iy] = mv;
			}
			err += sqr(mv + mo->sky - mo->patch->d[ix][iy]);
		}
	}
	d4_printf(":err:%.1f\n", err);
	return err;
}

/* subtract fitted stars */
void multistar_subtract(double p[], void *rmodel)
{
	int i, ix, iy;
	struct rmodel *mo = rmodel;

	for (ix = 0; ix < mo->patch->w; ix++) {
		for (iy = 0; iy < mo->patch->h; iy++) {
			float mv, w;
			mv = 0.0;
			for (i = 0; i < mo->n; i++) {
				int si;
				si = 3 * i + 1;
				w = psf_subsample_pixel(mo->psf, mo->ovsample,
							 1.0 * ix - p[si],
							 1.0 * iy - p[si+1]);
				mv += w * p[si+2];
			}
			mo->patch->d[ix][iy] -= mv;
		}
	}
}




/* ********************************************************************** */
/* psf fitting top level */

void do_fit_psf(gpointer window, GSList *found)
{
	struct psf *psf, *patch;
	struct gui_star *gs;
	struct image_channel *i_ch;
	struct stats stats;
	float sky;// flux;
	struct ap_params apdef;
	struct rmodel rmodel;
	double **p;
	double *y;
	int i, np;
	int iter;
//	int ret;
//	double psfvol;


	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL) {
		err_printf_sb2(window, "No image\n");
		error_beep();
		return;
	}

	if (found == NULL)
		return;
	gs = GUI_STAR(found->data);

	ap_params_from_par(&apdef);
	sky = get_sky(i_ch->fr, gs->x, gs->y, &apdef, NULL, &stats, NULL, NULL);
//	flux = get_star(i_ch->fr, gs->x, gs->y, &apdef, &stats, NULL);

	patch = psf_new(30, 30);
	extract_patch(i_ch->fr, patch, gs->x, gs->y);
	plot_psf(patch);

	psf = psf_new(100, 100);
//	psfvol = moffat_psf(psf, 8, 1.5, 4);
//	psfvol = gaussian_psf(psf, 8, P_DBL(SYNTH_FWHM) / FWHMSIG);

	rmodel.psf = psf;
	rmodel.patch = patch;
	rmodel.n = 1;
	rmodel.r = P_DBL(SYNTH_FWHM) * 3;
	rmodel.ovsample = 8;
	rmodel.sky = sky;



	np = rmodel.n * 3;

	p = (double **) malloc((np + 2) * sizeof(double *));
	y = (double *) malloc((np + 2) * sizeof(double));

	for (i = 1; i <= np + 1; i++) {
		p[i] = (double *) malloc((np + 1) * sizeof(double));
	}

	p[1][1] = patch->cx;
	p[1][2] = patch->cy;
	p[1][3] = (stats.max - sky) / sqr(rmodel.ovsample);

	p[2][1] = patch->cx + 1;
	p[2][2] = patch->cy;
	p[2][3] = p[1][3];

	p[3][1] = patch->cx;
	p[3][2] = patch->cy + 1;
	p[3][3] = p[1][3];

	p[4][1] = patch->cx;
	p[4][2] = patch->cy;
	p[4][3] = 1.5 * p[1][3];

	for (i = 1; i <= np+1; i++) {
		y[i] = multistar_funk(p[i], &rmodel);
		d4_printf("y[%d] = %.2f\n", i, y[i]);
	}

	//ret = amoeba(p, y, np, 0.01, multistar_funk, &iter, &rmodel);
	amoeba(p, y, np, 0.01, multistar_funk, &iter, &rmodel);

	d1_printf("%d iterations, fitted to %.2f %.2f %.1f\n", iter,
		    p[1][1]-patch->cx+floor(gs->x + 0.5),
		    p[1][2]-patch->cy+floor(gs->y + 0.5),
		    p[1][3]);

	multistar_subtract(p[1], &rmodel);

	plot_psf(patch);


	psf_release(patch);
	psf_release(psf);

}



/* *********************************************************************** */
/* gui/plotting stuff below */


int do_plot_profile(struct ccd_frame *fr, GSList *selection)
{
	GSList *sl, *vsl;
	struct gui_star *gs;
	int n, i, j;
	FILE *dfp = NULL;
	GSList *gr = NULL;
	double *grc = NULL;
	struct rp_point *rpp = NULL;
	int nrp;
	GSList *vs = NULL;
	char name[256];
	char preamble[16384];
	int np=0, nn = 0, pop;
	double peak, sky, flux;

	g_return_val_if_fail(fr != NULL, -1);

	n =  2 * GROWTH_POINTS * P_DBL(AP_R1) / growth_radius(GROWTH_POINTS) + 0.5;
	if (n > GROWTH_POINTS)
		n = GROWTH_POINTS;

	nrp = sqr(4 * ceil(P_DBL(AP_R1)));

	np += snprintf(preamble+np, 16384-np, "set key below\n");
	np += snprintf(preamble+np, 16384-np, "set y2tics autofreq\n");
	np += snprintf(preamble+np, 16384-np, "set y2label 'encircled flux'\n");
	np += snprintf(preamble+np, 16384-np, "set ylabel 'radial profile'\n");
	np += snprintf(preamble+np, 16384-np, "set grid xtics\n");
	np += snprintf(preamble+np, 16384-np, "plot [0:%.1f] 0 notitle lt 0, 1 axes x1y2 notitle lt 0, ",
		       2 * ceil(P_DBL(AP_R1)) );
	i = 0;

	for (sl = selection; sl != NULL; sl = sl->next) {
		double A, s;
		gs = GUI_STAR(sl->data);
		if (grc == NULL) {
			grc = malloc(GROWTH_POINTS * sizeof(double));
		}
		if (rpp == NULL) {
			rpp = malloc(nrp * sizeof(struct rp_point));
		}
		if (growth_curve(fr, gs->x, gs->y, grc, n))
			continue;
		if (radial_profile(fr, gs->x, gs->y, 2 * P_DBL(AP_R1), rpp, nrp,
				   &peak, &flux, &sky, NULL) <= 0)
			continue;
		A = P_DBL(AP_R1) / FWHMSIG / 2; s = 1.0;
		fit_1d_profile(rpp, &A, NULL, &s, NULL);
		if (gs->s)
			nn = snprintf(name, 255, "%s ",
				      CAT_STAR(gs->s)->name);
		else
			nn = snprintf(name, 255, "x%.0fy%.0f ", gs->x, gs->y);
		snprintf(name+nn, 255-nn, "(FWHM:%.1f peak:%.0f sky:%.1f)",
			 FWHMSIG * s, peak, sky);
		for (j=0; rpp[j].r >= 0; j++) {
			rpp[j].v /= A;
		}

		gr = g_slist_append(gr, grc);
		vs = g_slist_append(vs, rpp);
		grc = NULL;
		rpp = NULL;
		if (i > 0)
			np += snprintf(preamble+np, 16384-np,", ");
		np += snprintf(preamble+np, 16384-np,"'-' title '%s' lt %d, "
			"exp(-0.5*x*x/%.2f) notitle lt %d, "
			"'-' axes x1y2 notitle with lines lt %d",
			name, i+1, s*s, i+1, i+1);
		i++;
	}
	if (g_slist_length(gr) <= 0) {
		if (grc)
			free(grc);
		if (rpp)
			free(rpp);
		err_printf("Bad star (too close to edge?)\n");
		return -1;
	}

	pop = open_plot(&dfp, NULL);
	if (pop >= 0) {
		fprintf(dfp, "%s\n", preamble);

		for (sl = gr, vsl = vs; sl != NULL && vsl != NULL;
		     sl = sl->next, vsl = vsl->next) {
			struct rp_point *rp;
			rp = vsl->data;
			for(i = 0; rp[i].r >= 0; i++)
				fprintf(dfp, "%.2f %.3f\n", rp[i].r, rp[i].v);
			fprintf(dfp, "e\n");
			for(i=0; i<n; i++)
				fprintf(dfp, "%.2f %.3f\n", growth_radius(i),
					((double *)sl->data)[i]);
			fprintf(dfp, "e\n");
		}
		close_plot(dfp, pop);
	}

	if (grc)
		free(grc);
	if (rpp)
		free(rpp);
	for (sl = gr; sl != NULL; sl = sl->next) {
		free(sl->data);
	}
	for (sl = vs; sl != NULL; sl = sl->next) {
		free(sl->data);
	}
	g_slist_free(gr);
	g_slist_free(vs);
	return 0;
}


void print_star_measures(gpointer window, GSList *found)
{
	struct gui_star *gs;
	struct rp_point *rpp = NULL;
	int nrp;
	char name[256];
	double peak, sky, flux, err;
	struct image_channel *i_ch;
	double A, s;

	g_return_if_fail(window != NULL);

	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL) {
		err_printf_sb2(window, "No image\n");
		error_beep();
		return;
	}

	if (found == NULL)
		return;
	gs = GUI_STAR(found->data);

	nrp = sqr(4 * ceil(P_DBL(AP_R1)));
	rpp = malloc(nrp * sizeof(struct rp_point));

	if (radial_profile(i_ch->fr, gs->x, gs->y, 2 * P_DBL(AP_R1), rpp, nrp,
			   &peak, &flux, &sky, &err) <= 0) {
		err_printf_sb2(window, "Bad star (too close to edge?)\n");
		error_beep();
		return;
	}
	A = P_DBL(AP_R1) / FWHMSIG / 2; s = 1.0;
	fit_1d_profile(rpp, &A, NULL, &s, NULL);
	if (gs->s)
		snprintf(name, 255, "%s %.1f,%.1f",
			      CAT_STAR(gs->s)->name, gs->x, gs->y);
	else
		snprintf(name, 255, "%.1f,%.1f", gs->x, gs->y);

	info_printf_sb2(window, "%s  FWHM:%.1f peak:%.0f sky:%.1f flux:%.0f"
			" mag:%.03f/%.2g SNR:%.1f",
			name, FWHMSIG * s, peak, sky,
			flux, flux_to_absmag(flux), err, 1.08/err);
	free(rpp);

}


void plot_psf(struct psf *psf)
{
	FILE *dfp;
	int x, y;
	int pop;

	pop = open_plot(&dfp, NULL);
	if (pop >= 0) {
//		fprintf(dfp, "set nosurface \n");
//		fprintf(dfp, "set view 0,90,1,1 \n");

		fprintf(dfp, "set nokey\n");
		fprintf(dfp, "set hidden3d\n");
		fprintf(dfp, "set contour base\n");
		fprintf(dfp, "set cntrparam levels 20\n");
		fprintf(dfp, "splot '-' with lines\n");
		for(x = 0; x < psf->w; x++) {
			for(y = 0; y < psf->w; y++) {
				fprintf(dfp, "%.2f %.2f %.2f\n",
					1.0 * (x - psf->cx - psf->dx),
					1.0 * (y - psf->cy - psf->dx),
					psf->d[x][y] > 0 ? psf->d[x][y] : -1);
			}
		fprintf(dfp, "\n");
		}
		fprintf(dfp, "e\n");
 		close_plot(dfp, pop);
	}
}

void plot_sky_aperture(gpointer window, GSList *found)
{
	struct psf *psf;
	struct gui_star *gs;
	struct image_channel *i_ch;
//	struct stats rs;
//	double sky, err, allp;


	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL) {
		err_printf_sb2(window, "No image\n");
		error_beep();
		return;
	}

	if (found == NULL)
		return;
	gs = GUI_STAR(found->data);

//	sky = get_sky(i_ch->fr, gs->x, gs->y, &err, &rs, &psf, &allp);
//	d3_printf("sky:%.1f err:%.3f %.0f/%0.f pixels\n", sky, err, rs.all, allp);
//	aperture_multiply(i_ch->fr, psf, gs->x, gs->y);

	psf = psf_new((int) (3 + 2 * ceil(P_DBL(AP_R1))), (int) (3 + 2 * ceil(P_DBL(AP_R1))));
	psf->dx = gs->x - floor(gs->x + 0.5);
	psf->dy = gs->y - floor(gs->y + 0.5);
	make_circular_aperture(psf, P_DBL(AP_R1));

	aperture_multiply(i_ch->fr, psf, gs->x, gs->y);

	plot_psf(psf);
	psf_release(psf);

}

void plot_sky_histogram(gpointer window, GSList *found)
{
	struct gui_star *gs;
	struct rp_point *rpp = NULL;
	int nrp;
	FILE *dfp;
	int pop, i;
	double peak, sky, flux, err;
	struct image_channel *i_ch;
	struct rstats *rs;

	g_return_if_fail(window != NULL);

	i_ch = g_object_get_data(G_OBJECT(window), "i_channel");
	if (i_ch == NULL || i_ch->fr == NULL) {
		err_printf_sb2(window, "No image\n");
		error_beep();
		return;
	}

	if (found == NULL)
		return;
	gs = GUI_STAR(found->data);

	nrp = sqr(4 * ceil(P_DBL(AP_R1)));
	rpp = malloc(nrp * sizeof(struct rp_point));

	if (radial_profile(i_ch->fr, gs->x, gs->y, 2 * P_DBL(AP_R1), rpp, nrp,
			   &peak, &flux, &sky, &err) <= 0) {
		err_printf_sb2(window, "Bad star (too close to edge?)\n");
		error_beep();
		return;
	}
	rs = malloc(sizeof(struct rstats));
	ring_stats(i_ch->fr, gs->x, gs->y,
		      P_DBL(AP_R2), P_DBL(AP_R3), ALLQUADS, rs,
		      -HUGE, HUGE);

	pop = open_plot(&dfp, NULL);
	if (pop >= 0) {
		int s, e;
		int w, m=0, all=0, iy=0;
		double is, n;


		for (i = 0; i < H_SIZE; i++) {
			all += rs->h[i];
		}
		for (i = 0; i < H_SIZE; i++) { /* get the median */
			iy += rs->h[i];
			if (iy >= all / 2) {
				m = i + H_START;
				break;
			}
		}

		w = rs->sigma * 6;
		if (w > 100)
			w = 100;
		fprintf(dfp, "set key below\n");

		fprintf(dfp, "plot '-' title 'sky:%.0f min:%.0f max:%.0f mean:%.0f "
			"median:%.0f sigma:%.0f' with boxes\n",
			sky, rs->min, rs->max, rs->avg, 1.0*m, rs->sigma);

		s = floor(sky - w) - H_START;
		if (s < 0)
			s = 0;
		e = s + 2 * w;
		if (e >= H_SIZE)
			e = H_SIZE - 1;
		is = 0; n = 0;
		for (i = s; i < e; i++) {
			fprintf(dfp, "%d %d\n", i + H_START, rs->h[i]);
			is += rs->h[i] * (i + H_START);
			n += rs->h[i];
		}
		d3_printf("centroid at %.1f\n", is / n);
		fprintf(dfp, "e\n");
		close_plot(dfp, pop);
	}
	free(rpp);
	free(rs);
}

