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

/* Multiband reduction plot routines */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "gcx.h"
#include "gcximageview.h"
#include "params.h"
#include "misc.h"
#include "obsdata.h"
#include "multiband.h"
#include "catalogs.h"
#include "recipy.h"
#include "symbols.h"
#include "sourcesdraw.h"
#include "wcs.h"
#include "filegui.h"
#include "gui.h"

#define STD_ERR_CLAMP 30.0
#define RESIDUAL_CLAMP 2.0

/* open a pipe to the plotting process, or a file to hold the plot
   commands. Return -1 for an error, 0 if a file was opened and 1 if a
   pipe was opened */
int open_plot(FILE **fp, char *initial)
{
	FILE *plfp = NULL;
	char *fn;
	char qu[1024];

	if (!P_INT(FILE_PLOT_TO_FILE)) {
		plfp = popen(P_STR(FILE_GNUPLOT), "w");
		if (plfp != NULL && (long)plfp != -1) {
			if (fp)
				*fp = plfp;
			return 1;
		}
		err_printf("Cannot run gnuplot, prompting for file\n");
	}
	fn = modal_file_prompt("Select Plot File", initial);
	if (fn == NULL) {
		err_printf("Aborted\n");
		return -1;
	}
	if ((plfp = fopen(fn, "r")) != NULL) { /* file exists */
		snprintf(qu, 1023, "File %s exists\nOverwrite?", fn);
		if (!modal_yes_no(qu, "gcx: file exists")) {
			fclose(plfp);
			err_printf("Aborted\n");
			return -1;
		} else {
			fclose(plfp);
		}
	}
	plfp = fopen(fn, "w");
	if (plfp == NULL) {
		err_printf("Cannot create file %s (%s)", fn, strerror(errno));
		return -1;
	}
	if (fp)
		*fp = plfp;
	return 0;
}

/* close a plot pipe or file. Should be passed the value returned by open_plot */
int close_plot(FILE *fp, int pop)
{
	if (pop)
		return pclose(fp);
	else
		return fclose(fp);
}

/* commands prepended at the beginning of each plot */
void plot_preamble(FILE *dfp)
{
	g_return_if_fail(dfp != NULL);
	fprintf(dfp, "set key below\n");
	fprintf(dfp, "set term x11\n");
}

/* create a plot of ofr residuals versus magnitude (as a gnuplot file) */
int ofrs_plot_residual_vs_mag(FILE *dfp, GList *ofrs, int weighted)
{
	GList *sl, *osl, *bl, *bsl = NULL;
	struct star_obs *sob;
	struct o_frame *ofr = NULL;
	int n = 0, i = 0;
	long band;
	double v, u;

	g_return_val_if_fail(dfp != NULL, -1);
	plot_preamble(dfp);
	fprintf(dfp, "set xlabel 'Standard magnitude'\n");
	if (weighted) {
		fprintf(dfp, "set ylabel 'Standard errors'\n");
	} else {
		fprintf(dfp, "set ylabel 'Residuals'\n");
	}
//	fprintf(dfp, "set yrange [-1:1]\n");
//	fprintf(dfp, "set title '%s: band:%s mjd=%.5f'\n",
//		ofr->obs->objname, ofr->filter, ofr->mjd);
	fprintf(dfp, "plot ");

	osl = ofrs;
	while (osl != NULL) {
		ofr = O_FRAME(osl->data);
		osl = g_list_next(osl);
		if (ofr->band < 0)
			continue;
		if (g_list_find(bsl, (gpointer)ofr->band) == NULL) {
			bsl = g_list_append(bsl, (gpointer)ofr->band);
			if (i > 0)
				fprintf(dfp, ", ");
			fprintf(dfp, "'-' title '%s'", ofr->trans->bname);
			i++;
		}
	}
	fprintf(dfp, "\n");

	for (bl = bsl; bl != NULL; bl = g_list_next(bl)) {
		band = (long) bl->data;
		osl = ofrs;
		while (osl != NULL) {
			ofr = O_FRAME(osl->data);
			osl = g_list_next(osl);
			if (ofr->band != band)
				continue;
			sl = ofr->sol;
			while(sl != NULL) {
				sob = STAR_OBS(sl->data);
				sl = g_list_next(sl);
				if (CATS_TYPE(sob->cats) != CAT_STAR_TYPE_APSTD)
					continue;
				if (sob->weight <= 0.0001)
					continue;
				n++;
				v = sob->residual * sqrt(sob->nweight);
				u = sob->residual;
				clamp_double(&v, -STD_ERR_CLAMP, STD_ERR_CLAMP);
				clamp_double(&u, -RESIDUAL_CLAMP, RESIDUAL_CLAMP);
				if (weighted)
					fprintf(dfp, "%.5f %.5f %.5f\n", sob->ost->smag[ofr->band],
						v, sob->imagerr);
				else
					fprintf(dfp, "%.5f %.5f %.5f\n", sob->ost->smag[ofr->band],
						u, sob->imagerr);
			}
		}
		fprintf(dfp, "e\n");
	}
	g_list_free(bsl);
//	fprintf(dfp, "pause -1\n");
	return n;
}


/* create a plot of ofr residuals versus magnitude (as a gnuplot file) */
int ofrs_plot_zp_vs_time(FILE *dfp, GList *ofrs)
{
	GList *osl, *bl, *bsl = NULL;
	struct o_frame *ofr = NULL;
	double mjdi = 0.0;
	int n = 0, i = 0;
	long band;
	GList *asfl = NULL, *bnames = NULL;


	osl = ofrs;
	while (osl != NULL) {
		ofr = O_FRAME(osl->data);
		osl = g_list_next(osl);
		if (ofr->band < 0)
			continue;
		mjdi = floor(ofr->mjd);
		break;
	}
	g_return_val_if_fail(dfp != NULL, -1);
	plot_preamble(dfp);
	fprintf(dfp, "set xlabel 'Days from MJD %.1f'\n", mjdi);
	fprintf(dfp, "set ylabel 'Magnitude'\n");
	fprintf(dfp, "set title 'Fitted Frame Zeropoints'\n");
//	fprintf(dfp, "set format x \"%%.3f\"\n");
	fprintf(dfp, "set xtics autofreq\n");
//	fprintf(dfp, "set yrange [-1:1]\n");
//	fprintf(dfp, "set title '%s: band:%s mjd=%.5f'\n",
//		ofr->obs->objname, ofr->filter, ofr->mjd);
	fprintf(dfp, "plot  ");

	osl = ofrs;
	while (osl != NULL) {
		ofr = O_FRAME(osl->data);
		osl = g_list_next(osl);
		if (ofr->band < 0)
			continue;
//		d3_printf("*%d\n", ZPSTATE(ofr));
		if (ZPSTATE(ofr) == ZP_ALL_SKY) {
			asfl = g_list_prepend(asfl, ofr);
		}
		if (g_list_find(bsl, (gpointer)ofr->band) == NULL) {
			bsl = g_list_append(bsl, (gpointer)ofr->band);
			if (i > 0)
				fprintf(dfp, ", ");
			fprintf(dfp, "'-' title '%s' with errorbars ",
				ofr->trans->bname);
			bnames = g_list_append(bnames, ofr->trans->bname);
			i++;
		}
	}
	if (asfl != NULL)
		for (bl = bnames; bl != NULL; bl = g_list_next(bl)) {
			fprintf(dfp, ", '-' title '%s-allsky' with errorbars ",
				(char *)(bl->data));
		}
	fprintf(dfp, "\n");

	for (bl = bsl; bl != NULL; bl = g_list_next(bl)) {
		band = (long) bl->data;
		osl = ofrs;
		while (osl != NULL) {
			ofr = O_FRAME(osl->data);
			osl = g_list_next(osl);
			if (ofr->band != band)
				continue;
			if (ofr->zpointerr >= BIG_ERR)
				continue;
			if (ZPSTATE(ofr) < ZP_FIT_NOCOLOR)
				continue;
			n++;
			fprintf(dfp, "%.5f %.5f %.5f\n", ofr->mjd - mjdi,
				ofr->zpoint, ofr->zpointerr);
			}
		fprintf(dfp, "e\n");
	}
	if (asfl != NULL)
		for (bl = bsl; bl != NULL; bl = g_list_next(bl)) {
			band = (long) bl->data;
			osl = asfl;
			while (osl != NULL) {
				ofr = O_FRAME(osl->data);
				osl = g_list_next(osl);
				if (ofr->band != band)
					continue;
				if (ofr->zpointerr >= BIG_ERR)
					continue;
				n++;
				fprintf(dfp, "%.5f %.5f %.5f\n", ofr->mjd - mjdi,
					ofr->zpoint, ofr->zpointerr);
			}
			fprintf(dfp, "e\n");
		}
//	fprintf(dfp, "pause -1\n");
	g_list_free(bsl);
	g_list_free(asfl);
	return n;
}


/* create a plot of ofr residuals versus magnitude (as a gnuplot file) */
int ofrs_plot_zp_vs_am(FILE *dfp, GList *ofrs)
{
	GList *osl, *bl, *bsl = NULL;
	struct o_frame *ofr = NULL;
	int n = 0, i = 0;
	long band;
	GList *asfl = NULL, *bnames = NULL;

	g_return_val_if_fail(dfp != NULL, -1);
	plot_preamble(dfp);
	fprintf(dfp, "set xlabel 'Airmass'\n");
	fprintf(dfp, "set ylabel 'Magnitude'\n");
	fprintf(dfp, "set title 'Fitted Frame Zeropoints'\n");
//	fprintf(dfp, "set yrange [-1:1]\n");
//	fprintf(dfp, "set title '%s: band:%s mjd=%.5f'\n",
//		ofr->obs->objname, ofr->filter, ofr->mjd);
	fprintf(dfp, "plot  ");

	osl = ofrs;
	while (osl != NULL) {
		ofr = O_FRAME(osl->data);
		osl = g_list_next(osl);
		if (ofr->band < 0)
			continue;
//		d3_printf("*%d\n", ZPSTATE(ofr));
		if (ZPSTATE(ofr) == ZP_ALL_SKY) {
			asfl = g_list_prepend(asfl, ofr);
		}
		if (g_list_find(bsl, (gpointer)ofr->band) == NULL) {
			bsl = g_list_append(bsl, (gpointer)ofr->band);
			if (i > 0)
				fprintf(dfp, ", ");
			fprintf(dfp, "'-' title '%s' with errorbars ",
				ofr->trans->bname);
			bnames = g_list_append(bnames, ofr->trans->bname);
			i++;
		}
	}
	if (asfl != NULL)
		for (bl = bnames; bl != NULL; bl = g_list_next(bl)) {
			fprintf(dfp, ", '-' title '%s-allsky' with errorbars ",
				(char *)(bl->data));
		}
	fprintf(dfp, "\n");

	for (bl = bsl; bl != NULL; bl = g_list_next(bl)) {
		band = (long) bl->data;
		osl = ofrs;
		while (osl != NULL) {
			ofr = O_FRAME(osl->data);
			osl = g_list_next(osl);
			if (ofr->band != band)
				continue;
			if (ofr->zpointerr >= BIG_ERR)
				continue;
			if (ZPSTATE(ofr) < ZP_FIT_NOCOLOR)
				continue;
			n++;
			fprintf(dfp, "%.5f %.5f %.5f\n", ofr->airmass,
				ofr->zpoint, ofr->zpointerr);
			}
		fprintf(dfp, "e\n");
	}
	if (asfl != NULL)
		for (bl = bsl; bl != NULL; bl = g_list_next(bl)) {
			band = (long) bl->data;
			osl = asfl;
			while (osl != NULL) {
				ofr = O_FRAME(osl->data);
				osl = g_list_next(osl);
				if (ofr->band != band)
					continue;
				if (ofr->zpointerr >= BIG_ERR)
					continue;
				n++;
				fprintf(dfp, "%.5f %.5f %.5f\n", ofr->airmass,
					ofr->zpoint, ofr->zpointerr);
			}
			fprintf(dfp, "e\n");
		}
//	fprintf(dfp, "pause -1\n");
	g_list_free(bsl);
	g_list_free(asfl);
	return n;
}


/* create a plot of ofr residuals versus color (as a gnuplot file) */
int ofrs_plot_residual_vs_col(struct mband_dataset *mbds, FILE *dfp,
			      int band, GList *ofrs, int weighted)
{
	GList *sl, *osl;
	struct star_obs *sob;
	struct o_frame *ofr = NULL;
	int n = 0, i = 0;
	double v, u;

//	d3_printf("plot: band is %d\n", band);
	g_return_val_if_fail(dfp != NULL, -1);
	g_return_val_if_fail(mbds != NULL, -1);
	g_return_val_if_fail(band >= 0, -1);
	g_return_val_if_fail(mbds->trans[band].c1 >= 0, -1);
	g_return_val_if_fail(mbds->trans[band].c2 >= 0, -1);

	plot_preamble(dfp);
	fprintf(dfp, "set xlabel '%s-%s'\n", mbds->trans[mbds->trans[band].c1].bname,
		mbds->trans[mbds->trans[band].c2].bname);
	if (weighted) {
		fprintf(dfp, "set ylabel 'Standard errors'\n");
	} else {
		fprintf(dfp, "set ylabel 'Residuals'\n");
	}
//	fprintf(dfp, "set ylabel '%s zeropoint fit weighted residuals'\n", mbds->bnames[band]);
//	fprintf(dfp, "set yrange [-1:1]\n");
	fprintf(dfp, "set title 'Transformation: %s = %s_i + %s_0 + %.3f * (%s - %s)'\n",
		mbds->trans[band].bname, mbds->trans[band].bname,
		mbds->trans[band].bname, mbds->trans[band].k,
		mbds->trans[mbds->trans[band].c1].bname, mbds->trans[mbds->trans[band].c2].bname);
//	fprintf(dfp, "set pointsize 1.5\n");
	fprintf(dfp, "plot ");

	osl = ofrs;
	while (osl != NULL) {
		ofr = O_FRAME(osl->data);
		osl = g_list_next(osl);
		if (ofr->band != band)
			continue;
		if (i > 0)
			fprintf(dfp, ", ");
//		fprintf(dfp, "'-' title '%s(%s)'",
//			ofr->obs->objname, ofr->xfilter);
		fprintf(dfp, "'-' notitle ");
		i++;
	}
	fprintf(dfp, "\n");

	osl = ofrs;
	while (osl != NULL) {
		ofr = O_FRAME(osl->data);
		osl = g_list_next(osl);
		if (ofr->band != band)
			continue;
		if (ofr->tweight < 0.0000000001)
			continue;
		sl = ofr->sol;
		while(sl != NULL) {
			sob = STAR_OBS(sl->data);
			sl = g_list_next(sl);
			if (CATS_TYPE(sob->cats) != CAT_STAR_TYPE_APSTD)
				continue;
			if (sob->weight <= 0.00001)
				continue;
			n++;
			if (sob->ost->smagerr[mbds->trans[band].c1] < 9
			    && sob->ost->smagerr[mbds->trans[band].c2] < 9) {
				v = sob->residual * sqrt(sob->nweight);
				u = sob->residual;
				clamp_double(&v, -STD_ERR_CLAMP, STD_ERR_CLAMP);
				clamp_double(&u, -RESIDUAL_CLAMP, RESIDUAL_CLAMP);
				if (weighted)
					fprintf(dfp, "%.5f %.5f %.5f\n",
						sob->ost->smag[mbds->trans[band].c1]
						- sob->ost->smag[mbds->trans[band].c2],
						v, sob->imagerr);
				else
					fprintf(dfp, "%.5f %.5f %.5f\n",
						sob->ost->smag[mbds->trans[band].c1]
						- sob->ost->smag[mbds->trans[band].c2],
						u, sob->imagerr);
			}
		}
		fprintf(dfp, "e\n");
	}
//	fprintf(dfp, "pause -1\n");
	return n;
}

static int sol_stats(GList *ssol, double *avg, double *sigma, double *min,
		     double *max, double *merr)
{
	struct star_obs *sol;
	double esum = 0.0;
	double sum = 0.0;
	double sumsq = 0.0;
	double mi = HUGE;
	double ma = -HUGE;
	int n = 0;
	GList *sl;
	double m, me;
	int band;

	for (sl = ssol; sl != NULL; sl = sl->next) {
		sol = STAR_OBS(sl->data);
		if (sol->ofr->band < 0)
			continue;
		if (sol->ofr->skip)
			continue;
		band = sol->ofr->band;
		if (sol->imagerr >= BIG_ERR)
			continue;
		if (sol->ofr->zpstate == ZP_FIT_ERR)
			continue;
		if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTAR && sol->err < BIG_ERR) {
			m = sol->mag; me = sol->err;
		} else if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTAR) {
			m = sol->imag; me = sol->imagerr;
		} else if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTD
			   && sol->ofr->zpstate >= ZP_ALL_SKY) {
			m = sol->ost->smag[band] + sol->residual;
			me = sqrt( sqr(sol->imagerr) + sqr(sol->ost->smagerr[band]));
		} else if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTD) {
			m = sol->imag; me = sol->imagerr;
		} else
			continue;
		esum += me;
		sum += m;
		if (mi > m)
			mi = m;
		if (ma < m)
			ma = m;
		sumsq += sqr(m);
		n++;
	}
	if (n > 0) {
		if (avg)
			*avg = sum/n;
		if (sigma)
			*sigma = sqrt(sumsq/n - sqr(sum/n));
		if (merr)
			*merr = esum/n;
	}
	if (min)
		*min = mi;
	if (max)
		*max = ma;
	return n;
}


/* plot stars vs time */
int plot_star_mag_vs_time(FILE *dfp, GList *sobs)
{
	GList *osl, *sl = NULL;
	struct o_frame *ofr = NULL;
	struct star_obs *sob = NULL, *sol;
	double jdi = 0.0;
	int n = 0, i = 0;
	int band;

	if (sobs == NULL)
		return -1;
	ofr = STAR_OBS(sobs->data)->ofr;
	band = ofr->band;
	jdi = floor(mjd_to_jd(ofr->mjd));

	g_return_val_if_fail(dfp != NULL, -1);
	plot_preamble(dfp);
	fprintf(dfp, "set xlabel 'Days from JD %.1f'\n", jdi);
//	fprintf(dfp, "set ylabel 'Magnitude'\n");
//	fprintf(dfp, "set title 'Fitted Frame Zeropoints'\n");
//	fprintf(dfp, "set format x \"%%.3f\"\n");
	fprintf(dfp, "set xtics autofreq\n");
	fprintf(dfp, "set yrange [:] reverse\n");
//	fprintf(dfp, "set mouse\n");
//	fprintf(dfp, "set title '%s: band:%s mjd=%.5f'\n",
//		ofr->obs->objname, ofr->filter, ofr->mjd);
	fprintf(dfp, "plot  ");

	osl = sobs;
	while (osl != NULL) {
		int ns;
		double min, max, avg=0.0, sigma=0.0, merr=BIG_ERR;
		sob = STAR_OBS(osl->data);
		ofr = sob->ofr;
		osl = g_list_next(osl);
		ns = sol_stats(sob->ost->sol, &avg, &sigma, &min, &max, &merr);
		if (i > 0)
			fprintf(dfp, ", ");
		if (ns == 0)
			fprintf(dfp, "'-' title '%s(%s)' with errorbars ",
				sob->cats->name, ofr->trans->bname);
		else
			fprintf(dfp, "'-' title '%s(%s) avg:%.3f sd:%.3f min:%.3f"
				" max:%.3f me:%.3f sd/me:%4.1f' with errorbars ",
				sob->cats->name, ofr->trans->bname,
				avg, sigma, min, max, merr, (merr > 0) ? sigma/merr : 99.999);
		i++;
	}
	fprintf(dfp, "\n");

	osl = sobs;
	while (osl != NULL) {
		sob = STAR_OBS(osl->data);
		ofr = sob->ofr;
		osl = g_list_next(osl);
		if (ofr->band != band)
			continue;

		for (sl = sob->ost->sol; sl != NULL; sl = sl->next) {
			sol = STAR_OBS(sl->data);
			if (sol->ofr->skip)
				continue;
			if (sol->ofr->band != band)
				continue;
			if (sol->imagerr >= BIG_ERR)
				continue;
			if (sol->ofr->zpstate == ZP_FIT_ERR)
				continue;
			n++;
			if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTAR
			    && sol->err < BIG_ERR)
				fprintf(dfp, "%.5f %.5f %.5f\n", mjd_to_jd(sol->ofr->mjd) - jdi,
					sol->mag, sol->err);
			else if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTAR)
				fprintf(dfp, "%.5f %.5f %.5f\n", mjd_to_jd(sol->ofr->mjd) - jdi,
					sol->imag, sol->imagerr);
			else if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTD
				 && sol->ofr->zpstate >= ZP_ALL_SKY)
				fprintf(dfp, "%.5f %.5f %.5f\n", mjd_to_jd(sol->ofr->mjd) - jdi,
					sol->ost->smag[band] + sol->residual,
					sqrt( sqr(sol->imagerr) +
						sqr(sol->ost->smagerr[band])));
			else if (CATS_TYPE(sol->cats) == CAT_STAR_TYPE_APSTD)
				fprintf(dfp, "%.5f %.5f %.5f\n", mjd_to_jd(sol->ofr->mjd) - jdi,
					sol->imag, sol->imagerr);
		}
		fprintf(dfp, "e\n");
	}
	return n;
}

/* generate a vector plot of astrometric errors */
int stf_plot_astrom_errors(FILE *dfp, struct stf *stf, struct wcs *wcs)
{
	GList *asl;
	double x, y;
	struct cat_star *cats;
	int n = 0;
	double r, me;

	n = stf_centering_stats(stf, wcs, &r, &me);

	if (n < 1)
		return 0;

	fprintf(dfp, "set xlabel 'pixels'\n");
	fprintf(dfp, "set ylabel 'pixels'\n");
	fprintf(dfp, "set nokey\n");
	fprintf(dfp, "set title 'Frame vs catalog positions (%.0fX) rms:%.2f, max:%.2f'\n",
		P_DBL(WCS_PLOT_ERR_SCALE), r, me);
	fprintf(dfp, "plot '-' with vector\n");

	asl = stf_find_glist(stf, 0, SYM_STARS);
	n = 0;
	for (; asl != NULL; asl = asl->next) {
		cats = CAT_STAR(asl->data);
		cats_xypix(wcs, cats, &x, &y);
		if (cats->flags & INFO_POS) {
			n++;
			fprintf (dfp, "%.3f %.3f %.3f %.3f\n",
				 x, -y, P_DBL(WCS_PLOT_ERR_SCALE) * (cats->pos[POS_X] - x),
				 -P_DBL(WCS_PLOT_ERR_SCALE) * (cats->pos[POS_Y] - y) );
		}
	}
	return n;
}
