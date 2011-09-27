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

// aphot.c: aperture photometry routines
// $Revision: 1.20 $
// $Date: 2009/06/20 20:02:34 $

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "ccd.h"
//#include "x11ops.h"


/* compute the average and median of the k-s clipped histogram */
double hist_clip_avg(struct rstats *rs, double *median, double sigmas,
		     double *lclip, double *hclip)
{
	int ix, it=0;
	int iy = 0;
	int all = 0, n;
	double m=0, so, s2, sum;
	int s=0, e=H_SIZE;


	for (ix = 0; ix < H_SIZE; ix++) {
		all += rs->h[ix];
	}
	for (ix = 0; ix < H_SIZE; ix++) { /* get the median */
		iy += rs->h[ix];
		if (iy >= all / 2) {
			m = ix + H_START;
			break;
		}
	}
	s2 = 0;
	do {
		so = s2;
		sum = s2 = 0;
		n = 0;
		for (ix = s; ix < e; ix++) {
			sum += (ix + H_START) * rs->h[ix];
			s2 += sqr(ix + H_START - m) * rs->h[ix] ;
			n += rs->h[ix];
		}
		if (n == 0)
			goto nullhist;
//		if (it > 0)
//			m = sum / n;
		s2 = sqrt(s2 / n);
		s = m - sigmas * s2 - H_START;
		e = m + sigmas * s2 - H_START;
		if (s < 0)
			s = 0;
		if (e > H_SIZE)
			e = H_SIZE;
		d4_printf("hist clip: [%d %d] (m=%.1f s=%.1f)\n", s+H_START, e+H_START,
			  m, s2);
		it ++;

	} while (it < 10 && (it == 1 || fabs(so - s2) > 0.00001) && (s != e));

	if (s == e)
		m = s + H_START;
	if (median) {
		if (s == e)
			*median = s + H_START;
		iy = 0;
		for (ix = s; ix < e; ix++) { /* get the median */
			iy += rs->h[ix];
			if (iy >= n / 2) {
				*median = ix + H_START;
				break;
			}
		}
	}
	if (lclip)
		*lclip = s + H_START;
	if (hclip)
		*hclip = e + H_START;
	return sum / n;

nullhist:
	d4_printf("NULL (s:%d e:%d)\n", s, e);
	if (median)
		*median = m;
	if (lclip)
		*lclip = s + H_START;
	if (hclip)
		*hclip = e + H_START;
	return m;
}



// compute ring statistics between r1 and r2; ring has the origin at x, y;
// values lower than min or larger than max are skipped
// quads contains a bitfield enabling the measurement of quadrants; bit 0 is Q0 (dx>=0,dy>=0),
// bit 1 is Q1 (dx<0, dy>=0) etc.
int ring_stats(struct ccd_frame *fr, double x, double y,
	       double r1, double r2, int quads, struct rstats *rs,
	       double min, double max)
{
	int ix, iy;
	int xs, ys;
	int xe, ye;
	int xc, yc;
	int nring = 0, nskipped = 0;
	float v;
	double sum, sumsq, r;
//	d3_printf("x:%f y:%f r1:%f r2:%f\n", x, y, r1, r2);

// round the coords off to make sure we use the same number of pixels for the
// flux measurement

	xc = (int)floor(x+0.5);
	yc = (int)floor(y+0.5);

	xs = (int)(xc-r2);
	xe = (int)(xc+r2+1);
	ys = (int)(yc-r2);
	ye = (int)(yc+r2+1);

	if (xs < 0)
		xs = 0;
	if (ys < 0)
		ys = 0;
	if (xe >= fr->w)
		xe = fr->w - 1;
	if (ye >= fr->h)
		ye = fr->h - 1;

//	d3_printf("xc:%d yc:%d r1:%f r2:%f\n", xc, yc, r1, r2);

//	d3_printf("xs:%d xe:%d ys:%d ye:%d w:%d\n", xs, xe, ys, ye, w);

//	d3_printf("enter ringstats\n");

	for (ix = 0; ix < H_SIZE; ix++)
		(rs->h[ix]) = 0;

//	d3_printf("init histogram\n");

// walk the ring and collect statistics
	sum=0.0;
	sumsq = 0.0;
	rs->min = HUGE;
	rs->max = -HUGE;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			r = sqr(ix - xc + 0.0) + sqr(iy - yc + 0.0);
			if (r < sqr(r1) || r >= sqr(r2))
				continue;

			if (((quads & QUAD1) == 0) && ix-xc >= 0 && iy-yc >= 0)
				continue;
			if (((quads & QUAD2) == 0) && ix-xc < 0 && iy-yc >= 0)
				continue;
			if (((quads & QUAD3) == 0) && ix-xc < 0 && iy-yc > 0)
				continue;
			if (((quads & QUAD4) == 0) && ix-xc >= 0 && iy-yc < 0)
				continue;

			nring ++;
			v = get_pixel_luminence(fr, ix, iy);
			if (v < H_START || v >= H_START+H_SIZE) {
				nskipped ++;
				continue;
			}
			if (v < min || v > max) {
				nskipped ++;
				continue;
			}
			rs->h[(int)floor(v) - H_START] ++;
			sum += v;
			sumsq += v*v;
			if (v < rs->min)
				rs->min = v;
			if (v > rs->max)
				rs->max = v;
		}
	}

	if (nring - nskipped <= 0)
		return -1;

// fill the result structure with the stats
	rs->all = nring;
	rs->used = nring - nskipped;
	rs->sum = sum;
	rs->sumsq = sumsq;
	rs->avg = sum / rs->used;
	rs->sigma = sqrt(sumsq / rs->used - sqr(sum / rs->used));
// walk the histogram and compute median
	iy = 0;
	for (ix = 0; ix < H_SIZE; ix++) {
		iy += rs->h[ix];
		if (iy >= rs->used / 2)
			break;
	}
//	d3_printf("ix = %d iy = %d\n", ix, iy);
	rs->median = ix + H_START;
	d3_printf("ringstats avg:%.2f sum:%.2f sigma:%.2f, min:%.2f max:%.2f\n",
		  rs->avg, rs->sum, rs->sigma, rs->min, rs->max);
	return 0;
}

// get the sky value and error; we assume that the star position
// has been checked against the edges
static int ap_get_sky(struct ccd_frame *fr, struct star *s,
	       struct ap_params *p, struct bad_pix_map *bp)
{
	struct rstats rs;
	int ret;
	double phn, flnsq;
	double mean, median;

// get a first round of ring stats
	ret = ring_stats(fr, s->x, s->y, p->r2, p->r3, p->quads, &rs, -HUGE, HUGE);
	if (ret) {
		return ret;
	}

	switch(p->sky_method) {
	case APMET_AVG:
		s->aph.sky = rs.avg;
		break;
	case APMET_MEDIAN:
		s->aph.sky = rs.median;
		break;
	case APMET_KS:
		s->aph.sky = hist_clip_avg(&rs, NULL, p->sigmas, NULL, NULL);
		break;
	case APMET_SYMODE:
		d4_printf("SYMODE--");
		mean = hist_clip_avg(&rs, &median, p->sigmas, NULL, NULL);
		if (mean > median)
			s->aph.sky = 3 * median - 2 * mean;
		else
			s->aph.sky = (median + mean) / 2;
		d4_printf("med %.3f, mean %.3f\n", median, mean);
		break;
	default:
		err_printf("ap_get_sky: unknown method %d\n", p->sky_method);
//		free(rs.h);
		return ERR_FATAL;
	}

// copy stats to the star structure
	s->aph.sky_avg = rs.avg;
	s->aph.sky_min = rs.min;
	s->aph.sky_max = rs.max;
	s->aph.sky_med = rs.median;
	s->aph.sky_sigma = rs.sigma;
	s->aph.sky_all = rs.all;
	s->aph.sky_used = rs.used;

// compute background error
	phn = sqrt(fabs(s->aph.sky - fr->exp.bias) * fr->exp.scale) / fr->exp.scale; // sky+dark photon shot noise / pixel
	flnsq = sqr(fr->exp.flat_noise) * rs.sumsq;
	s->aph.sky_err = sqrt((sqr(fr->exp.rdnoise) + sqr(phn) + flnsq) / s->aph.sky_used);
	return 0;
}

/* calculate total flux inside an aperture of radius r centered on x, y;
 * update min, max, npix */
static double aperture_flux(struct ccd_frame *fr, double r, double x, double y,
			    double *minv, double *maxv, double *npix, double *ssq)
{
	int ix, iy;
	int xs, ys;
	int xe, ye;
	int xc, yc;
	int w;
	float v;
	double sum, sumsq;
	double min, max;
	double n, d, we;
//	d3_printf("x:%f y:%f r1:%f r2:%f\n", x, y, r1, r2);

// round the coords off to make sure we use the same number of pixels for the
// flux measurement

	xc = (int)floor(x+0.5);
	yc = (int)floor(y+0.5);

	xs = (int)(xc-r);
	xe = (int)(xc+r+1);
	ys = (int)(yc-r);
	ye = (int)(yc+r+1);
	w = fr->w;

	if (xs < 0)
		xs = 0;
	if (ys < 0)
		ys = 0;
	if (xe >= w)
		xe = w - 1;
	if (ye >= fr->h)
		ye = fr->h - 1;

	min = HUGE;
	max = -HUGE;
	sum = 0.0;
	sumsq = 0.0;
	n = 0.0;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			v = get_pixel_luminence(fr, ix, iy);
			d = sqrt(sqr(1.0 * ix - x) + sqr(1.0 * iy - y));
			if (d > r + 0.5)
				we = 0.0;
			else if (d < r - 0.5)
				we = 1.0;
			else
				we = r + 0.5 - d;
			sum += v * we;
			sumsq += sqr(v * we);
			if (v < min && we > 0.5)
				min = v;
			if (v > max && we > 0.5)
				max = v;
			n += we;
		}
	}
	if (minv)
		*minv = min;
	if (maxv)
		*maxv = max;
	if (npix)
		*npix = n;
//	d3_printf("used %.4g pixels\n", n);
	if (ssq)
		*ssq = sumsq;
	return sum;
}

/* calculate the centroid inside the aperture, using only pixels above
   threshold; return total flux above threshold */
static double aperture_centroid(struct ccd_frame *fr, double r, double x, double y,
				double threshold, double *cx, double *cy,
				double *cxerr, double *cyerr)
{
	int ix, iy;
	int xs, ys;
	int xe, ye;
	int xc, yc;
	int w;
	float v;
	double sum;
	double sx, sy, xsq, ysq, xw, yw;
	double n, d, we;

	xc = (int)floor(x+0.5);
	yc = (int)floor(y+0.5);

	xs = (int)(xc-r);
	xe = (int)(xc+r+1);
	ys = (int)(yc-r);
	ye = (int)(yc+r+1);
	w = fr->w;

	if (xs < 0)
		xs = 0;
	if (ys < 0)
		ys = 0;
	if (xe >= w)
		xe = w - 1;
	if (ye >= fr->h)
		ye = fr->h - 1;

	sum = 0.0;
	sx = 0.0; sy = 0.0;
	xsq = 0.0; ysq = 0.0;
	n = 0.0;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			v = get_pixel_luminence(fr, ix, iy);
			d = sqrt(sqr(1.0 * ix - x) + sqr(1.0 * iy - y));
			if (d > r + 0.5)
				we = 0.0;
			else if (d < r - 0.5)
				we = 1.0;
			else
				we = r + 0.5 - d;
			if (v > threshold) {
				sum += (v - threshold) * we;
				sx += ix * (v - threshold) * we;
				sy += iy * (v - threshold) * we;
				xsq += sqr(ix) * (v - threshold) * we;
				ysq += sqr(iy) * (v - threshold) * we;
				n += we;
			}
		}
	}
	if (n == 0 || sum == 0) {
		if (cx)
			*cx = x;
		if (cy)
			*cy = y;
		if (cxerr)
			*cxerr = BIG_ERR;
		if (cyerr)
			*cyerr = BIG_ERR;
	} else {
		if (cx)
			*cx = sx / sum;
		if (cy)
			*cy = sy / sum;
		xw = sqrt(xsq / sum - sqr(sx/sum));
		yw = sqrt(ysq / sum - sqr(sy/sum));
		if (xw < 2)
			xw = 2;
		if (yw < 2)
			yw = 2;
		if (cxerr)
			*cxerr = xw / sqrt(sum);
		if (cyerr)
			*cyerr = yw / sqrt(sum);
	}
	d3_printf("ap_centroid %.2f %.2f [%.3f %.3f] errors: %.3f %.3f\n",
		  *cx, *cy, x - *cx, y - *cy, *cyerr, *cyerr);
	return sum;
}



// get the star flux
static int ap_get_star(struct ccd_frame *fr, struct star *s,
	       struct ap_params *p, struct bad_pix_map *bp)
{
	double phn, flnsq, fsq;


	s->aph.tflux = aperture_flux(fr, p->r1, s->x, s->y,
				     &s->aph.star_min, &s->aph.star_max,
				     &s->aph.star_all, &fsq);

	phn = sqrt(fabs(s->aph.tflux - fr->exp.bias * s->aph.star_all)
		   * fr->exp.scale) / fr->exp.scale; // total photon shot noise
	flnsq = sqr(fr->exp.flat_noise) * (fsq);
	s->aph.flux_err = sqrt((sqr(fr->exp.rdnoise) * s->aph.star_all + sqr(phn) + flnsq));
	s->aph.pshot_noise = phn;
	s->aph.rd_noise = fr->exp.rdnoise * sqrt(s->aph.star_all);
	if (s->aph.star_max > p->sat_limit)
		s->aph.flags |= AP_BURNOUT;
	else if (s->aph.star_max > p->sat_limit / 2)
		s->aph.flags |= AP_BRIGHT;
//	d3_printf("flux %.4g pixels %.3f pshotn %.4f fluxerr %.4f flat %.4f\n", s->aph.tflux,
//		  s->aph.star_all, phn, s->aph.flux_err, sqrt(flnsq));
	return 0;

}


// do aperture photometry for star s; star image coordinates are taken from s. The function
// fills up the results in s. If an error (!= 0) is returned, the values in s are undefined.
// the bad pix map is used for flagging possible data errors due to the presence of bad pixels
int aperture_photometry(struct ccd_frame *fr, struct star *s,
			struct ap_params *p, struct bad_pix_map *bp)
{
	double rm;
	int ret, i;
	double cx, cy, cxerr=0.0, cyerr=0.0;
//	double skn, rdn;

	rm = ceil(p->r3) + 1;

//	d3_printf("rm = %f\n", rm);
	if ((s->x - rm) <= 0 || (s->x + rm) >= fr->w ||
	    (s->y - rm) <= 0 || (s->y + rm) >= fr->h) {
		d1_printf("aperture_photometry: star is too close to the frame edge\n");
		return ERR_FATAL;
	}

// get the sky
	ret = ap_get_sky(fr, s, p, bp);
	if (ret)
		return ret;

	if (p->center) {
		for (i=0; i< 10; i++) {
			rm = aperture_centroid(fr, p->r1, s->x, s->y,
					       s->aph.sky + 2 * fr->stats.csigma, &cx, &cy,
					       &cxerr, &cyerr);
			if (fabs(cx - s->x) < 0.0001 && fabs(cx - s->x) < 0.0001)
				break;
			if (cxerr < 0.1)
				s->x = cx;
			if (cyerr < 0.1)
				s->y = cy;
		}
		s->xerr = cxerr;
		s->yerr = cyerr;
	}

/* get the sky in the new position */
	ret = ap_get_sky(fr, s, p, bp);
	if (ret)
		return ret;

// now get the data for the star
	ret = ap_get_star(fr, s, p, bp);
	if (ret)
		return ret;


	d4_printf("r1=%.1f, star+sky=%.5f sky=%.5f, flux=%.5f\n",
		  p->r1,
		  s->aph.tflux, s->aph.star_all * s->aph.sky,
		  s->aph.tflux - s->aph.star_all * s->aph.sky);


// compute star flux and error
	s->aph.star = s->aph.tflux - s->aph.star_all * s->aph.sky;
	s->aph.star_err = sqrt(sqr(s->aph.flux_err) + sqr(s->aph.star_all * s->aph.sky_err));


	if (cxerr < BIG_ERR && rm > 1)
		s->xerr = cxerr * sqrt(rm) * s->aph.star_err / s->aph.star;
	if (cyerr < BIG_ERR && rm > 1)
		s->yerr = cyerr * sqrt(rm) * s->aph.star_err / s->aph.star;


	if (s->aph.star < 3 * s->aph.star_err)
		s->aph.flags |= AP_FAINT;
// clip to protect the logarithms
	if (s->aph.star < MIN_STAR)
		s->aph.star = MIN_STAR;
	if (s->aph.star_err < MIN_STAR)
		s->aph.star_err = MIN_STAR;

	s->aph.absmag = flux_to_absmag(s->aph.star);

// compute limiting magnitude as the magnitude at which the SNR = 1
//	skn = sqr(s->aph.star_all * s->aph.sky_err);
//	rdn = s->aph.star_all * sqr(fr->exp.rdnoise);
//	d3_printf("mag: %.4g; star error before log: %.3f\n", s->aph.star,
//		  s->aph.star_err / s->aph.star);
// temporary magerr that doesn't include scintillation
	s->aph.magerr = fabs(flux_to_absmag(s->aph.star + s->aph.star_err)
		- flux_to_absmag(s->aph.star));
	s->aph.flags |= AP_MEASURED;
	return 0;
}

// compute absolute magnitude from flux
double flux_to_absmag(double flux)
{
	return -2.5*log10(flux);
}

// compute flux from absolute magnitude
double absmag_to_flux(double mag)
{
	return pow(10, -mag / 2.5);
}


/*
static void print_star_results(struct ph_star *s)
{
	info_printf("imag:%8.3f err:%8.3f\n", s->absmag, s->magerr);
	info_printf("Sky pixels:%f used:%f value:%.2f error:%.2f\n",
		    s->sky_all, s->sky_used, s->sky, s->sky_err);
	info_printf("Star pixels:%f tflux:%.2f flux:%.2f err:%.2f mag:%.2f\n",
		    s->star_all, s->tflux, s->star, s->star_err, s->absmag);
	info_printf("Star min:%.2f max:%.2f flags:%x\n",
		    s->star_min, s->star_max, s->flags);
}
*/

// get scintillation noise parameters
void get_scint_pars(struct ccd_frame *fr, struct vs_recipy *vs, double *t, double *d, double *am)
{

	FITS_row * cp;
	float r;

// determine exposure time

	if (fits_get_double(fr, "EXPTIME", t) <= 0 ) {
		err_printf("bad exposure time, assuming 1 sec\n");
		*t = 1.0;
	}
// telescope aperture
	cp = fits_keyword_lookup(fr, "APERT");
	if (vs->aperture > 1.0 && vs->aperture < 1000) { // look in recipy
		*d = vs->aperture;
	} else if (cp != NULL && (sscanf((char *)cp, "%*s = %f", &r) )) {
		*d = r;
	} else {
		err_printf("bad telescope aperture, assuming 30 cm\n");
		*d = 30.0;
	}
// airmass
	cp = fits_keyword_lookup(fr, "OBJCTALT");
	if (cp != NULL && (sscanf(((char *)cp), "%*s= %f", &r) )) {
		*am = airmass(r);
		return;
	}
	cp = fits_keyword_lookup(fr, "AIRMASS");
	if (cp != NULL && (sscanf(((char *)cp), "%*s = %f", &r) )) {
		*am = r;
		return;
	}
	if (vs->airmass >= 1.0 && vs->airmass < 50.0) { // we get it from here
		*am = vs->airmass;
		return;
	}
	err_printf("could not determine airmass, assuming 1.5\n");
	*am = 1.5;
}

double scintillation(double t, double d, double am)
{
	double sn;
	if (t <= 0.00001 || d <=0.00001 || am < 1.0) {
		err_printf("bad scintillation pars: t %.1f d %.1f am %.1f\n", t, d, am);
	} else {
		d3_printf("scintillation pars: t %.1f d %.1f am %.1f\n", t, d, am);
	}
	sn = log(0.09) - 0.66666 * log(d) + 1.75 * log(am) - 0.5 * log(2 * t);
	sn = exp(sn);
	d3_printf("calculated scint is %.3f\n", sn);
	return sn;
}

// compute 1-sigma scintillation noise
double scint_noise(struct ccd_frame *fr, struct vs_recipy *vs)
{
	double t, d, am;

	get_scint_pars(fr, vs, &t, &d, &am);
	return vs->scint_factor * scintillation(t, d, am);
}


// max shift of a star from the std position
#define MAX_SHIFT 3
// measure the stars in a recipy; return 0 if no error occured
int measure_stars(struct ccd_frame *fr, struct vs_recipy *vs)
{
	int i;
	struct star *s;
	struct star st;
	int ret;
	double snsq;

	ret = 0;

	snsq = scint_noise(fr, vs);
	snsq = sqr(snsq);

	for(i = 0; i < vs->cnt; i++) {
		s = &(vs->s[i]);
		if (vs->usewcs) {
			xypix(s->ra, s->dec, fr->fim.xref, fr->fim.yref,

			      fr->fim.xrefpix, fr->fim.yrefpix, fr->fim.xinc,
			      fr->fim.yinc, fr->fim.rot,
			      "-TAN", &s->x, &s->y);
			info_printf("Star near %.1f %.1f ", s->x, s->y);
			get_star_near(fr, (int)s->x, (int)s->y, 0, &st);
			info_printf("found at %.1f %.1f ", st.x, st.y) ;
//			st.x = s->x;
//			st.y = s->y;
			if (fabs(s->x - st.x) > MAX_SHIFT || fabs(s->y - st.y) > MAX_SHIFT) {
				info_printf(" ... Cannot locate star, too faint?\n");
			} else {
				s->x = st.x;
				s->y = st.y;
			}
		}
		ret |= aperture_photometry(fr, s, &(vs->p), NULL);
//		print_star_results(s);
//		d3_printf("magerr before scint: %.4f\n", s->magerr);
		s->aph.magerr = sqrt(sqr(s->aph.magerr) + snsq);
//		info_printf("after scint err:%8.3f\n", s->magerr);
	}
	return ret;

}

// compute a photometry solution for the given recipy
void get_ph_solution(struct vs_recipy *vs)
{
	int i;
	double std_cal;
	double nstd;
	double weight;
	double std_cal_err;

	nstd = 0.0;
	std_cal = 0.0;
	std_cal_err = 0.0;

	for (i=0; i<vs->cnt; i++) {
		if (vs->s[i].aph.flags & AP_STD_STAR) {
			if (vs->s[i].aph.magerr > 0.0001) {
				weight = 1 / vs->s[i].aph.magerr;
			} else {
				weight = 1 / 0.0001;
			}
			nstd += weight;
			std_cal += weight * (vs->s[i].aph.stdmag - vs->s[i].aph.absmag);
			std_cal_err += sqr(weight * vs->s[i].aph.magerr);
		}
	}
	if (nstd == 0) {
		err_printf("No standard stars!\n");
		return;
	}
	std_cal = std_cal / nstd;
	std_cal_err = sqrt(std_cal_err) / nstd;
//	d3_printf("std_cal_err : %.4f\n", std_cal_err);
	vs->p.std_err = std_cal_err;
	for (i=0; i<vs->cnt; i++) {
		if ((vs->s[i].aph.flags & AP_STD_STAR) == 0) {
			vs->s[i].aph.stdmag = vs->s[i].aph.absmag + std_cal;
			vs->s[i].aph.magerr = sqrt(sqr(vs->s[i].aph.magerr) + sqr(std_cal_err));
		} else {
			vs->s[i].aph.residual = vs->s[i].aph.absmag
				+ std_cal - vs->s[i].aph.stdmag;
		}
	}
}


