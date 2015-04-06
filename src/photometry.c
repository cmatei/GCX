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

/* do a photometry run; mostly ugly glue code between the gui_star_list, libccd
   aperture_photometry and photsolve.c */

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
#include "misc.h"

/* photometry actions */
#define PHOT_CENTER_STARS 1
#define PHOT_RUN 2
#define PHOT_ACTION_MASK 0xff
#define PHOT_CENTER_PLOT 3

#define PHOT_TO_STDOUT 0x400
#define PHOT_TO_STDOUT_AA 0x500
#define PHOT_TO_FILE 0x100
#define PHOT_TO_FILE_AA 0x600
#define PHOT_TO_MBDS 0x200
#define PHOT_OUTPUT_MASK 0xff00

static double stf_scint(struct stf *stf);

static double distance(double x1, double y1, double x2, double y2)
{
	return sqrt(sqr(x1 - x2) + sqr(y1 - y2));
}

/* initialise ap_params from user parameters */
void ap_params_from_par(struct ap_params *ap)
{
	ap->r1 = P_DBL(AP_R1);
	ap->r2 = P_DBL(AP_R2);
	ap->r3 = P_DBL(AP_R3);
	ap->quads = ALLQUADS;
	ap->sky_method = P_INT(AP_SKY_METHOD);
//	ap->ap_shape = P_INT(AP_SHAPE);
	ap->sigmas = P_DBL(AP_SIGMAS);
//	ap->max_iter = P_INT(AP_MAX_ITER);
	ap->sat_limit = P_DBL(AP_SATURATION);
	ap->std_err = 0.0;
	ap->grow = P_INT(AP_SKY_GROW);
}

/* calculate scintillation for frame */
static double stf_scint(struct stf *stf)
{
	double t, d, am;

	if (!stf_find_double(stf, &t, 1, SYM_OBSERVATION, SYM_EXPTIME)) {
		err_printf("cannot calculate scintillation: no exptime\n");
		return 0.0;
	}
	if (!stf_find_double(stf, &am, 1, SYM_OBSERVATION, SYM_AIRMASS)) {
		err_printf("cannot calculate scintillation: no airmass\n");
		return 0.0;
	}
	if (!stf_find_double(stf, &d, 1, SYM_OBSERVATION, SYM_APERTURE)) {
		d = P_DBL(OBS_APERTURE);
		d1_printf("using default aperture %.1f for scintillation\n", d);
	}
	return scintillation(t, d, am);
}


/* estract airmass-calculation pars. return 0 if succesful */
static int stf_am_pars(struct stf *stf, double *lat, double *lng, double *jd)
{
	double mjd;
	char *la, *ln;

	la = stf_find_string(stf, 1, SYM_OBSERVATION, SYM_LATITUDE);
	ln = stf_find_string(stf, 1, SYM_OBSERVATION, SYM_LONGITUDE);

	if (la == NULL || dms_to_degrees(la, lat))
		return -1;
	if (ln == NULL || dms_to_degrees(ln, lng))
		return -1;

	if (!stf_find_double(stf, &mjd, 1, SYM_OBSERVATION, SYM_MJD)) {
		d3_printf("no time\n");
		return -1;
	}
	*jd = mjd_to_jd(mjd);
	return 0;
}


/* return 1 if star is inside the frame, with a margin of margin pixels */
static int star_in_frame(struct ccd_frame *fr, double x, double y, int margin)
{
	if (x < margin || x > fr->w - margin
	    || y < margin || y > fr->h - margin)
		return 0;
	if (sqr(x - fr->w / 2) + sqr(y - fr->h / 2) >
		sqr(P_DBL(AP_MAX_STD_RADIUS)) * (sqr(fr->w / 2) + sqr(fr->h / 2)))
		return 0;
	return 1;
}

/* center the star; return 0 if successfull, <0 if it couldn't be centered */
/* the error fields of the star are filled with the width as determined
 * from moments. it should be multiplied by the snr to obtain the position error. */
int center_star(struct ccd_frame *fr, struct star *st, double max_ce)
{
	struct star sf;

	if (st->x < 0 || st->x > fr->w || st->y < 0 || st->y > fr->h)
		return -1;
	d3_printf("Star near %.4g %.4g ", st->x, st->y);
	get_star_near(fr, (int)(round(st->x)), (int)(round(st->y)), 0, &sf);
	d3_printf("found at %.4g %.4g ", sf.x, sf.y) ;
	if (((distance(st->x, st->y, sf.x, sf.y)) > max_ce)) {
		st->xerr = BIG_ERR;
		st->yerr = BIG_ERR;
		d3_printf(" -- too far, skipped\n");
		return -1;
	} else {
		st->x = sf.x;
		st->y = sf.y;
		st->xerr = sf.xerr;
		st->yerr = sf.yerr;
		d3_printf(" -- centered\n");
	}
	return 0;
}

/* measure instrumental magnitudes for the stars in the stf. */
static int stf_aphot(struct stf *stf, struct ccd_frame *fr,
		     struct wcs *wcs, struct ap_params *ap)
{
//	struct star *star;
	GList *sl, *asl;
	char *filter;
	double scint;
	double rm;
	double imag, imag_err;
	struct ap_params apdef;
	struct star s;

	struct cat_star *cats;
	double x, y;
	double lat, lng, jd=0, fam=0;
	int dodiffam=0;
	int ret;

	if (ap == NULL) {
		ap_params_from_par(&apdef);
		ap = &apdef;
	}
	rm = ceil(ap->r3) + 1;
	memcpy(&ap->exp, &fr->exp, sizeof(struct exp_data));

	filter = stf_find_string(stf, 1, SYM_OBSERVATION, SYM_FILTER);
	if (filter == NULL) {
		err_printf("no filter in stf, aborting aphot\n");
		return -1;
	}
	scint = stf_scint(stf);
	asl = stf_find_glist(stf, 0, SYM_STARS);
	if (asl == NULL) {
		err_printf("no star list in stf, aborting aphot\n");
		return -1;
	}
	if (!stf_am_pars(stf, &lat, &lng, &jd)) {
		dodiffam = 1;
		fam = calculate_airmass(wcs->xref, wcs->yref, jd, lat, lng);
		if (fam <= 0.99)
			dodiffam = 0;
	} else {
		d1_printf("cannot calculate differential AM. lat, long ot time unknown\n");
	}
	for (sl = asl; sl != NULL; sl = g_list_next(sl)) {
		cats = CAT_STAR(sl->data);
		cats_xypix(wcs, cats, &x, &y);
		if (!star_in_frame(fr, x, y, rm)) {
			cats->flags |= CPHOT_INVALID;
			continue;
		}
		if (dodiffam) {
			cats->diffam = calculate_airmass(cats->ra, cats->dec, jd, lat, lng) - fam;
			cats->flags |= INFO_DIFFAM;
		}
		memset(&s, 0, sizeof(struct star));
		s.x = x;
		s.y = y;
		s.xerr = BIG_ERR;
		s.yerr = BIG_ERR;
		s.aph.scint = scint;
		ap->center = P_INT(AP_AUTO_CENTER);
		if (P_INT(AP_AUTO_CENTER)) {
			if (center_star(fr, &s, P_DBL(AP_MAX_CENTER_ERR))) {
				cats->flags |= CPHOT_NOT_FOUND;
				cats->flags &= ~CPHOT_CENTERED;
			} else {
				cats->flags &= ~CPHOT_NOT_FOUND;
				cats->flags |= CPHOT_CENTERED;
			}
		}

//		ret = aperture_photometry(fr, &s, ap, NULL);
		ret = aphot_star(fr, &s, ap, NULL);

		if (ret)
			continue;

		if ((cats->flags & CPHOT_CENTERED) &&
		    !(cats->flags & CAT_ASTROMET) &&
		    P_INT(AP_MOVE_TARGETS)) {
			w_worldpos(wcs, s.x, s.y, &cats->ra, &cats->dec);
			cats->pos[POS_DX] = 0;
			cats->pos[POS_DY] = 0;
		} else {
			cats->pos[POS_DX] = s.x - x;
			cats->pos[POS_DY] = s.y - y;
		}
		cats->pos[POS_X] = s.x;
		cats->pos[POS_Y] = s.y;
		cats->pos[POS_XERR] = s.xerr * s.aph.star_err/s.aph.star;
		cats->pos[POS_YERR] = s.yerr * s.aph.star_err/s.aph.star;
		cats->flags |= INFO_POS;
		if (ret < 0) {
			cats->flags |= CPHOT_INVALID;
		} else {
			if (s.aph.flags & AP_FAINT) {
				cats->flags |= CPHOT_NOT_FOUND;
			}
		}
		imag = s.aph.absmag;
		imag_err = sqrt(sqr(s.aph.magerr) + sqr(s.aph.scint));
		if (s.aph.flags & AP_STAR_SKIP)
			cats->flags |= CPHOT_BADPIX;
		if (s.aph.flags & AP_BURNOUT)
			cats->flags |= CPHOT_BURNED;
		cats->noise[NOISE_SKY] = s.aph.sky_err * s.aph.star_all / s.aph.star;
		cats->noise[NOISE_READ] = s.aph.rd_noise / s.aph.star;
		cats->noise[NOISE_PHOTON] = s.aph.pshot_noise / s.aph.star;
		cats->noise[NOISE_SCINT] = s.aph.scint;
		cats->flags |= INFO_NOISE;
		cats->sky = s.aph.sky;
		cats->flags |= INFO_SKY;

		update_band_by_name(&cats->imags, filter, imag, imag_err);
	}
	return 0;
}


/* create the observation alist from the frame header */
static struct stf * create_obs_alist(struct ccd_frame *fr, struct wcs *wcs)
{
	struct stf *stf = NULL;
	double v, jd;
	char s[80];

	if (P_INT(AP_FORCE_IBAND) ||
	    (fits_get_string(fr, P_STR(FN_FILTER), s, 79) <= 0)) {
		stf = stf_append_string(NULL, SYM_FILTER, P_STR(AP_IBAND_NAME));
	} else {
		trim_lcase_first_word(s);
		stf = stf_append_string(NULL, SYM_FILTER, s);
	}
	if (fits_get_string(fr, P_STR(FN_OBJECT), s, 79) > 0) {
		trim_blanks(s);
		stf_append_string(stf, SYM_OBJECT, s);
	}
	degrees_to_dms_pr(s, wcs->xref / 15.0, 2);
	stf_append_string(stf, SYM_RA, s);
	degrees_to_dms_pr(s, wcs->yref, 1);
	stf_append_string(stf, SYM_DEC, s);
	stf_append_double(stf, SYM_EQUINOX, wcs->equinox);
	if (wcs->jd)
		jd = wcs->jd;
	else
		jd = 0.0;

	if (fits_get_string(fr, P_STR(FN_TELESCOP), s, 79) > 0) {
		trim_blanks(s);
		stf_append_string(stf, SYM_TELESCOPE, s);
	}
	if (fits_get_double(fr, P_STR(FN_APERTURE), &v) > 0) {
		d1_printf("using aperture = %.3f from %s\n",
			  v, P_STR(FN_APERTURE));
		stf_append_double(stf, SYM_APERTURE, v);
	}
	if (fits_get_double(fr, P_STR(FN_EXPTIME), &v) > 0) {
		d1_printf("using exptime = %.3f from %s\n",
			  v, P_STR(FN_EXPTIME));
		stf_append_double(stf, SYM_EXPTIME, v);
		if (jd > 0)
			stf_append_double(stf, SYM_MJD,
					  jd_to_mjd(jd + v / 2/24/3600));
	} else if (jd > 0) {
		stf_append_double(stf, SYM_MJD, jd_to_mjd(jd));
	}

	if (fits_get_double(fr, P_STR(FN_SNSTEMP), &v) > 0) {
		stf_append_double(stf, SYM_SNS_TEMP, v);
	}
	if (wcs->flags & WCS_LOC_VALID) {
		degrees_to_dms_pr(s, wcs->lat, 0);
		stf_append_string(stf, SYM_LATITUDE, s);
		degrees_to_dms_pr(s, wcs->lng, 0);
		stf_append_string(stf, SYM_LONGITUDE, s);
	}
	if (fits_get_double(fr, P_STR(FN_ALTITUDE), &v) > 0) {
		stf_append_double(stf, SYM_ALTITUDE, v);
	}
	if (fits_get_double(fr, P_STR(FN_AIRMASS), &v) > 0) {
		d1_printf("using airmass = %.3f from %s\n",
			  v, P_STR(FN_AIRMASS));
		stf_append_double(stf, SYM_AIRMASS, v);
	} else if ((wcs->flags & WCS_LOC_VALID) && jd > 0.0) {
		/* attempt to calculate airmass from other fields */
		v = calculate_airmass(wcs->xref, wcs->yref, jd, wcs->lat, wcs->lng);
		if (v > 0.0) {
			d1_printf("using calculated airmass %.3f\n", v);
			stf_append_double(stf, SYM_AIRMASS, v);
		}
	}
	if (fits_get_string(fr, P_STR(FN_OBSERVER), s, 79) > 0)
		trim_blanks(s);
		stf_append_string(stf, SYM_OBSERVER, s);
	return stf;
}


/* create a stf from the frame and a list of cat stars. The wcs is assumed valid */
static struct stf * build_stf_from_frame(struct wcs *wcs, GList *sl,
		       struct ccd_frame *fr, struct ap_params *ap)
{
	GList *apsl=NULL;
	struct cat_star *cats;
	struct stf *stf, *st;

	for (; sl != NULL; sl = g_list_next(sl)) {
		cats = CAT_STAR(sl->data);
		if (CATS_TYPE(cats) == CAT_STAR_TYPE_APSTAR ||
		    CATS_TYPE(cats) == CAT_STAR_TYPE_APSTD) {
			apsl = g_list_prepend(apsl, cats);
			cat_star_ref(cats);
		}
	}
	stf = stf_append_list(NULL, SYM_OBSERVATION,
			      create_obs_alist(fr, wcs));
	st = stf_append_double(NULL, SYM_READ, fr->exp.rdnoise);
	stf_append_double(st, SYM_ELADU, fr->exp.scale);
	stf_append_double(st, SYM_FLAT, fr->exp.flat_noise);
	stf_append_list(stf, SYM_NOISE, st);
	if (ap != NULL) {
		int i;
		st = stf_append_double(NULL, SYM_R1, ap->r1);
		stf_append_double(st, SYM_R2, ap->r2);
		stf_append_double(st, SYM_R3, ap->r3);
		i = 0;
		while (sky_methods[i] != NULL) {
			if (i == ap->sky_method)
				stf_append_string(st, SYM_SKY_METHOD, sky_methods[i]);
			i++;
		}
		i = 0;
/*
		while (ap_shapes[i] != NULL) {
			if (i == ap->ap_shape)
				stf_append_string(st, SYM_AP_SHAPE, ap_shapes[i]);
			i++;
		}
		stf_append_list(stf, SYM_AP_PAR, st);
*/
	}
	stf_append_glist(stf, SYM_STARS, apsl);
	return stf;
}

/* keep only the non-invalid, apstd or apstar stars */
static void stf_keep_good_phot(struct stf *stf)
{
	GList *sl, *nsl=NULL;
	struct stf *st;
	struct cat_star *cats;

	st = stf_find(stf, 0, SYM_STARS);
	if (st == NULL || st->next == NULL || !STF_IS_GLIST(st->next))
		return;
	sl = STF_GLIST(st->next);
	for (; sl != NULL; sl = sl->next) {
		cats = CAT_STAR(sl->data);
		if (CATS_TYPE(cats) != CAT_STAR_TYPE_APSTAR &&
		    CATS_TYPE(cats) != CAT_STAR_TYPE_APSTD) {
			cat_star_release(cats);
			continue;
		}
		if (cats->flags & CPHOT_INVALID) {
			cat_star_release(cats);
			continue;
		}
		if ((CATS_TYPE(cats) == CAT_STAR_TYPE_APSTD) &&
		    (cats->flags & CPHOT_NOT_FOUND) &&
		    P_INT(AP_DISCARD_UNLOCATED)) {
			cat_star_release(cats);
			continue;
		}
		nsl = g_list_prepend(nsl, cat_star_dup(cats));
		cat_star_release(cats);
	}
	g_list_free(STF_GLIST(st->next));
	STF_SET_GLIST(st->next, nsl);
}

/* single-field photometric run. Returns a stf for the observation. */
struct stf * run_phot(gpointer window, struct wcs *wcs, struct gui_star_list *gsl,
		       struct ccd_frame *fr)
{
	GSList *apsl=NULL, *sl;
	GList *asl = NULL;
	int all;
	struct cat_star *cats;
	struct gui_star *gs;
//	struct bad_pix_map *bpmap;
//	double scint;
	struct stf *stf;
	struct ap_params apdef;
	struct stf *rcp;
	char *seq;


//	bpmap = g_object_get_data(G_OBJECT(window), "bad_pix_map");

	apsl = gui_stars_of_type(gsl, TYPE_MASK_PHOT);
	all = g_slist_length(apsl);
	if (all == 0) {
		err_printf_sb2(window, "No phot stars\n");
		return  NULL;
	}
//	scint = frame_scint(fr, wcs);

	sl = apsl;
	while (sl != NULL) {
		gs = GUI_STAR(sl->data);
		sl = g_slist_next(sl);
		if (gs->s != NULL)
			cats = CAT_STAR(gs->s);
		else
			continue;
		asl = g_list_prepend(asl, cats);
	}
	g_slist_free(apsl);

	rcp = g_object_get_data(G_OBJECT(window), "recipe");

	ap_params_from_par(&apdef);
	stf = build_stf_from_frame(wcs, asl, fr, &apdef);

	if (rcp != NULL) { 	/* get the sequence and maybe object info from rcp */
		seq = stf_find_string(rcp, 0, SYM_SEQUENCE);
		if (seq != NULL)
			stf_append_string(stf, SYM_SEQUENCE, seq);
		if (stf_find(stf, 1, SYM_OBSERVATION, SYM_OBJECT) == NULL) {
			d3_printf("looking for obj in rcp\n");
			seq = stf_find_string(rcp, 1, SYM_RECIPE, SYM_OBJECT);
			if (seq != NULL) {
				d3_printf("found %s\n", seq);
				rcp = stf_find(stf, 0, SYM_OBSERVATION);
				if (rcp != NULL && rcp->next != NULL
				    && STF_IS_LIST(rcp->next)) {
					d3_printf("appending\n");
					stf_append_string(STF_LIST(rcp->next), SYM_OBJECT, seq);
				}
			}
		}
	}
	stf_aphot(stf, fr, wcs, &apdef);
	stf_keep_good_phot(stf);
	asl = stf_find_glist(stf, 0, SYM_STARS);
	for (; asl != NULL; asl = asl->next) {
		cats = CAT_STAR(asl->data);
		gs = find_window_gs_by_cats_name(window, cats->name);
		if (gs != NULL && (cats->flags & INFO_POS)) {
			gs->x = cats->pos[POS_X];
			gs->y = cats->pos[POS_Y];
		}
	}
	gtk_widget_queue_draw(GTK_WIDGET(window));
	return stf;
}



static void rep_mbds(char *fn, gpointer data, unsigned action)
{
	GList *ofrs = NULL, *sl;
	FILE *repfp = NULL;
	struct mband_dataset *mbds;
	struct o_frame *ofr;
	char qu[1024];

	d3_printf("Report action %x fn:%s\n", action, fn);

	mbds = g_object_get_data(G_OBJECT(data), "temp-mbds");

	g_return_if_fail(mbds != NULL);

	if ((repfp = fopen(fn, "r")) != NULL) { /* file exists */
		snprintf(qu, 1023, "File %s exists\nAppend?", fn);
		if (!modal_yes_no(qu, "gcx: file exists")) {
			fclose(repfp);
			return;
		} else {
			fclose(repfp);
		}
	}

	repfp = fopen(fn, "a");
	if (repfp == NULL) {
		return;
	}
	for (sl = mbds->ofrs; sl != NULL; sl = g_list_next(sl)) {
		ofr = O_FRAME(sl->data);
		if (((action & FMT_MASK) != REP_DATASET) &&
		    (ofr == NULL || ZPSTATE(ofr) <= ZP_FIT_ERR)) {
			continue;
		}
		ofrs = g_list_prepend(ofrs, ofr);
	}
	d3_printf("reporting %d frames\n", g_list_length(ofrs));
	if (ofrs == NULL) {
		error_beep();
		return;
	}
	mbds_report_from_ofrs(mbds, repfp, ofrs, action);
	g_list_free(ofrs);
	fclose(repfp);
}

int stf_centering_stats(struct stf *stf, struct wcs *wcs, double *rms, double *max)
{
	GList *asl;
	double x, y;
	struct cat_star *cats;
	int n = 0;
	double dsq = 0.0, maxe = 0.0, d;

	asl = stf_find_glist(stf, 0, SYM_STARS);

	for (; asl != NULL; asl = asl->next) {
		cats = CAT_STAR(asl->data);
		cats_xypix(wcs, cats, &x, &y);
		if (cats->flags & INFO_POS) {
			n++;
			d = sqr((cats->pos[POS_X] - x)) + sqr((cats->pos[POS_Y] - y));
			dsq += d;
			if (d > maxe)
				maxe = d;
		}
	}
	if (n < 1)
		return 0;
	if (rms) {
		*rms = sqrt(dsq / n);
	}
	if (max) {
		*max = sqrt(maxe);
	}
	return n;
}

/* photometry callback from menu; report goes to stdout */
static void photometry_cb(gpointer window, guint action)
{
	struct gui_star_list *gsl = NULL;
	struct ccd_frame *fr = NULL;
	struct wcs *wcs;
	struct mband_dataset *mbds;
	char *ret = NULL;
	struct stf *stf;
	int n;
	FILE *plfp;

	fr = frame_from_window (window);
	if (fr == NULL) {
		err_printf_sb2(window, "No frame\n");
		return;
	}
	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		err_printf_sb2(window, "No stars\n");
		return;
	}
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if ((wcs == NULL) || ((wcs->wcsset & WCS_VALID) == 0)) {
		err_printf_sb2(window, "Invalid wcs\n");
		return;
	}

//	d3_printf("airmass %f\n", frame_airmass(fr, wcs->xref, wcs->yref));

	switch(action & PHOT_ACTION_MASK) {
	case PHOT_CENTER_STARS:
//		center_phot_stars(window, gsl, fr, P_DBL(AP_MAX_CENTER_ERR));
		stf = run_phot(window, wcs, gsl, fr);
		if (stf == NULL) {
			err_printf_sb2(window, "Phot: %s\n", last_err());
			return;
		} else {
			double r, me;
			if ((n = stf_centering_stats(stf, wcs, &r, &me)) != 0) {
				info_printf_sb2(window, "Centered %d stars. Errors (pixels) rms: %.2f max: %.2f",
						n, r, me);
			}
		}
		stf_free_all(stf);
		break;
	case PHOT_CENTER_PLOT:
		stf = run_phot(window, wcs, gsl, fr);
		if (stf == NULL) {
			err_printf_sb2(window, "Phot: %s\n", last_err());
			return;
		}
		plfp = popen(P_STR(FILE_GNUPLOT), "w");
		if (plfp == NULL) {
			err_printf_sb2(window, "Error running gnuplot (with %s)\n",
				    P_STR(FILE_GNUPLOT));
			return ;
		} else {
			stf_plot_astrom_errors(plfp, stf, wcs);
			pclose(plfp);
		}
		stf_free_all(stf);
		break;
	case PHOT_RUN:
//		center_phot_stars(window, gsl, fr, P_DBL(AP_MAX_CENTER_ERR));
		stf = run_phot(window, wcs, gsl, fr);
		if (stf == NULL) {
			err_printf_sb2(window, "Phot: %s\n", last_err());
			return;
		}
		if ((action & PHOT_OUTPUT_MASK) == PHOT_TO_MBDS) {
			gpointer mbd;
			mbd = g_object_get_data(G_OBJECT(window), "mband_window");
			if (mbd == NULL) {
				act_control_mband(NULL, window);
				mbd = g_object_get_data(G_OBJECT(window), "mband_window");
			}
			stf_to_mband(mbd, stf);
			return;
		}

		mbds = mband_dataset_new();
		d3_printf("mbds: %p\n", mbds);
		mband_dataset_add_stf(mbds, stf);
		d3_printf("mbds has %d frames\n", g_list_length(mbds->ofrs));
		ofr_fit_zpoint(O_FRAME(mbds->ofrs->data), P_DBL(AP_ALPHA), P_DBL(AP_BETA), 1);
		ofr_transform_stars(O_FRAME(mbds->ofrs->data), mbds, 0, 0);
		if (3 * O_FRAME(mbds->ofrs->data)->outliers >
		    O_FRAME(mbds->ofrs->data)->vstars) {
			info_printf(
				"\nWarning: Frame has a large number of outliers (more than 1/3\n"
				"of the number of standard stars). The output of the robust\n"
				"fitter is not reliable in this case. This can be caused\n"
				"by erroneous standard magnitudes, reducing in the wrong band\n"
				"or very bad noise model parameters. \n");
		}
		d3_printf("mbds has %d frames\n", g_list_length(mbds->ofrs));
		switch(action & PHOT_OUTPUT_MASK) {
		case 0:
			if (mbds->ofrs->data != NULL)
				ret = mbds_short_result(O_FRAME(mbds->ofrs->data));
			if (ret != NULL) {
				info_printf_sb2(window, ret);
				free(ret);
			}
			mband_dataset_release(mbds);
			break;
		case PHOT_TO_STDOUT:
			mbds_report_from_ofrs(mbds, stdout, mbds->ofrs, REP_ALL|REP_DATASET);
			mband_dataset_release(mbds);
			break;
		case PHOT_TO_STDOUT_AA:
			mbds_report_from_ofrs(mbds, stdout, mbds->ofrs, REP_TGT|REP_AAVSO);
			mband_dataset_release(mbds);
			break;
		case PHOT_TO_FILE:
			/* bad hack passing the mbds like that. It will cause a leak.*/
			g_object_set_data_full(G_OBJECT(window), "temp-mbds", mbds,
					       (GDestroyNotify) mband_dataset_release);
//			gtk_object_set_data(G_OBJECT(window), "temp-mbds", mbds);
			file_select(window, "Report File", "", rep_mbds, REP_ALL|REP_DATASET);
			break;
		case PHOT_TO_FILE_AA:
			g_object_set_data_full(G_OBJECT(window), "temp-mbds", mbds,
					       (GDestroyNotify) mband_dataset_release);
//			gtk_object_set_data(G_OBJECT(window), "temp-mbds", mbds);
			file_select(window, "Report File", "", rep_mbds, REP_TGT|REP_AAVSO);
			break;
		}
		break;
	default:
		err_printf("unknown action %d in photometry_cb\n", action);
	}
}

void act_phot_center_stars (GtkAction *action, gpointer window)
{
	photometry_cb (window, PHOT_CENTER_STARS);
}

void act_phot_quick (GtkAction *action, gpointer window)
{
	photometry_cb (window, PHOT_RUN);
}

void act_phot_multi_frame (GtkAction *action, gpointer window)
{
	photometry_cb (window, PHOT_RUN | PHOT_TO_MBDS);
}

void act_phot_to_file (GtkAction *action, gpointer window)
{
	photometry_cb (window, PHOT_RUN | PHOT_TO_FILE);
}

void act_phot_to_aavso (GtkAction *action, gpointer window)
{
	photometry_cb (window, PHOT_RUN | PHOT_TO_FILE_AA);
}

void act_phot_to_stdout (GtkAction *action, gpointer window)
{
	photometry_cb (window, PHOT_RUN | PHOT_TO_STDOUT);
}


/* run photometry on the stars in window, writing report to fd
 * a 'short' result (malloced string) is returned (NULL for an error) */
char * phot_to_fd(gpointer window, FILE *fd, int format)
{
	struct gui_star_list *gsl = NULL;
	struct ccd_frame *fr = NULL;
	struct wcs *wcs;
	struct mband_dataset *mbds;
	char *ret = NULL;
	struct stf *stf;

	fr = frame_from_window (window);
	if (fr == NULL) {
		err_printf("No frame\n");
		return NULL;
	}

	gsl = g_object_get_data(G_OBJECT(window), "gui_star_list");
	if (gsl == NULL) {
		err_printf("No phot stars\n");
		return NULL;
	}
	wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if ((wcs == NULL) || ((wcs->wcsset & WCS_VALID) == 0)) {
		err_printf("Invalid wcs\n");
		return NULL;
	}

	stf = run_phot(window, wcs, gsl, fr);
	if (stf == NULL)
		return NULL;
	mbds = mband_dataset_new();
	d3_printf("mbds: %p\n", mbds);
	if (mband_dataset_add_stf(mbds, stf) < 0) {
		err_printf("cannot add stf: aborting\n");
		stf_free_all(stf);
		return NULL;
	}
	d3_printf("mbds has %d frames\n", g_list_length(mbds->ofrs));
	ofr_fit_zpoint(O_FRAME(mbds->ofrs->data), P_DBL(AP_ALPHA), P_DBL(AP_BETA), 1);
	ofr_transform_stars(O_FRAME(mbds->ofrs->data), mbds, 0, 0);
	d3_printf("mbds has %d frames\n", g_list_length(mbds->ofrs));
	mbds_report_from_ofrs(mbds, fd, mbds->ofrs, format);
	if (3 * O_FRAME(mbds->ofrs->data)->outliers >
	    O_FRAME(mbds->ofrs->data)->vstars) {
		info_printf(
			"\nWarning: Frame has a large number of outliers (more than 1/3\n"
			"of the number of standard stars). The output of the robust\n"
			"fitter is not reliable in this case. This can be caused\n"
			"by erroneous standard magnitudes, reducing in the wrong band\n"
			"or very bad noise model parameters. \n");
	}
	if (mbds->ofrs->data != NULL)
		ret = mbds_short_result(O_FRAME(mbds->ofrs->data));
	mband_dataset_release(mbds);
	return ret;
}
