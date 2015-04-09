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

// sources.c: find/track sources
// $Revision: 1.20 $
// $Date: 2009/07/14 00:45:53 $

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

// parameters for source extraction
#define	MAXSR		50		// max star radius
#define	MINSR		2		// min star radius
#define	SKYR		2		// measure sky @ starr
#define	MINSEP		3		// min star separation
#define	NWALK		8		// n pixels surrounding a pixel
#define	NCONN		4		// min connected pixels to qualify
#define	PKP		20.0		// dips must be > this % of peak-med
#define NSIGMA		3.0		// peak of star must be > NSIGMA * csigma + median to count
#define BURN		60000		// burnout limit for locate_star

#define RING_MAX 3000
#define MAX_STAR_CANDIDATES 65536	/* max number of candidates before we find the first star
					 */

// compute ring statistics of pixels in the r1-radius ring that has the center at x, y;
// values lower than min or larger than max are skipped
static int thin_ring_stats(struct ccd_frame *fr, int x, int y,
		int r2, struct rstats *rs, double min, double max)
{
	int ix, iy;
	int xs, ys;
	int xe, ye;
	int xc, yc;
	int w;
	int nring = 0, nskipped = 0;
	float v;
	double sum, sumsq, r;
	int dy2, rsq1, rsq2;
	double ring[RING_MAX];

// round the coords off to make sure we use the same number of pixels for the
// flux measurement


	xc = x;
	yc = y;


	xs = (xc-r2);
	xe = (xc+r2+1);
	ys = (yc-r2);
	ye = (yc+r2+1);
	w = fr->w;

	if (xs < 0)
		xs = 0;
	if (ys < 0)
		ys = 0;
	if (xe >= w)
		xe = w - 1;
	if (ye >= fr->h)
		ye = fr->h - 1;

// walk the ring and collect statistics
	sum=0.0;
	sumsq = 0.0;
	rs->min = HUGE;
	rs->max = -HUGE;
	rsq1 = sqr(r2);
	rsq2 = sqr(r2+1.001);
	for (iy = ys; iy < ye; iy++) {
		dy2 = (iy - yc) * (iy - yc);
		for (ix = xs; ix < xe; ix++) {
			if (((r = (ix - xc) * (ix - xc) + dy2))
			    < rsq1 || r > rsq2)
				continue;

			v = get_pixel_luminence(fr, ix, iy);
			if (v < min || v > max) {
				nskipped ++;
				continue;
			}
			if (nring < RING_MAX)
				ring[nring++] = v;
			sum += v;
			sumsq += v*v;
			if (v < rs->min)
				rs->min = v;
			if (v > rs->max) {
				rs->max = v;
				rs->max_x = ix;
				rs->max_y = iy;
			}
		}
	}

// fill the result structure with the stats

	rs->all = nring + nskipped;
	rs->used = nring;
	rs->sum = sum;
	rs->avg = sum / rs->used;
	rs->sigma = sqrt(sumsq / rs->used - sqr(sum / rs->used));
	rs->median = dmedian(ring, nring);
//	d3_printf("thinrs x:%d y:%d r:%d avg:%.2f sum:%.2f sigma%.2f, min%.2f max%.2f\n",
//		  x, y, r2,
//		  rs->avg, rs->sum, rs->sigma, rs->min, rs->max);
	return 0;
}

// compute moments of disk of radius r
// only pixels larger than sky are considered
static int star_moments(struct ccd_frame *fr, double x, double y,
		double r, double sky, struct moments *m)
{
	int ix, iy;
	int xs, ys;
	int xe, ye;
	int xc, yc;
	int w;
	int nring = 0, nskipped = 0;
	float v;
	double sum, rn;
	double mx, my, mxy, mx2, my2;

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

//	d3_printf("xc:%d yc:%d r1:%f r2:%f\n", xc, yc, r1, r2);

//	d3_printf("xs:%d xe:%d ys:%d ye:%d w:%d\n", xs, xe, ys, ye, w);

//	d3_printf("enter ringstats\n");

// walk the ring and collect statistics
	sum=0.0;
	mx = 0;
	my = 0;
	mx2 = 0;
	my2 = 0;
	mxy = 0;
	for (iy = ys; iy < ye; iy++) {
		for (ix = xs; ix < xe; ix++) {
			v = get_pixel_luminence(fr, ix, iy);
			if (((rn = sqr(ix - xc) + sqr(iy - yc)) > sqr(r)))
				continue;
			nring ++;
			if (v < sky) {
				nskipped ++;
				continue;
			}
			v -= sky;
/*			mx += v * (ix - x);
			my += v * (iy - y);
			mxy += v * (ix - x) * (iy - y);
			mx2 += v * sqr(ix - x);
			my2 += v * sqr(iy - y); */
			mx += v * (ix);
			my += v * (iy);
			mxy += v * (ix) * (iy);
			mx2 += v * sqr(ix);
			my2 += v * sqr(iy);
			sum += v;
		}
	}
// fill the result structure with the stats
	mx /= sum;
	my /= sum;
	mxy /= sum;
	mx2 /= sum;
	my2 /= sum;

	m->mx = mx;
	m->my = my;

/*
	m->mxy = mxy + (x - mx) * my + (y - my) * mx + (x - mx) * (y - my);
	m->mx2 = mx2 + sqr(x - mx) + mx * (x - mx);
	m->my2 = my2 + sqr(y - my) + my * (y - my);
*/
	m->mxy = mxy;// + (m->mx) * my + (m->my) * mx + (m->mx) * (m->my);
	m->mx2 = mx2;// + sqr(m->mx) + mx * (m->mx);
	m->my2 = my2;// + sqr(m->my) + my * (m->my);
	m->sum = sum;
	m->npix = nring;

	return 0;
}

// compute the eccentricity and orientation of major
// axis for a star, given it's moments
static int moments_to_ecc(struct moments *m, double *pa, double *ecc)
{
	double a;
	double tg2a, csqa, s2a;
	double mpy, mpx;

	if (m->mx2 + m->my2 == 0)	// 'null' star
		return -1;
	if (m->mx2 != m->my2) {
		tg2a = - 2 * m->mxy / (m->mx2 - m->my2);
		a = 0.5 * atan(tg2a);
	} else { //we have a round star, make angle = 0
		a = 0;
	}

	s2a = sin(2.0 * a);
	csqa = sqr(cos(a));

	mpx = csqa * m->mx2 + (1 - csqa) * m->my2 - s2a * m->mxy;
	mpy = csqa * m->my2 + (1 - csqa) * m->mx2 + s2a * m->mxy;

	if (mpx >= mpy) { // 'horisontal' major axis after rotation, angle is true
		*ecc = sqrt(mpx / mpy);
		*pa = a * 180.0 / PI;
	} else { // we got the major axis vertical, change pa by 90 degrees
		*ecc = sqrt(mpy / mpx);
		if (a > 0)
			*pa = a * 180.0 / PI - 90.0;
		else
			*pa = a * 180.0 / PI + 90.0;
	}
	return 0;
}

// search for the edge of the 'star' image; sky is an estimate of the
// background near the star.
// returns the star radius, or a negative value if found to be an improper
// star
static int star_radius(struct ccd_frame *fr, int x, int y, double peak, double sky, double *fwhm)
{
	double lmax, mindip;
	int rn;
	struct rstats *rsn;
	int starr = 0;
	double bhf, ahf;  // before/after half fluxes and radiuses
	int bhr = 0;
	double hm;

	rsn = malloc(sizeof(struct rstats));

	ahf = 0.0;

	d4_printf("(%d,%d) peak: %.1f, sky: %.1f     ", x, y, peak, sky);

	lmax = peak;
	mindip = PKP / 100.0 * (peak - sky);
	hm = (peak - sky) / 2 + sky;
	bhf = peak;
	for (rn = 1; rn < MAXSR; rn++) {
		thin_ring_stats(fr, x, y, rn, rsn, -HUGE, HUGE);
/*		if (rsn->max >= BURN) {
			d4_printf("burnt out\n");
			free(rsn);
			return -1;
			}*/
		if (rsn->max > peak) {// we are not at the peak
			d4_printf("NP: %.0f (%d,%d) \n", rsn->max, rsn->max_x, rsn->max_y);
			free(rsn);
			return -2;
		}

		if (rsn->avg > hm) {// we are before half maximum
//			d4_printf("Ring %d avg %.1f (peak %.1f hm %.1f)\n", rn, rsn->avg, peak, hm);
			bhf = rsn->avg;
			bhr = rn;
		} else if (ahf == 0.0) {// first radius after half maximum
			ahf = rsn->avg;
		}

// search for the star edge
		if (rsn->max > lmax && (peak - lmax) > mindip) {
			starr = rn;
			break;
		}
		lmax = rsn->max;
	}
// see if we found the edge before reaching MAXSR
	if (rn == MAXSR) {
		d4_printf("MAXSR reached\n");
		starr = rn;
//		return -3;
	}
	if (starr < MINSR) {
		d4_printf("Radius %d too small\n", starr);
		free(rsn);
		return -4;
	}
	d4_printf(" starr is %d\n", starr);

	*fwhm = (2.0 * bhr + 1.0) + (bhf - hm) / (bhf - ahf);
//	d4_printf("FWHM= %.1f + %.1f\n", (2.0 * bhr + 1.0), (bhf - hm) / (bhf - ahf));
	free(rsn);
	return starr;
}

// find the 'star' closest to the designated position;
// search a circular region from the center out, and stop
// when a star of the required flux is found.
// returns 0 if a star is found, negative if not.
// the star's centroided position, estimated flux and size are updated in
// the result structure
int locate_star(struct ccd_frame *fr, double x, double y, double r, double min_flux,
		struct star *s)
{
	struct rstats *rsn, *rs, *reg;
	struct moments m;
	int ret;
	int xc, yc;
	int ring, rn;
	double rmax;
	double minpk;
	double skycut, sky;
	int starr;
	double fwhm;

	rsn = malloc(sizeof(struct rstats));
	reg = malloc(sizeof(struct rstats));
	rs = malloc(sizeof(struct rstats));

// first, find the stats for the search region
	ret = ring_stats(fr, x, y, 0, r, QUAD1|QUAD2|QUAD3|QUAD4, reg, -HUGE, HUGE);
	if (ret) {
		s->xerr = BIG_ERR;
		s->yerr = BIG_ERR;
		free(rsn); free(reg); free(rs);
		return ret;
	}
	if (fr->stats.statsok && fr->stats.csigma < reg->sigma) {
		minpk = reg->median + NSIGMA * fr->stats.csigma;
	} else {
		minpk = reg->median + NSIGMA * reg->sigma;
	}

	x = floor(x + 0.5);
	y = floor(y + 0.5);
	xc = (int)x;
	yc = (int)y;

	for (ring = 0; ring < r; ring++) {
		rmax = HUGE;
		do {
			thin_ring_stats(fr, xc, yc, ring, rs, -HUGE, rmax);
			if (rs->max < minpk || rs->used == 0) {// no peak found, go to next ring
				d4_printf("Ring %d has no peak larger than %.2f\n", ring, minpk);
				break;
			}
// now check if the star is valid
			ret = star_radius(fr, rs->max_x, rs->max_y, rs->max, reg->median, &fwhm);
			if (ret < 0)
				goto badstar;
			else starr = ret;

// estimate sky here
			rn = SKYR * starr;
			if (rn > MAXSR)
				rn = MAXSR;

			thin_ring_stats(fr, rs->max_x, rs->max_y, rn, rsn, -HUGE, HUGE);
			if (fr->stats.statsok && fr->stats.csigma < rsn->sigma)
				skycut = NSIGMA * fr->stats.csigma;
			else
				skycut = NSIGMA * rsn->sigma;
			skycut += rsn->median;
			sky = rsn->median;
			if (rs->max < skycut) {
				d4_printf("peak %.2f lower than skycut %.2f\n", rs->max, skycut);
				goto badstar;
			}
// check that we have a few connected pixels above the cut
			ring_stats(fr, 1.0*rs->max_x, 1.0*rs->max_y, 0, 3,
				   QUAD1|QUAD2|QUAD3|QUAD4, rsn, skycut, HUGE);
			if (rsn->used < NCONN) {
				d4_printf("only %d connected pixels found\n", rsn->used);
				goto badstar;
			}
			s->peak = rsn->max;
// get the star's centroid and flux
			star_moments(fr, 1.0*rs->max_x, 1.0*rs->max_y, 1.0*starr, sky, &m);
// check star has enough flux
			if ((m.sum) < min_flux) {
				d4_printf("flux %.2f too low\n", (rsn->sum - rsn->used * skycut));
				goto badstar;
			}
// we finally have it; fill up return values and exit
			s->x = m.mx;
			s->y = m.my;
			s->flux = m.sum;
			s->starr = starr;
			s->fwhm = fwhm;
			s->sky = sky;
			s->npix = m.npix;
			if (m.sum < 1) {
				s->xerr = BIG_ERR;
				s->yerr = BIG_ERR;
			} else {
				double xw, yw;
				xw = sqrt(m.mx2 - sqr(m.mx));
				yw = sqrt(m.mx2 - sqr(m.mx));
				if (xw < 2)
					xw = 2;
				if (yw < 2)
					yw = 2;

				s->xerr = xw;
				s->yerr = yw;
			}
			d4_printf("found starr: %d\n", starr);
//			d3_printf("mx2:%.1f my2:%.1f mxy:%.1f\n", m.mx2, m.my2, m.mxy);
			moments_to_ecc(&m, &s->fwhm_pa, &s->fwhm_ec);
//			d3_printf("ecc:%.1f pa:%.1f\n", s->fwhm_ec, s->fwhm_pa);
//			s->fwhm_ec = 0;
//			s->fwhm_pa = 0;
			free(rsn); free(reg); free(rs);
			return 0;
		badstar:
// look for the next maximum in the ring
		rmax = rs->max - 0.01;
		} while (1);
	}
	s->xerr = BIG_ERR;
	s->yerr = BIG_ERR;
	free(rsn); free(reg); free(rs);
	return -1;
}

// Find where the current star has moved, whithin a radius of r
// The closest star with >0.7 the initial flux is selected
// returns 0 if a star was found, negative otherwise
int follow_star(struct ccd_frame *fr, double r, struct star *os, struct star *s)
{
	int ret;

// try to locate the star
	ret = locate_star(fr, os->x, os->y, r, 0.7*os->flux, s);
	if (ret != 0)
		return ret;
	return 0;
}

// find a star near the specified point, with it's flux larger than min_flux
// returns 0 if a star was found, negative otherwise
int get_star_near(struct ccd_frame *fr, int x, int y, double min_flux, struct star *s)
{
	int ret;

// try to locate the star
	ret = locate_star(fr, x*1.0, y*1.0, 10.0, min_flux, s);
	if (ret != 0)
		return ret;
	return 0;
}

// search for a place to insert star; return position
static int star_pos_search(struct sources *src, double flux)
{
	int i;

	for (i=0; i<src->ns; i++)
		if (flux > src->s[i].flux)
			break;
	return i;
}

// insert a star in the sources structure; only the first src->maxn
// brightest stars are kept; src->ns is updated to reflect the
// actual number of stars.
// returns 1 if the star was inserted, 0 is it was discarded, negative
// for error
static int insert_star(struct sources *src, struct star *s)
{
	int pos, i;

//	if (src->ns < 20)
//		d3_printf("insert (ns:%d maxn:%d flux:%.1f xy:%.1f,%.1f)\n",
//			  src->ns, src->maxn, s->flux, s->x, s->y);

	if (src->ns == 0 || src->s[src->ns-1].flux > s->flux) { // insert star at the end
		if (src->maxn > src->ns) {
			memcpy(src->s + src->ns, s, sizeof(struct star));
			src->ns ++;
			return 1;
		}
		return 0;
	}
//	return 0;

// if we get here, we must insert the star somewhere in the array
	pos = star_pos_search(src, s->flux);

//	if (src->ns < 20)
//		d3_printf("ns:%d maxn:%d pos:%d\n", src->ns, src->maxn, pos);

	if (pos > src->ns - 1) {
		err_printf("bad search result, aborting\n");
		return 0;
	}
// shift lower brightness stars down
	for (i = src->maxn - 2; i >= pos; i--) {
		memcpy(&src->s[i+1], &src->s[i], sizeof(struct star));
	}
	if (src->maxn > src->ns)
		src->ns ++;
	memcpy(&src->s[pos], s, sizeof(struct star));
	return 1;
}

// check if the current star is too near a previously detected one
// if it is, it replaces the old one if brighter
// returns 1 if the star is to be inserted in the list (it's not a multiple), 0
// otherwise

static int check_multiple(struct sources *src, struct star *s)
{
	int i;
	for (i = 0; i < src->ns; i++) {
		if (fabs(s->x - src->s[i].x) + fabs(s->y - src->s[i].y) < MINSEP) {
			if (s->flux > src->s[i].flux)
				memcpy(&src->s[i], s, sizeof(struct star));
			return 0;
		}
	}
	return 1;
}

// same as extract_stars, but uses an annulus instead of a box region
int extract_stars_annular(struct ccd_frame *fr, struct annular_region *reg, double min_flux, double sigmas, struct sources *src)
{
	struct rstats *rsn;
	struct moments m;
	struct star st;
	int ret;
	int x, y;
	int xs, ys, w, h, xc, yc;
	int rn;
	double minpk, pk;
	double skycut, sky;
	int starr;
	double fwhm;
	float dp;
	int candidates = 0;
	int r1sq, r2sq;

	if (reg != NULL) {
		xs = reg->xc - reg->r2;
		if (xs < 0)
			xs = 0;
		if (xs > fr->w - 1)
			xs = fr->w - 1;

		ys = reg->yc - reg->r2;
		if (ys < 0)
			ys = 0;
		if (ys > fr->h - 1)
			ys = fr->h - 1;

		w = 2 * reg->r2;
		h = 2 * reg->r2;
		xc = reg->xc;
		yc = reg->yc;
		r1sq = sqr(reg->r1);
		r2sq = sqr(reg->r2);
	} else {
		xs = 0;
		ys = 0;
		w = fr->w;
		h = fr->h;
		xc = fr->w / 2;
		yc = fr->h / 2;

		r1sq = 0;
		r2sq = sqr(fr->h / 2);
		if (r2sq > sqr(fr->w / 2))
			r2sq = sqr(fr->w / 2);
	}

	if (!fr->stats.statsok)
		frame_stats(fr);

//	d3_printf("extract_stars: frame size is %dx%d\n", fr->w, fr->h);

	minpk = fr->stats.cavg + 2 * sigmas * fr->stats.csigma;

	rsn = malloc(sizeof(struct rstats));

	for (y = ys; y < ys + h; y++) {
		for (x = xs; x < xs + w; x++) {
			if (x > fr->w - 1 || y > fr->h - 1)
				continue;

			double d = sqr(x - xc) + sqr(y - yc);
			if (d < r1sq || d > r2sq)
				continue;

			dp = get_pixel_luminence(fr, x, y);
			if ((pk = dp) <= minpk) {// below detection threshold
				continue;
			}
			candidates ++;
			if (candidates > MAX_STAR_CANDIDATES) {
				err_printf("extract_stars: Too many bad candidates, aborting\n");
				return 0;
			}
// now check if the star is valid
			ret = star_radius(fr, x, y, pk, minpk, &fwhm);
			if (ret < 0)
				continue;
			else starr = ret;
// estimate sky here
			rn = SKYR * starr;
			if (rn > MAXSR)
				rn = MAXSR;
			thin_ring_stats(fr, x, y, starr, rsn, -HUGE, HUGE);
			skycut = NSIGMA * rsn->sigma;
			thin_ring_stats(fr, x, y, rn, rsn, -HUGE, HUGE);
			skycut += rsn->median;
			sky = rsn->median;
			if (pk < skycut) {
//				d3_printf("peak %.2f lower than skycut %.2f\n", rs->max, skycut);
				continue;
			}
// check that we have a few connected pixels above the cut
			ret = ring_stats(fr, 1.0*x, 1.0*y, 0, 2,
					 QUAD1|QUAD2|QUAD3|QUAD4, rsn, skycut, HUGE);
			if (ret < 0 || rsn->used < NCONN) {
//				d3_printf("only %d connected pixels found\n", rsn->used);
				continue;
			}
			st.peak = rsn->max;
// get the star's centroid and flux
			star_moments(fr, 1.0*x, 1.0*y, 1.0*starr, sky, &m);
// check star has enough flux
			if ((m.sum) < min_flux) {
//				d3_printf("flux %.2f too low\n", (rsn->sum - rsn->used * skycut));
				continue;
			}
// we finally have it; fill up return values and exit
			st.x = m.mx;
			st.y = m.my;
			st.flux = m.sum;
			st.starr = starr;
			st.fwhm = fwhm;
//			d3_printf("mx2:%.1f my2:%.1f mxy:%.1f\n", m.mx2, m.my2, m.mxy);
			moments_to_ecc(&m, &st.fwhm_pa, &st.fwhm_ec);
//			d3_printf("ecc:%.1f pa:%.1f\n", s->fwhm_ec, s->fwhm_pa);
//			s->fwhm_ec = 0;
//			s->fwhm_pa = 0;
			st.datavalid = 1;
			if (check_multiple(src, &st)) {
//				d3_printf("candidates so far: %d\n", candidates);
				candidates = 0;
				insert_star(src, &st);
			}
		}
	}
	free(rsn);
	return src->ns;
}

int extract_stars(struct ccd_frame *fr, struct region *reg, double min_flux, double sigmas, struct sources *src)
{
	struct rstats *rsn;
	struct moments m;
	struct star st;
	int ret;
	int x, y;
	int xs, ys, w, h;
	int rn;
	double minpk, pk;
	double skycut, sky;
	int starr;
	double fwhm;
	float dp;
	int candidates = 0;

	if (reg != NULL) {
		xs = reg->xs;
		ys = reg->ys;
		w = reg->w;
		h = reg->h;
	} else {
		xs = 0;
		ys = 0;
		w = fr->w;
		h = fr->h;
	}

	if (!fr->stats.statsok)
		frame_stats(fr);

//	d3_printf("extract_stars: frame size is %dx%d\n", fr->w, fr->h);

	minpk = fr->stats.cavg + 2 * sigmas * fr->stats.csigma;

	rsn = malloc(sizeof(struct rstats));

	for (y = ys; y < ys + h; y++) {
		for (x = xs; x < xs + w; x++) {
			dp = get_pixel_luminence(fr, x, y);
			if ((pk = dp) <= minpk) {// below detection threshold
				continue;
			}
			candidates ++;
			if (candidates > MAX_STAR_CANDIDATES) {
				err_printf("extract_stars: Too many bad candidates, aborting\n");
				return 0;
			}
// now check if the star is valid
			ret = star_radius(fr, x, y, pk, minpk, &fwhm);
			if (ret < 0)
				continue;
			else starr = ret;
// estimate sky here
			rn = SKYR * starr;
			if (rn > MAXSR)
				rn = MAXSR;
			thin_ring_stats(fr, x, y, starr, rsn, -HUGE, HUGE);
			skycut = NSIGMA * rsn->sigma;
			thin_ring_stats(fr, x, y, rn, rsn, -HUGE, HUGE);
			skycut += rsn->median;
			sky = rsn->median;
			if (pk < skycut) {
//				d3_printf("peak %.2f lower than skycut %.2f\n", rs->max, skycut);
				continue;
			}
// check that we have a few connected pixels above the cut
			ret = ring_stats(fr, 1.0*x, 1.0*y, 0, 2,
					 QUAD1|QUAD2|QUAD3|QUAD4, rsn, skycut, HUGE);
			if (ret < 0 || rsn->used < NCONN) {
//				d3_printf("only %d connected pixels found\n", rsn->used);
				continue;
			}
			st.peak = rsn->max;
// get the star's centroid and flux
			star_moments(fr, 1.0*x, 1.0*y, 1.0*starr, sky, &m);
// check star has enough flux
			if ((m.sum) < min_flux) {
//				d3_printf("flux %.2f too low\n", (rsn->sum - rsn->used * skycut));
				continue;
			}
// we finally have it; fill up return values and exit
			st.x = m.mx;
			st.y = m.my;
			st.flux = m.sum;
			st.starr = starr;
			st.fwhm = fwhm;
//			d3_printf("mx2:%.1f my2:%.1f mxy:%.1f\n", m.mx2, m.my2, m.mxy);
			moments_to_ecc(&m, &st.fwhm_pa, &st.fwhm_ec);
//			d3_printf("ecc:%.1f pa:%.1f\n", s->fwhm_ec, s->fwhm_pa);
//			s->fwhm_ec = 0;
//			s->fwhm_pa = 0;
			st.datavalid = 1;
			if (check_multiple(src, &st)) {
//				d3_printf("candidates so far: %d\n", candidates);
				candidates = 0;
				insert_star(src, &st);
			}
		}
	}
	free(rsn);
	return src->ns;
}


// create an (empty) sources structure that can hold n stars
struct sources *new_sources(int n)
{
	void *stbl;
	struct sources *src;

	src = calloc(1, sizeof(struct sources));
	if (src == NULL) {
		err_printf("new_sources: cannot alloc sources structure\n");
		return NULL;
	}
	stbl = calloc(n, sizeof(struct star));
	if (stbl == NULL) {
		err_printf("new_sources: cannot alloc star table\n");
		free(src);
		return NULL;
	}
	src->maxn = n;
	src->s = stbl;
	src->ref_count = 1;
	return src;
}

// free a sources structure
void release_sources(struct sources *src)
{
	if (src->ref_count > 1) {
		src->ref_count --;
		return;
	}
	free(src->s);
	free(src);
}

void ref_sources(struct sources *src)
{
		src->ref_count ++;
}

void release_star(struct star *s)
{
	if (s == NULL)
		return;
	if (s->ref_count < 1)
		err_printf("cat_star has ref_count of %d\n", s->ref_count);
	if (s->ref_count == 1) {
		free(s);
	} else {
		s->ref_count --;
	}
}
