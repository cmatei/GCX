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

/* handling of image wcs */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "sourcesdraw.h"
#include "wcs.h"
#include "params.h"
#include "obsdata.h"
#include "sidereal_time.h"

//#define CHECK_WCS_CLOSURE

/* try to get an inital wcs from observation data
 */
void try_wcs_from_frame_obs(struct ccd_frame *fr, struct wcs *wcs)
{
	int havera=0, havedec=0, haveeq=0, havescale=0;
	double ra, dec, eq, scale;
	struct cat_star *cats;
	char name[80];
	int w_ref_count;

	if (fits_get_double(fr, P_STR(FN_RA), &ra) > 0)
		havera = 1;
	else if (fits_get_dms(fr, P_STR(FN_RA), &ra) > 0) {
		havera = 1;
		ra *= 15.0;
	} else if (fits_get_double(fr, P_STR(FN_OBJCTRA), &ra) > 0)
		havera = 1;
	else if (fits_get_dms(fr, P_STR(FN_OBJCTRA), &ra) > 0) {
		havera = 1;
		ra *= 15.0;
	}

	if (fits_get_double(fr, P_STR(FN_DEC), &dec) > 0)
		havedec = 1;
	else if (fits_get_dms(fr, P_STR(FN_DEC), &dec) > 0)
		havedec = 1;
	else if (fits_get_double(fr, P_STR(FN_OBJCTDEC), &dec) > 0)
		havedec = 1;
	else if (fits_get_dms(fr, P_STR(FN_OBJCTDEC), &dec) > 0)
		havedec = 1;

	if (fits_get_double(fr, P_STR(FN_EQUINOX), &eq) > 0)
		haveeq = 1;
	else if (fits_get_double(fr, P_STR(FN_EPOCH), &eq) > 0)
		haveeq = 1;

	if (fits_get_double(fr, P_STR(FN_SECPIX), &scale) > 0)
		havescale = 1;

	if (!havera || !havedec) {
		if (fits_get_string(fr, P_STR(FN_OBJECT), name, 80) <= 0)
			return;
		d3_printf("looking for object %s\n", name);
		cats = get_object_by_name(name);
		if (cats == NULL)
			return;
		ra = cats->ra;
		dec = cats->dec;
	}
	if (!haveeq)
		eq = 2000.0;

	if (!havescale)
		scale = P_DBL(WCS_SEC_PER_PIX);
	d3_printf("scale is %.2g\n", scale);

	fr->fim.xref = ra;
	fr->fim.yref = dec;
	fr->fim.xrefpix = fr->xs + (fr->xe - fr->xs) / 2;
	fr->fim.yrefpix = fr->ys + (fr->ye - fr->ys) / 2;
	fr->fim.rot = 0;
	fr->fim.equinox = eq;
	fr->fim.xinc = - scale / 3600.0;
	fr->fim.yinc = - scale / 3600.0;
	if (P_INT(OBS_FLIPPED))
		fr->fim.yinc = -fr->fim.yinc;
	fr->fim.wcsset = WCS_INITIAL;
	wcs->flags &= ~WCS_HINTED;

	w_ref_count = wcs->ref_count;
	memcpy(wcs, &(fr->fim), sizeof (struct wcs));
	wcs->ref_count = w_ref_count;
}

/* adjust the current wcs when a new frame is loaded
 */
void wcs_from_frame(struct ccd_frame *fr, struct wcs *wcs)
{
	int w_ref_count;
	double lat, lng;
	int hla=0, hlo=0;
	char s[80];

	rescan_fits_wcs(fr, &fr->fim);
	w_ref_count = wcs->ref_count;
	if (fr->fim.wcsset != WCS_INVALID) {
		memcpy(wcs, &(fr->fim), sizeof (struct wcs));
	} else {
		wcs->wcsset = WCS_INVALID;
		wcs->xrefpix = fr->xs + (fr->xe - fr->xs) / 2;
		wcs->yrefpix = fr->ys + (fr->ye - fr->ys) / 2;
		try_wcs_from_frame_obs(fr, wcs);
	}
	wcs->jd = frame_jdate(fr);
	if (wcs->jd != 0.0)
		wcs->flags |= WCS_JD_VALID;
	else
		wcs->flags &= ~WCS_JD_VALID;

	if (fits_get_double(fr, P_STR(FN_LATITUDE), &lat) > 0) {
		d1_printf("using latitude = %.3f from %s\n",
			  lng, P_STR(FN_LATITUDE));
		hla = 1;
	} else if (fits_get_string(fr, P_STR(FN_LATITUDE), s, 79) > 0) {
		if (!dms_to_degrees(s, &lat)) {
			d1_printf("using latitude = %.3f from %s\n",
				  lat, P_STR(FN_LATITUDE));
			hla = 1;
		}
	}
	if (fits_get_double(fr, P_STR(FN_LONGITUDE), &lng) > 0) {
		d1_printf("using %s longitude = %.3f from %s\n",
			  P_INT(FILE_WESTERN_LONGITUDES) ? "western" : "eastern",
			  lng, P_STR(FN_LONGITUDE));
		hlo = 1;
	} else if (fits_get_string(fr, P_STR(FN_LONGITUDE), s, 79) > 0) {
		if (!dms_to_degrees(s, &lng)) {
			d1_printf("using %s longitude = %.3f from %s\n",
				  P_INT(FILE_WESTERN_LONGITUDES) ? "western" : "eastern",
				  lng, P_STR(FN_LONGITUDE));
			hlo = 1;
		}
	}
	if (hlo) {
		if (!P_INT(FILE_WESTERN_LONGITUDES))
			lng = -lng;
		if (hla) {
			wcs->lng = lng;
			wcs->lat = lat;
			wcs->flags |= WCS_LOC_VALID;
		}
	}
	wcs->ref_count = w_ref_count;
}

/* creation/deletion of wcs
 */
struct wcs *wcs_new(void)
{
	struct wcs *wcs;
	wcs = calloc(1, sizeof(struct wcs));
	wcs->ref_count = 1;
	return wcs;
}

void wcs_ref(struct wcs *wcs)
{
	if (wcs == NULL)
		return;
	wcs->ref_count ++;
}

void wcs_release(struct wcs *wcs)
{
	if (wcs == NULL)
		return;
	if (wcs->ref_count < 1)
		g_warning("wcs has ref_count of %d on release\n", wcs->ref_count);
	if (wcs->ref_count == 1) {
		free(wcs);
	} else {
		wcs->ref_count --;
	}
}


/* refract coordinates in-place */
void refracted_from_true(double *ra, double *dec, double jd, double lat, double lng)
{
	double gast;
	double alt, az, R;

	gast = get_apparent_sidereal_time (jd);
	get_hrz_from_equ_sidereal_time (*ra, *dec, lng, lat, gast, &alt, &az);
	R = get_refraction_adj_true (alt, 1010, 10.0);
//	d3_printf("alt:%.3f az:%.3f R:%.3f\n", alt, az, R);
	alt += R ;
	get_equ_from_hrz_sidereal_time (alt, az, lng, lat, gast, ra, dec);
}



/* calculate the apparent position of a cat_star, for the epoch of jd. Nutation,
   abberation, obliquity, parallax and deflection are not taken into account
   - their effect is supposedly  "absorbed" in the wcs fit. */
void cats_apparent_pos(struct cat_star *cats, double *raa, double *deca, double jd)
{
	double ra = cats->ra;
	double dec = cats->dec;
	double epoch = 2000.0 + (jd - JD2000) / 365.25;

	/* proper motion */
	if ((cats->astro) && (cats->astro->flags & ASTRO_HAS_PM)) {
		ra += (epoch - cats->astro->epoch) * cats->astro->ra_pm / 3600000;
		dec += (epoch - cats->astro->epoch) * cats->astro->dec_pm / 3600000;
	}
	/* precess wcs and star to current epoch */
	precess_hiprec(cats->equinox, epoch, &ra, &dec);
	/* refraction */
	if (raa)
		*raa = ra;
	if (deca)
		*deca = dec;
}

/* un-refract coordinates in-place */
void true_from_refracted(double *ra, double *dec, double jd, double lat, double lng)
{
	double gast;
	double alt, az, R;
	double aalt;
	int i;

	gast = get_apparent_sidereal_time (jd);
	get_hrz_from_equ_sidereal_time (*ra, *dec, lng, lat, gast, &alt, &az);
	aalt = alt;
	for (i = 0; i < 40; i++) { 	/* iterate to reverse refraction */
		R = get_refraction_adj_true (alt, 1010, 10.0);
		if (fabs(alt - (aalt - R)) < 0.000001) {
			break;
		}
		alt = aalt - R;
	}
//	d3_printf("alt:%.3f R:%.5f i=%d\n", alt, R, i);
	get_equ_from_hrz_sidereal_time (alt, az, lng, lat, gast, ra, dec);
}


int w_worldpos(struct wcs *wcs, double xpix, double ypix, double *xpos, double *ypos)
{
	double X, E, ra, dec;
	double xref=wcs->xref;
	double yref=wcs->yref;
	double epoch = 2000.0 + (wcs->jd - JD2000) / 365.25;

//	d3_printf("pix x:%.4f y:%.4f\n", xpix, ypix);
	xy_to_XE(wcs, xpix, ypix, &X, &E);
//	d3_printf("pix X:%.4f E:%.4f\n", X, E);
	if (wcs->flags & WCS_JD_VALID) {
		precess_hiprec(wcs->equinox, epoch, &xref, &yref);
		if ((wcs->flags & WCS_LOC_VALID) && P_INT(WCS_REFRACTION_EN)) {
			refracted_from_true(&xref, &yref, wcs->jd, wcs->lat, wcs->lng);
		}
	}
	worldpos(X, E, xref, yref, 0.0,
		 0.0, 1.0, 1.0, 0.0, "-TAN", &ra, &dec);
//	d3_printf("pix apparent ra:%.4f dec:%.4f\n", ra, dec);
	if (wcs->flags & WCS_JD_VALID) {
		if ((wcs->flags & WCS_LOC_VALID) && P_INT(WCS_REFRACTION_EN)) {
			true_from_refracted(&ra, &dec, wcs->jd, wcs->lat, wcs->lng);
		}
		precess_hiprec(epoch, wcs->equinox, &ra, &dec);
	}
//	d3_printf("pix mean ra:%.4f dec:%.4f\n", ra, dec);
	if (xpos)
		*xpos = ra;
	if (ypos)
		*ypos = dec;
	return 0;
}

/* calculate the projection plane coordinates (xi, eta) of a cat_star, for the epoch of jd.
   If lat and lng are provided (either of them non-zero), refraction is taken into account.
   Nutation, abberation, obliquity, parallax and deflection are not taken into account
   - their effect is supposed to be "absorbed" in the wcs fit. */
void cats_to_XE (struct wcs *wcs, struct cat_star *cats, double *X, double *E)
{
	double xref=wcs->xref;
	double yref=wcs->yref;
	double ra, dec;
	double epoch = 2000.0 + (wcs->jd - JD2000) / 365.25;
//	d3_printf("\ninitial ra:%.4f dec:%.4f\n", cats->ra, cats->dec);
	if (wcs->flags & WCS_JD_VALID) {
		cats_apparent_pos(cats, &ra, &dec, wcs->jd);
		precess_hiprec(wcs->equinox, epoch, &xref, &yref);
		if ((wcs->flags & WCS_LOC_VALID) && P_INT(WCS_REFRACTION_EN)) {
			refracted_from_true(&ra, &dec, wcs->jd, wcs->lat, wcs->lng);
			refracted_from_true(&xref, &yref, wcs->jd, wcs->lat, wcs->lng);
		}
	} else {
		ra = cats->ra;
		dec = cats->dec;
	}
//	d3_printf("apparent ra:%.4f dec:%.4f\n", ra, dec);
	xypix(ra, dec, xref, yref, 0.0, 0.0, 1.0, 1.0, 0.0, "-TAN", X, E);
//	d3_printf("apparent X:%.4f E:%.4f\n", *X, *E);
}

/* calculate the pixel position of a cat_star, for the epoch of jd. If lat and lng are
   provided (either of them non-zero), refraction is taken into account. Nutation,
   abberation, obliquity, parallax and deflection are not taken into account
   - their effect is supposed to be "absorbed" in the wcs fit. */
void cats_xypix (struct wcs *wcs, struct cat_star *cats, double *xpix, double *ypix)
{
	double X, E;
#ifdef CHECK_WCS_CLOSURE
	double cra, cdec, cle = 0.0;
#endif
	cats_to_XE(wcs, cats, &X, &E);
	XE_to_xy(wcs, X, E, xpix, ypix);
//	d3_printf("apparent x:%.4f y:%.4f\n", *xpix, *ypix);

#ifdef CHECK_WCS_CLOSURE
	xy_to_XE(wcs, *xpix, *ypix, &cra, &cdec);
	cle = 3600.0 * sqrt(sqr(cra - X) + sqr(cdec - E));
#ifdef CHECK_WCS_CLOSURE_WORLD
	w_worldpos(wcs, *xpix, *ypix, &cra, &cdec);
	cle = 3600.0 * sqrt(sqr(cra - cats->ra) + sqr(cdec - cats->dec));
	d4_printf("wcs closure error at %.4f, %.4f is %.3g arcsec\n",
		  cats->ra, cats->dec, cle);
#endif
	if (cle > 0.001) {
		err_printf("wcs closure error at %.4f, %.4f is %.3g arcsec!\n",
			   cats->ra, cats->dec, cle);
	}
#endif
}

/* fit the wcs to match star pairs */
#define POS_ERR 1.0 /* expected position error of stars */
#define VLD_ERR P_DBL(WCS_ERR_VALIDATE) /* max error at which we validate the fit */
#define VLD_PAIRS P_INT(WCS_PAIRS_VALIDATE) /* minimum # of pairs at which we validate the fit */
#define MAX_ITER 200 /* max number of pair fitting iterations */
#define MIN_DIST_RS 5 * POS_ERR /* minimum distance between stars at which
				 * we use then for rotation/scale */
#define MAX_SE P_DBL(WCS_MAX_SCALE_CHANGE)  /* max scale err (per iteration) */
#define CONV_LIMIT 0.00001 /* convergnce limit; if error doesn't
			    * decrease nor than this limit, we stop iterating */
double gui_star_distance(struct gui_star *gs1, struct gui_star *gs2)
{
	return sqrt(sqr(gs1->x - gs2->x) + sqr(gs1->y - gs2->y));
}

/* in-place rotation about origin (angle in degrees) */
static void rot_vect(double *x, double *y, double a)
{
	double nx, ny;

	a *= PI / 180.0;
	nx = *x * cos(a) - *y * sin(a);
	ny = *x * sin(a) + *y * cos(a);
	*x = nx; *y = ny;
}

static void pairs_change_wcs(GSList *pairs, struct wcs *wcs)
{
	struct gui_star *gs;
	struct cat_star *cats;

	while (pairs != NULL) {
		gs = GUI_STAR(pairs->data);
		if (gs->pair != NULL) {
			gs = GUI_STAR(gs->pair);
		} else {
			err_printf("Not a real pair !\n");
			return;
		}
		if (gs->s != NULL) {
			cats = CAT_STAR(gs->s);
		} else {
			err_printf("No cat star found!\n");
			return;
		}
		pairs = g_slist_next(pairs);
		cats_xypix(wcs, cats, &(gs->x), &(gs->y));
	}
}

void cat_change_wcs(GSList *sl, struct wcs *wcs)
{
	struct gui_star *gs;
	struct cat_star *cats;

	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK_CATREF) && gs->s != NULL) {
			cats = CAT_STAR(gs->s);
			cats_xypix(wcs, cats, &(gs->x), &(gs->y));
		}
		sl = g_slist_next(sl);
	}
}


static void adjust_wcs(struct wcs *wcs, double dx, double dy, double ds, double dtheta)
{
	double t;
	d4_printf("adjust xref=%.3f yref=%.3f rot=%.3f\n",
		  wcs->xref, wcs->yref, wcs->rot, dx, dy, dtheta);
	if (wcs->xinc * wcs->yinc < 0) { /* flipped */
		wcs->rot += dtheta;
		t = degrad(dtheta);
	} else {
		wcs->rot -= dtheta;
		t = -degrad(dtheta);
	}
	if (fabs(dtheta) < 5 && fabs(ds - 1) < 0.05) {
		if (wcs->xinc * wcs->yinc < 0) { /* flipped */
			rot_vect(&dx, &dy, -wcs->rot);
		} else {
			rot_vect(&dx, &dy, wcs->rot);
		}
		wcs->xref -= wcs->xinc * dx;
		wcs->yref -= wcs->yinc * dy;
	}
	wcs->xinc /= ds;
	wcs->yinc /= ds;
	wcs->pc[0][0] = wcs->pc[0][0] * cos(t) - wcs->pc[0][1] * sin(t);
	wcs->pc[0][1] = wcs->pc[0][0] * sin(t) + wcs->pc[0][1] * cos(t);
	wcs->pc[1][0] = wcs->pc[1][0] * cos(t) - wcs->pc[1][1] * sin(t);
	wcs->pc[1][1] = wcs->pc[1][0] * sin(t) + wcs->pc[1][1] * cos(t);
	d4_printf("adjust result xref=%.3f yref=%.3f rot=%.3f t=%.5f\n",
		  wcs->xref, wcs->yref, wcs->rot, t);
}

/* compute rms of distance between paired stars */
double pairs_fit_err(GSList *pairs)
{
	struct gui_star *gs, *cgs;
	GSList *sl;
	double sumsq = 0.0;
	int n = 0;

	sl = pairs;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		cgs = GUI_STAR(gs->pair);
		sumsq += sqr(gui_star_distance(gs, cgs));
		sl = g_slist_next(sl);
		n++;
	}
	if (n != 0) {
		return sqrt(sumsq / n);
	} else {
		return 0.0;
	}
}

/* compute rms of ra and de distances between paired stars */
void pairs_fit_errxy(GSList *pairs, struct wcs *wcs, double *ra_err, double *de_err)
{
	struct gui_star *gs, *cgs;
	GSList *sl;
	double sumsq_ra = 0.0, sumsq_de = 0.0;
	double gs_ra, gs_de, cgs_ra, cgs_de;
	int n = 0;

	sl = pairs;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		cgs = GUI_STAR(gs->pair);
		w_worldpos(wcs, gs->x, gs->y, &gs_ra, &gs_de);
		w_worldpos(wcs, cgs->x, cgs->y, &cgs_ra, &cgs_de);
		sumsq_ra += sqr((gs_ra - cgs_ra) * cos(degrad((gs_de + cgs_de)/2.0)));
		sumsq_de += sqr(gs_de - cgs_de);
//		sumsq_ra += sqr(fabs(gs->x - cgs->x));
//		sumsq_de += sqr(fabs(gs->y - cgs->y));
		sl = g_slist_next(sl);
		n++;
	}
	if (n != 0) {
		*ra_err = sqrt(sumsq_ra / n);
		*de_err = sqrt(sumsq_de / n);
	} else {
		*ra_err = *de_err = 0.0;
	}
}

/* compute the angular difference between t and ct (radians)
 * taking 2pi wrapping into account (and convert to degrees)
 */
static double angular_diff(double t, double ct)
{
	double diff;
	diff = (t - ct);
	if (diff > PI)
		diff -= 2 * PI;
	if (diff < -PI)
		diff += 2 * PI;
	diff *= 180 / PI;
	return diff;
}

/* Calculate dx, dy, ds and dtheta for a set of pairs
   If scale_en is false, scale is not fitted; if rot_en
   is false, rotation is not fitted */

int pairs_cs_diff(GSList *pairs, double *dxo, double *dyo,
		  double *dso, double *dthetao, int scale_en, int rot_en)
{
	double dx=0.0;
	double dy=0.0;
	double dtheta = 0.0;
	double ds = 0.0;
	double st_weights = 0.0;
	double xy_weights = 0.0;
	struct gui_star *gs, *cgs;
	struct gui_star *ngs, *ncgs;
	GSList *sl;
	double s, cs, t, ct; /* scale/rotation */

	sl = pairs;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK_FRSTAR) == 0) {
			err_printf("fit_pairs_xy: first star in pair must be a frstar \n");
			return -1;
		}
		cgs = GUI_STAR(gs->pair);
		if ((TYPE_MASK_GSTAR(cgs) & (TYPE_MASK_CATREF | TYPE_MASK_ALIGN)) == 0) {
			err_printf("fit_pairs_xy: second star in pair must be a catref \n");
			return -1;
		}
		sl = g_slist_next(sl);
		if (sl == NULL) {
			ngs = GUI_STAR(pairs->data);
		} else {
			ngs = GUI_STAR(sl->data);
		}
		ncgs = GUI_STAR(ngs->pair);
		s = gui_star_distance(gs, ngs);
		cs = gui_star_distance(cgs, ncgs);
		if (s < MIN_DIST_RS || cs < MIN_DIST_RS) {
//			d3_printf("rot skip \n");
/* skip this one for rot/scale calculations */
			continue;
		}
		t = atan2(ngs->y - gs->y, ngs->x - gs->x);
		ct = atan2(ncgs->y - cgs->y, ncgs->x - cgs->x);
		st_weights += sqr(cs);
		dtheta += sqr(cs) * angular_diff(t, ct);
		d4_printf("t=%.5f, ct = %.5f, dtheta: %.3f, stw: %.3f\n",
			  t, ct, dtheta/st_weights, st_weights);
		ds += sqr(cs) * (s / cs);
	}
	if (st_weights == 0) {
//		d3_printf("notheta\n");
		dtheta = 0;
		ds = 1.0;
	} else {
		dtheta = dtheta / st_weights;
		ds = ds / st_weights;
	}

	if (!scale_en)
		ds = 1.0;
	if (!rot_en)
		dtheta = 0.0;

	sl = pairs;
	while (sl != NULL) {
		double gx, gy;
		gs = GUI_STAR(sl->data);
		if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK_FRSTAR) == 0) {
			err_printf("fit_pairs_xy: first star in pair must be a frstar \n");
			return -1;
		}
		cgs = GUI_STAR(gs->pair);
		if ((TYPE_MASK_GSTAR(cgs) & (TYPE_MASK_CATREF | TYPE_MASK_ALIGN)) == 0) {
			err_printf("fit_pairs_xy: second star in pair must be a catref \n");
			return -1;
		}
		gx = gs->x;
		gy = gs->y;
		rot_vect(&gx, &gy, - dtheta);
		gx /= ds;
		gy /= ds;
		dx += gx - cgs->x;
		dy += gy - cgs->y;
		xy_weights += 1.0;
		sl = g_slist_next(sl);
	}
	dx /= xy_weights;
	dy /= xy_weights;
	if (dxo)
		*dxo = dx;
	if (dyo)
		*dyo = dy;
	if (dso)
		*dso = ds;
	if (dthetao)
		*dthetao = dtheta;
	d4_printf("fit deltas: dx:%.5f dy: %.5f ds: %.5f dt: %.5f\n",
		  dx, dy, ds, dtheta);
	return 0;
}

/* take a list of pairs and change wcs for best pair fit */
/* returns -1 if there was a problem trying to fit */
static int fit_pairs_xy(GSList *pairs, struct wcs *wcs)
{
	double dx=0.0;
	double dy=0.0;
	double dtheta = 0.0;
	double ds = 0.0;
	int ret;

	if ((ret = pairs_cs_diff(pairs, &dx, &dy, &ds, &dtheta, 1, 1)))
		return ret;
	clamp_double(&ds, 1 - MAX_SE, 1 + MAX_SE);
	adjust_wcs(wcs, dx, dy, ds, dtheta);
	return 0;
}

/* fit wcs pairs the "old" way */
static void fit_pairs_old(GSList *pairs, struct wcs *wcs)
{
	int iteration = 0;
	double fit_err = HUGE;

	while (iteration < MAX_ITER) {
//		d3_printf("iteration: %d pairs: %d err: %f\n",
//			  iteration, npairs, fit_err);
		iteration ++;
		fit_pairs_xy(pairs, wcs);
		pairs_change_wcs(pairs, wcs);
		fit_err = pairs_fit_err(pairs);
		if (fabs(fit_err - wcs->fit_err) <= CONV_LIMIT) {
//			d3_printf("stop\n");
			/* we are at best fit*/
			break;
		}
		wcs->fit_err = fit_err;
	}
}

/* fit wcs pairs the "new" way (calling plate.c functions) */
static int fit_pairs(GSList *pairs, struct wcs *wcs)
{
	struct fit_pair *fpairs;
	int npairs, n = 0;
	int ret = 0;
	int model;
	GSList *p;

	npairs = g_slist_length(pairs);
	fpairs = calloc(npairs, sizeof(struct fit_pair));
	g_assert(fpairs != NULL);

	for (p = pairs; p != NULL; p = p->next) {
		struct gui_star *gs, *cgs;
		struct cat_star *cats;
		gs = GUI_STAR(p->data);
		if ((TYPE_MASK_GSTAR(gs) & TYPE_MASK_FRSTAR) == 0) {
			err_printf("fit_pairs: first star in pair must be a frstar \n");
			ret = -1;
			goto fexit;
		}
		cgs = GUI_STAR(gs->pair);
		if ( ((TYPE_MASK_GSTAR(cgs) & (TYPE_MASK_CATREF)) == 0) || (cgs->s == NULL) ) {
			err_printf("fit_pairs: second star in pair must be a catref \n");
			ret = -1;
			goto fexit;
		}
		/* TODO: re-extract centroid and get true error */
		fpairs[n].x = gs->x;
		fpairs[n].y = gs->y;
		fpairs[n].poserr = 1.0;
		cats = cgs->s;
		cats_to_XE(wcs, cats, &(fpairs[n].X0), &(fpairs[n].E0));
		fpairs[n].caterr = cats->perr;
		xy_to_XE(wcs, fpairs[n].x, fpairs[n].y, &(fpairs[n].X), &(fpairs[n].E));
		fpairs[n].cats = cats;
		n++;
	}
	model = P_INT(WCSFIT_MODEL);
	if (model == PAR_WCSFIT_MODEL_AUTO) {
		if (n >= 10)
			model = PAR_WCSFIT_MODEL_LINEAR;
		else
			model = PAR_WCSFIT_MODEL_SIMPLE;
	}
	ret = plate_solve(wcs, fpairs, n, model);

fexit:
	free(fpairs);
	return ret;


}

/* return 0 if a good fit was found, -1 if we had an error */
int window_fit_wcs(GtkWidget *window)
{
	struct gui_star_list *gsl;
	struct gui_star *gs, *cgs;
	struct wcs *wcs;
	GSList *pairs=NULL, *goodpairs=NULL, *sl;
	double ra_err = HUGE, de_err = HUGE;
	int npairs, gpairs;
	char buf[256];
	struct image_channel *i_chan;

	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs == NULL || wcs->wcsset == WCS_INVALID) {
		err_printf_sb2(window, "Need an Initial WCS to Attempt Fit");
		error_beep();
		return -1;
	}
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		err_printf_sb2(window, "Need Sources Pairs to Attempt WCS Fit");
		error_beep();
		return -1;
	}
//	d3_printf("initial xinc: %f\n", wcs->xinc);
	sl = gsl->sl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		if ((gs->flags & STAR_HAS_PAIR) && gs->pair != NULL) {
			pairs = g_slist_append(pairs, gs);
			cgs = GUI_STAR(gs->pair);
			if ((TYPE_MASK_GSTAR(cgs) & (TYPE_MASK_CATREF)) && (cgs->s != NULL)) {
				if (CAT_STAR(cgs->s)->perr < BIG_ERR)
					goodpairs = g_slist_append(goodpairs, gs);
			}
		}
		sl = g_slist_next(sl);
	}
	npairs = g_slist_length(pairs);
	if (npairs < 2) {
		err_printf_sb2(window, "Need at Least 2 Pairs to Attempt WCS Fit");
		error_beep();
		g_slist_free(pairs);
		return -1;
	}
	gpairs = g_slist_length(goodpairs);
	if (gpairs >= 3 && gpairs > VLD_PAIRS && gpairs > P_INT(FIT_MIN_PAIRS)) {
		npairs = g_slist_length(goodpairs);
		d3_printf("using %d good pairs\n", npairs);
		g_slist_free(pairs);
		pairs = goodpairs;
	}
	wcs->fit_err = HUGE;

	if (npairs >= 2) {
		if (fit_pairs(pairs, wcs) < 0)
			fit_pairs_old(pairs, wcs);
	}

	pairs_fit_errxy(pairs, wcs, &ra_err, &de_err);
	if (npairs >= VLD_PAIRS && wcs->fit_err < VLD_ERR) {
		wcs->wcsset = WCS_VALID;
		wcs->flags &= ~WCS_HINTED;
		sprintf(buf, "Fitted %d pairs, Error in R.A.: %.2f\", Error in dec: %.2f\". Fit Validated",
			npairs, ra_err*3600.0, de_err*3600.0);
	} else {
		sprintf(buf, "Fitted %d pairs, Error in R.A.: %.2f\", Error in dec: %.2f\". Fit Not Validated",
			npairs, ra_err*3600.0, de_err*3600.0);
	}
	g_slist_free(pairs);
	info_printf_sb2(window, buf);
	cat_change_wcs(gsl->sl, wcs);

	i_chan = g_object_get_data(G_OBJECT(window), "i_channel");
	if (wcs == NULL || i_chan == NULL || i_chan->fr == NULL)
		return 0;
	memcpy(&(i_chan->fr->fim), wcs, sizeof (struct wcs));

//	d3_printf("final xinc: %f\n", wcs->xinc);
	return 0;
}





static int gui_star_compare(struct gui_star *a, struct gui_star *b)
{
	if (a->size > b->size)
		return -1;
	if (a->size < b->size)
		return 1;
	return 0;
}

/* find pairs of catalog->field stars
 * using starmatch and mark them in the gui_star_list
 * return number of matches found
 */
int auto_pairs(struct gui_star_list *gsl)
{
	GSList *cat = NULL, *field, *sl;
	int ret;
	int fastmatch(GSList *field, GSList *cat);

//	cat = filter_selection(gsl->sl, (TYPE_MASK_CATREF | TYPE_MASK_ALIGN), 0, 0);
	for (sl = gsl->sl; sl != NULL; sl = sl->next) {
		struct gui_star *gs;
		gs = GUI_STAR(sl->data);
		if ((TYPE_MASK_GSTAR(gs)) & TYPE_MASK_CATREF) {
			if (gs->s == NULL)
				continue;
			if (P_INT(AP_MOVE_TARGETS) &&
			    ((CAT_STAR(gs->s)->flags & CAT_ASTROMET) == 0))
				continue;
			cat = g_slist_prepend(cat, gs);
			continue;
		}
		if ((TYPE_MASK_GSTAR(gs)) & TYPE_MASK_ALIGN) {
			cat = g_slist_prepend(cat, gs);
			continue;
		}
	}
	d3_printf("matching to %d cat stars\n", g_slist_length(cat));
	field = filter_selection(gsl->sl, TYPE_MASK_FRSTAR, 0, 0);

	cat = g_slist_sort(cat, (GCompareFunc)gui_star_compare);
	field = g_slist_sort(field, (GCompareFunc)gui_star_compare);


/*
	for (sl = cat; sl != NULL; sl = sl->next)
		d3_printf("%s: size: %.3f mag:%.3f\n",
			  CAT_STAR(GUI_STAR(sl->data)->s)->name,
			  GUI_STAR(sl->data)->size,
			  CAT_STAR(GUI_STAR(sl->data)->s)->mag);
*/


	if (cat == NULL || field == NULL) {
		g_slist_free(cat);
		g_slist_free(field);
		return 0;
	}

	ret = fastmatch(field, cat);
	g_slist_free(field);
	g_slist_free(cat);
	return ret;
}

#define SCALE_TOL P_DBL(FIT_SCALE_TOL) /*0.1 tolerance of scale */
#define MATCH_TOL P_DBL(FIT_MATCH_TOL) /* 1max error to accept a match (in pixels) */
#define ROT_TOL P_DBL(FIT_ROT_TOL) /*20 max rotation tolerance */
#define MIN_PAIRS P_INT(FIT_MIN_PAIRS) /*5 minimum number of pairs for a match */
#define MAX_PAIRS P_INT(FIT_MAX_PAIRS) /*22 we stop when we find this many pairs */
#define MAX_F_SKIP P_INT(FIT_MAX_SKIP) /* 5max number of field stars we skip as unmatchable */
#define MIN_AB_DISTANCE P_DBL(FIT_MIN_AB_DIST)  /* min distance between the first two field star to
						 * consider for matching */

static double gui_star_pa(struct gui_star *b, struct gui_star *a)
{
	return atan2((b->y - a->y), b->x - a->x);
}

/* find a catalog star that matches the transformed position of c within
 * MATCH_TOL
 */

struct gui_star * find_cc(struct gui_star *a, struct gui_star *b,
			  struct gui_star *ca, struct gui_star *cb,
			  struct gui_star *c, GSList *cat)
{
	double d_ccca;
	double pa_ccca;
	double xmin, xmax, ymin, ymax;
	double x, y;
	struct gui_star *cc;

//	d3_printf("c: %.1f %.1f\n", c->x, c->y);

	d_ccca = gui_star_distance(a, c) * gui_star_distance(ca, cb) / gui_star_distance(a, b);
	pa_ccca = (gui_star_pa(c, a) - gui_star_pa(b, a)) + gui_star_pa(cb, ca);

	x = ca->x + d_ccca * cos(pa_ccca);
	y = ca->y + d_ccca * sin(pa_ccca);

//	d3_printf("looking around %.1f %.1f\n", x, y);

	xmin = x - MATCH_TOL;
	xmax = x + MATCH_TOL;
	ymin = y - MATCH_TOL;
	ymax = y + MATCH_TOL;

	while (cat != NULL) {
		cc = GUI_STAR(cat->data);
		cat = g_slist_next(cat);
//		d3_printf("----- cc: %.1f %.1f\n", cc->x, cc->y);
		if (cc->x > xmax || cc->x < xmin || cc->y < ymin || cc->y > ymax)
			continue;
		return cc;
	}
	return NULL;
}


/* find stars that are positioned relative to ca as b is to a (taking the
 * tolerances into account) */
GSList * find_likely_cb(struct gui_star *a, struct gui_star *b,
			struct gui_star *ca, GSList *cat)
{
	GSList *ret = NULL;
	struct gui_star *cb;
	double d_ba;
	double pa_ba;
	double d_cbca;
	double pa_cbca;
	double dmin, dmax, pamin, pamax;

	d_ba = gui_star_distance(a, b);
	pa_ba = gui_star_pa(b, a);
	dmin = d_ba * (1 - SCALE_TOL);
	dmax = d_ba * (1 + SCALE_TOL);
	pamin = pa_ba - ROT_TOL * PI / 180;
	pamax = pa_ba + ROT_TOL * PI / 180;

	while (cat != NULL) {
		cb = GUI_STAR(cat->data);
		cat = g_slist_next(cat);
		d_cbca = gui_star_distance(ca, cb);
		if (d_cbca > dmax || d_cbca < dmin)
			continue;
		pa_cbca = gui_star_pa(cb, ca);
//		d3_printf("distance\n");
//		d3_printf("------> d_ba: %.1f, pa_ba: %.3f\n", d_ba, pa_ba * 180 / PI);
//		d3_printf("d_cbca: %.1f pa_cbca=%.3f \n", d_cbca, pa_cbca * 180 / PI);
		if (pa_cbca > pamax || pa_cbca < pamin)
			continue;
		ret = g_slist_append(ret, cb);
	}
//	if (g_slist_length(ret))
//		d3_printf("found %d\n", g_slist_length(ret));
	return ret;
}

void make_pair(struct gui_star *cs, struct gui_star *s)
{
	if ((cs->flags & STAR_HAS_PAIR) && (s->flags & STAR_HAS_PAIR))
		return;
	gui_star_ref(cs);
	cs->flags |= STAR_HAS_PAIR;
	s->flags |= STAR_HAS_PAIR;
	s->pair = cs;
}

/* try to match more stars from field_c to the catalog
 * stop when MAX_PAIRS pairs are found.
 * at most MAX_F_SKIP field stars are skipped as unmatchable
 * before returning
 *
 * the pairs are appended to the m and cm lists
 * return the number of pairs found
 */
int more_pairs(struct gui_star *a, struct gui_star *b,
	       struct gui_star *ca, struct gui_star *cb,
	       GSList *field_c, GSList *cat,
	       GSList **m, GSList **cm)
{
	int pairs = 0;
	int skipped = 0;
	struct gui_star *cc;

	if (field_c == NULL)
		return 0;

	do {
		cc = find_cc(a, b, ca, cb,
			     GUI_STAR(field_c->data), cat);
		if (cc != NULL) {
			*m = g_slist_append(*m, GUI_STAR(field_c->data));
			*cm = g_slist_append(*cm, cc);
			pairs ++;
//			d3_printf("adding pair\n");
		} else {
			skipped ++;
		}
		field_c = g_slist_next(field_c);
		if (skipped >= MAX_F_SKIP)
			break;
		if (pairs >= MAX_PAIRS)
			break;
	} while(field_c != NULL);
//	d3_printf("more: returning %d\n", pairs);
	return pairs;
}

/* take the lists of cat and field stars and mark them as pairs
 */
static void make_pairs_from_list(GSList *cm, GSList *m)
{
	while (cm != NULL && m != NULL) {
		make_pair(GUI_STAR(cm->data), GUI_STAR(m->data));
		cm = g_slist_next(cm);
		m = g_slist_next(m);
	}
}


/* try to match strting with the first two stars in field
 * return number of stars matched
 */
int match_from(GSList *field, GSList *cat)
{
	GSList *cat_a;
	GSList *cat_b, *cat_b_start;
	GSList *field_c, *fc;


	GSList *m=NULL, *cm=NULL;
	int ret, max = 0;
	int cskips;
	struct gui_star *a, *b, *c;
	struct gui_star *ca, *cb, *cc;

/*
	for (sl = field; sl != NULL; sl = sl->next)
		d3_printf("FIELD: %.0f, %.0f: size: \n",
			  GUI_STAR(sl->data)->x,
			  GUI_STAR(sl->data)->y,
			  GUI_STAR(sl->data)->size);
*/



	a = GUI_STAR(field->data);
	do {
		b = GUI_STAR(field->next->data);
		fc = field->next->next;
		field = g_slist_next(field);
	} while (field->next->next != NULL &&
		 gui_star_distance(a, b) < MIN_AB_DISTANCE);

/* this is a n**3 algorithm, which really should be optimised */
	cat_a = cat;
//	d3_printf("-----------------------> a: %.1f %.1f\n", a->x, a->y);
	while (cat_a != NULL) {
		ca = GUI_STAR(cat_a->data);
		cat_b = find_likely_cb(a, b, ca, cat);
		cat_b_start = cat_b;
		field_c = fc;
		cskips = MAX_F_SKIP; /* max number of skips until we find c */
//		if (gui_star_distance(a, ca) > 10.0)
//			cat_b = NULL;
//		d3_printf("a: %.1f %.1f, b: %.1f. %.1f\n", a->x, a->y, b->x, b->y);
//		d3_printf("ca: %.1f %.1f\n", ca->x, ca->y);
//		d3_printf("found %d likely cb\n", g_slist_length(cat_b));

		while (cat_b != NULL && field_c != NULL) {
			cb = GUI_STAR(cat_b->data);
//			d3_printf("a: %.1f %.1f, b: %.1f. %.1f\n", a->x, a->y, b->x, b->y);
//			d3_printf("ca: %.1f %.1f, cb: %.1f %.1f\n", ca->x, ca->y,
//				  cb->x, cb->y);
			cc = find_cc(a, b, ca, cb,
				     GUI_STAR(field_c->data), cat);
			if (cc == NULL && cskips > 0) {
				field_c = field_c->next;
				cskips --;
				continue;
			}
			cat_b = g_slist_next(cat_b);
			if (cc != NULL) {
				c = GUI_STAR(field_c->data);
				field_c = g_slist_next(field_c);
//				d3_printf("field_c = %08x\n", (unsigned) field_c);
				ret = more_pairs(a, b, ca, cb,
						 field_c, cat, &m, &cm);
				if (ret + 3 > max)
					max = ret + 3;
				if (ret + 3 < MIN_PAIRS) {
					g_slist_free(m);
					g_slist_free(cm);
					m = NULL;
					cm = NULL;
					d3_printf("found only %d pairs, trying for more\n",
						  ret+3);
					continue;
				}
				/* we have a match! */
				d3_printf("matched %d ;-)\n", ret+3);
				/* attach pairs */
				make_pair(ca, a);
				make_pair(cb, b);
				make_pair(cc, c);
				make_pairs_from_list(cm, m);
				g_slist_free(cat_b_start);
				g_slist_free(m);
				g_slist_free(cm);
				m = NULL;
				cm = NULL;
				return ret+3;
			}
		}
		g_slist_free(cat_b);
		cat_a = g_slist_next(cat_a);
	}
//	d3_printf("no match ;-(\n");
	return max;
}



/*
 * match the field stars to the catalog. create pairs in field
 * for the stars that are matched. Return the number of matches found
 * for best performance, the field list should be sorted by flux, so the first
 * stars are likely to be in the catalog.
 */
int fastmatch(GSList *field, GSList *cat)
{
	int ret = 0;
	int max = 0;

	while (g_slist_length(field) >= 3) { /* loop dropping the first star in the list */
		ret = match_from(field, cat);
		if (ret > max)
			max = ret;
		if (ret >= MIN_PAIRS)
			break;
		field = g_slist_next(field);
	}
	if (ret < MIN_PAIRS) {
		err_printf("Only found %d pairs, need at least %d\n", max, MIN_PAIRS);
		return 0;
	}
	return ret;
}
