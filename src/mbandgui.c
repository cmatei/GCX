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

/* Multiband reduction gui and menus */

/* we hold references to o_frames, sobs and o_stars without them being ref_counted.
   so while the clists in the dialog are alive, the mbds must not be destroyed */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "sourcesdraw.h"
#include "params.h"
#include "wcs.h"
#include "cameragui.h"
#include "filegui.h"
#include "interface.h"
#include "misc.h"
#include "obsdata.h"
#include "multiband.h"
#include "plots.h"
#include "recipy.h"
#include "symbols.h"


static void bands_list_update_vals(GtkWidget *dialog, struct mband_dataset *mbds);
static void mb_rebuild_sob_list(gpointer dialog, GList *sol);
static void sob_list_update_vals(GtkWidget *sob_list);
static void mb_rebuild_ofr_list(gpointer dialog);
static void mb_rebuild_bands_list(gpointer dialog);

#define PLOT_RES_SM 1
#define PLOT_RES_COL 2
#define PLOT_WEIGHTED 0x10
#define PLOT_ZP_AIRMASS 3
#define PLOT_ZP_TIME 4
#define PLOT_STAR 5

#define FIT_ZPOINTS 1
#define FIT_ZP_WTRANS 3
#define FIT_TRANS 2
#define FIT_ALL_SKY 4

#define SEL_ALL 1
#define UNSEL_ALL 2
#define HIDE_SEL 3
#define UNHIDE_ALL 4

#define ERR_SIZE 1024
/* print the error string and save it to storage */
static int mbds_printf(gpointer window, const char *fmt, ...)
{
	va_list ap, ap2;
	int ret;
	char err_string[ERR_SIZE+1];
	GtkWidget *label;
#ifdef __va_copy
	__va_copy(ap2, ap);
#else
	ap2 = ap;
#endif
	va_start(ap, fmt);
	va_start(ap2, fmt);
	ret = vsnprintf(err_string, ERR_SIZE, fmt, ap2);
	if (ret > 0 && err_string[ret-1] == '\n')
		err_string[ret-1] = 0;
	label = g_object_get_data(G_OBJECT(window), "status_label");
	if (label != NULL)
		gtk_label_set_text(GTK_LABEL(label), err_string);
	va_end(ap);
	return ret;
}

static int fit_progress(char *msg, void *data)
{
	mbds_printf(data, "%s", msg);
	while (gtk_events_pending ())
		gtk_main_iteration ();
	return 0;
}

static void rep_file_cb(char *fn, gpointer data, unsigned action)
{
	GtkWidget *ofr_list;
	GtkTreeModel *ofr_store;
	GList *ofrs = NULL, *sl;
	FILE *repfp = NULL;
	struct mband_dataset *mbds;
	struct o_frame *ofr;
	char qu[1024];
	int ret;
	GList *gl, *glh;
	GtkTreeSelection *sel;
	GtkTreeIter iter;

	d3_printf("Report action %x fn:%s\n", action, fn);

	mbds = g_object_get_data(G_OBJECT(data), "mbds");
	g_return_if_fail(mbds != NULL);

	ofr_list = g_object_get_data(G_OBJECT(data), "ofr_list");
	g_return_if_fail(ofr_list != NULL);

	ofr_store = gtk_tree_view_get_model(GTK_TREE_VIEW(ofr_list));

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
		mbds_printf(data, "Cannot open/create file %s (%s)", fn, strerror(errno));
		return;
	}

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ofr_list));
	glh = gtk_tree_selection_get_selected_rows(sel, NULL);
	if (glh != NULL) {
		for (gl = g_list_first(glh); gl != NULL; gl = g_list_next(gl)) {
			if (!gtk_tree_model_get_iter(ofr_store, &iter, gl->data))
				continue;

			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);
			if (ofr->skip)
				continue;

			if (((action & FMT_MASK) != REP_DATASET) &&
			    (ofr == NULL || ZPSTATE(ofr) <= ZP_FIT_ERR)) {
//				d3_printf("skipping frame with 0 fitted stars\n");
				continue;
			}
			ofrs = g_list_prepend(ofrs, ofr);
		}
		ofrs = g_list_reverse(ofrs);

		g_list_foreach(glh, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(glh);
	} else {
		for (sl = mbds->ofrs; sl != NULL; sl = g_list_next(sl)) {
			ofr = O_FRAME(sl->data);
			if (ofr->skip)
				continue;
			if (((action & FMT_MASK) != REP_DATASET) &&
			    (ofr == NULL || ZPSTATE(ofr) <= ZP_FIT_ERR)) {
				continue;
			}
			ofrs = g_list_prepend(ofrs, ofr);
		}
		ofrs = g_list_reverse(ofrs);
	}

	if (ofrs == NULL) {
		error_beep();
		mbds_printf(data, "Nothing to report. Only fitted frames will be reported.\n");
		return;
	}

	ret = mbds_report_from_ofrs(mbds, repfp, ofrs, action);
	if (ret < 0) {
		mbds_printf(data, "%s", last_err());
	} else {
		mbds_printf(data, "%d frame(s), %d star(s) reported to %s",
			    g_list_length(ofrs), ret, fn);
	}

	g_list_free(ofrs);
	fclose(repfp);
}

static void rep_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	file_select(data, "Report File", "", rep_file_cb, action);
}

void mband_save_dataset_action(GtkAction *action, gpointer data)
{
	rep_cb(data, REP_DATASET | REP_ALL, NULL);
}

void mband_close_dataset_action(GtkAction *action, gpointer data)
{
	struct mband_dataset *mbds;
	GtkWidget *list;

	mbds = g_object_get_data(G_OBJECT(data), "mbds");

	if (mbds && !modal_yes_no("Closing the dataset will "
				  "discard fit information\n"
				  "Are you sure?", "Close dataset?"))
		return;

	list = g_object_get_data(G_OBJECT(data), "ofr_list");
	if (list) {
		g_object_set_data(G_OBJECT(data), "ofr_list", NULL);
	}
	list = g_object_get_data(G_OBJECT(data), "sob_list");
	if (list) {
		g_object_set_data(G_OBJECT(data), "sob_list", NULL);
	}
	list = g_object_get_data(G_OBJECT(data), "bands_list");
	if (list) {
		g_object_set_data(G_OBJECT(data), "bands_list", NULL);
	}
	if (mbds)
		g_object_set_data(G_OBJECT(data), "mbds", NULL);
}

void mband_report_action(GtkAction *action, gpointer data)
{
	rep_cb(data, REP_TGT | REP_AAVSO, NULL);
}

static void select_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *list;
	GList *gl, *glh;
	struct mband_dataset *mbds;
	int n;
	struct o_frame *ofr;
	GtkTreeIter iter;
	GtkTreeModel *ofr_store;
	GtkTreeSelection *sel;

	switch(action) {
	case SEL_ALL:
		list = g_object_get_data(G_OBJECT(data), "ofr_list");
		if (list)
			gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)));

		list = g_object_get_data(G_OBJECT(data), "sob_list");
		if (list)
			gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)));
		break;

	case UNSEL_ALL:
		list = g_object_get_data(G_OBJECT(data), "ofr_list");
		if (list)
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)));

		list = g_object_get_data(G_OBJECT(data), "sob_list");
		if (list)
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)));
		break;

	case HIDE_SEL:
		n = 0;
		list = g_object_get_data(G_OBJECT(data), "ofr_list");
		if (list == NULL)
			return;

		ofr_store = gtk_tree_view_get_model(GTK_TREE_VIEW(list));

		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
		glh = gtk_tree_selection_get_selected_rows(sel, NULL);

		for (gl = g_list_first(glh); gl != NULL; gl = g_list_next(gl)) {
			if (!gtk_tree_model_get_iter(ofr_store, &iter, gl->data))
				continue;

			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);

			ofr->skip = 1;
			n++;
		}

		g_list_foreach(glh, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(glh);

		if (n > 0) {
			mb_rebuild_ofr_list(data);
			mbds_printf(data, "%d frames hidden: they will not be processed or saved\n", n);
		}
		break;

	case UNHIDE_ALL:
		n = 0;
		mbds = g_object_get_data(G_OBJECT(data), "mbds");
		g_return_if_fail(mbds != NULL);

		for (gl = mbds->ofrs; gl != NULL; gl = gl->next) {
			ofr = gl->data;
			if (ofr->skip) {
				ofr->skip = 0;
				n++;
			}
		}

		if (n > 0) {
			mb_rebuild_ofr_list(data);
			mbds_printf(data, "%d frames restored\n", n);
		}
		break;
	}
}

void mband_select_all_action (GtkAction *action, gpointer data)
{
	select_cb (data, SEL_ALL, NULL);
}

void mband_unselect_all_action (GtkAction *action, gpointer data)
{
	select_cb (data, UNSEL_ALL, NULL);
}

void mband_hide_action (GtkAction *action, gpointer data)
{
	select_cb (data, HIDE_SEL, NULL);
}

void mband_unhide_action (GtkAction *action, gpointer data)
{
	select_cb (data, UNHIDE_ALL, NULL);
}



static void plot_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *ofr_list;
	GList *gl, *glh;
	GList *ofrs = NULL, *sl;
	struct mband_dataset *mbds;
	struct o_frame *ofr;
	struct star_obs *sob;
	int band = -1;
	FILE *plfp = NULL;
	int pop;
	GtkTreeModel *ofr_store;
	GtkTreeSelection *sel;
	GtkTreeIter iter;


	mbds = g_object_get_data(G_OBJECT(data), "mbds");
	g_return_if_fail(mbds != NULL);

	if (action == PLOT_STAR) {
		/* FIXME: variable reuse is not pretty */
		ofr_list = g_object_get_data(G_OBJECT(data), "sob_list");
		if (ofr_list == NULL) {
			error_beep();
			mbds_printf(data, "No star selected\n");
			return;
		}

		ofr_store = gtk_tree_view_get_model(GTK_TREE_VIEW(ofr_list));

		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ofr_list));
		glh = gtk_tree_selection_get_selected_rows(sel, NULL);
		if (glh == NULL) {
			error_beep();
			mbds_printf(data, "No star selected\n");
			return;
		}

		for (gl = g_list_first(glh); gl != NULL; gl = g_list_next(gl)) {
			if (!gtk_tree_model_get_iter(ofr_store, &iter, gl->data))
				continue;

			gtk_tree_model_get(ofr_store, &iter, 0, &sob, -1);

			ofr = sob->ofr;
			if (ofr == NULL || ofr->skip || ZPSTATE(ofr) <= ZP_FIT_ERR)
				continue;

			ofrs = g_list_prepend(ofrs, sob);
		}

		g_list_foreach(glh, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(glh);

		pop = open_plot(&plfp, NULL);
		if (pop < 0) {
			mbds_printf(data, last_err());
			return;
		}
		plot_star_mag_vs_time(plfp, ofrs);
		close_plot(plfp, pop);
		return;
	}


	ofr_list = g_object_get_data(G_OBJECT(data), "ofr_list");
	g_return_if_fail(ofr_list != NULL);
	ofr_store = gtk_tree_view_get_model(GTK_TREE_VIEW(ofr_list));

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ofr_list));
	glh = gtk_tree_selection_get_selected_rows(sel, NULL);
	if (glh != NULL) {
		for (gl = g_list_first(glh); gl != NULL; gl = g_list_next(gl)) {
			if (!gtk_tree_model_get_iter(ofr_store, &iter, gl->data))
				continue;

			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);
			if (ofr == NULL || ofr->skip || ZPSTATE(ofr) <= ZP_FIT_ERR)
				continue;

			ofrs = g_list_prepend(ofrs, ofr);
		}
		ofrs = g_list_reverse(ofrs);

		g_list_foreach(glh, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(glh);
	} else {
		for (sl = mbds->ofrs; sl != NULL; sl = g_list_next(sl)) {
			ofr = O_FRAME(sl->data);

			if (ofr == NULL || ofr->skip || ZPSTATE(ofr) <= ZP_FIT_ERR)
				continue;

			ofrs = g_list_prepend(ofrs, ofr);
		}
		ofrs = g_list_reverse(ofrs);
	}

	if (ofrs == NULL) {
		error_beep();
		mbds_printf(data, "Nothing to plot. Only fitted frames will be plotted\n");
		return;
	}

	for (sl = ofrs; sl != NULL; sl = g_list_next(sl)) {
		if (O_FRAME(sl->data)->band >= 0) {
			band = O_FRAME(sl->data)->band;
			break;
		}
	}

	switch(action & 0x0f) {
	case PLOT_RES_SM:
		pop = open_plot(&plfp, NULL);
		if (pop < 0) {
			mbds_printf(data, last_err());
			break;
		}
		ofrs_plot_residual_vs_mag(plfp, ofrs, (action & PLOT_WEIGHTED) != 0);
		close_plot(plfp, pop);
		break;

	case PLOT_RES_COL:
		if (band < 0) {
			mbds_printf(data, "None of the selected frames has a valid band\n");
			break;
		}
		pop = open_plot(&plfp, NULL);
		if (pop < 0) {
			mbds_printf(data, last_err());
			break;
		}
		ofrs_plot_residual_vs_col(mbds, plfp, band,
					  ofrs, (action & PLOT_WEIGHTED) != 0);
		close_plot(plfp, pop);
		break;

	case PLOT_ZP_TIME:
		pop = open_plot(&plfp, NULL);
		if (pop < 0) {
			mbds_printf(data, last_err());
			break;
		}
		ofrs_plot_zp_vs_time(plfp, ofrs);
		close_plot(plfp, pop);
		break;

	case PLOT_ZP_AIRMASS:
		pop = open_plot(&plfp, NULL);
		if (pop < 0) {
			mbds_printf(data, last_err());
			break;
		}
		ofrs_plot_zp_vs_am(plfp, ofrs);
		close_plot(plfp, pop);
		break;
	}

	g_list_free(ofrs);
}

void mband_plot_resmag_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_RES_SM, NULL);
}

void mband_plot_rescol_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_RES_COL, NULL);
}

void mband_plot_errmag_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_RES_SM | PLOT_WEIGHTED, NULL);
}

void mband_plot_errcol_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_RES_COL | PLOT_WEIGHTED, NULL);
}

void mband_plot_zpairmass_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_ZP_AIRMASS, NULL);
}

void mband_plot_zptime_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_ZP_TIME, NULL);
}

void mband_plot_magtime_action(GtkAction *action, gpointer data)
{
	plot_cb (data, PLOT_STAR, NULL);
}

static void ofr_store_set_row_vals(GtkListStore *ofr_store, GtkTreeIter *iter, struct o_frame *ofr)
{
	char c[50 * 10];
	int i;
	char *obj;
	char *states[] = {"Not Fitted", "Err", "All-sky", "Diff", "ZP Only", "OK",
			  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			  NULL, NULL, NULL};

	obj = stf_find_string(ofr->stf, 1, SYM_OBSERVATION, SYM_OBJECT);
	if (obj)
		snprintf(c, 40, "%s", obj);
	else
		c[0] = 0;
	snprintf(c+50, 40, "%s", ofr->trans->bname);
	if (ofr->zpstate >= ZP_ALL_SKY) {
		snprintf(c+150, 40, "%.3f", ofr->zpoint);
		snprintf(c+200, 40, "%.3f", ofr->zpointerr);
		snprintf(c+350, 40, "%.2f", ofr->me1);
	} else {
		c[150] = 0;
		c[200] = 0;
		c[350] = 0;
	}
	snprintf(c+100, 40, "%s%s", states[ofr->zpstate & ZP_STATE_M],
		 ofr->as_zp_valid ? "-AV" : "");
	if ((ofr->zpstate & ZP_STATE_M) > ZP_NOT_FITTED) {
		snprintf(c+250, 40, "%d", ofr->vstars);
		snprintf(c+300, 40, "%d", ofr->outliers);
	} else {
		c[250] = 0;
		c[300] = 0;
	}
	snprintf(c+400, 40, "%.2f", ofr->airmass);
	snprintf(c+450, 40, "%.5f", ofr->mjd);

	gtk_list_store_set(ofr_store, iter, 0, ofr, -1);
	for (i = 0; i < 10; i++) {
		gtk_list_store_set(ofr_store, iter, i + 1, c + 50 * i, -1);
	}
}

static void fit_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *ofr_list, *sob_list;
	GList *gl, *glh;
	GList *ofrs = NULL, *sl;
	struct mband_dataset *mbds;
	struct o_frame *ofr;
	GtkTreeSelection *sel;
	GtkTreeModel *ofr_store;
	GtkTreeIter iter;

	ofr_list = g_object_get_data(G_OBJECT(data), "ofr_list");
	g_return_if_fail(ofr_list != NULL);
	ofr_store = gtk_tree_view_get_model(GTK_TREE_VIEW(ofr_list));

	sob_list = g_object_get_data(G_OBJECT(data), "sob_list");
//	g_return_if_fail(sob_list != NULL);

	mbds = g_object_get_data(G_OBJECT(data), "mbds");
	g_return_if_fail(mbds != NULL);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ofr_list));
	glh = gtk_tree_selection_get_selected_rows(sel, NULL);
	if (glh != NULL) {
		for (gl = g_list_first(glh); gl != NULL; gl = g_list_next(gl)) {
			if (!gtk_tree_model_get_iter(ofr_store, &iter, gl->data))
				continue;

			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);
			if (ofr == NULL)
				continue;

			d4_printf("ofr = %08x\n", ofr);
			ofrs = g_list_prepend(ofrs, ofr);
		}

		g_list_foreach(glh, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(glh);
	} else {
		for (gl = mbds->ofrs; gl != NULL; gl = g_list_next(gl)) {
			ofr = O_FRAME(gl->data);
			if (ofr->skip)
				continue;
			ofrs = g_list_prepend(ofrs, ofr);
		}
//		ofrs = g_list_copy(mbds->ofrs);
	}

	switch (action) {
	case FIT_ZPOINTS:
		fit_progress("Fitting zero points with no color", data);
		for (sl = ofrs; sl != NULL; sl = g_list_next(sl)) {
			ofr = O_FRAME(sl->data);
			if (ofr->band < 0)
				continue;
			ofr->trans->k = 0.0;
			ofr->trans->kerr = BIG_ERR;
			ofr_fit_zpoint(ofr, P_DBL(AP_ALPHA), P_DBL(AP_BETA), 1);
		}
		fit_progress("Transforming stars", data);
		for (sl = ofrs; sl != NULL; sl = g_list_next(sl)) {
			ofr = O_FRAME(sl->data);
			if (ofr->band < 0)
				continue;
			ofr_transform_stars(ofr, mbds, 0, 0);
		}
		bands_list_update_vals(data, mbds);
		fit_progress("Done", data);
		break;

	case FIT_ZP_WTRANS:
		fit_progress("Fitting zero points with current color coefficients", data);
		for (sl = ofrs; sl != NULL; sl = g_list_next(sl)) {
			ofr = O_FRAME(sl->data);
			if (ofr->band < 0)
				continue;
			ofr_fit_zpoint(ofr, P_DBL(AP_ALPHA), P_DBL(AP_BETA), 1);
		}
		fit_progress("Transforming stars", data);
		for (sl = ofrs; sl != NULL; sl = g_list_next(sl)) {
			ofr = O_FRAME(sl->data);
			if (ofr->band < 0)
				continue;
			ofr_transform_stars(ofr, mbds, 0, 0);
		}
		fit_progress("Done", data);
		break;

	case FIT_TRANS:
		mbds_fit_all(ofrs, fit_progress, data);
		fit_progress("Transforming stars", data);
		mbds_transform_all(mbds, ofrs, 0);
		bands_list_update_vals(data, mbds);
		fit_progress("Done", data);
		break;

	case FIT_ALL_SKY:
		fit_progress("Fitting all-sky extinction coefficient", data);
		if (fit_all_sky_zp(mbds, ofrs)) {
			error_beep();
			mbds_printf(data, "%s", last_err());
			mbds_transform_all(mbds, ofrs, 0);
			bands_list_update_vals(data, mbds);
			break;
		}
		fit_progress("Transforming stars", data);
		mbds_transform_all(mbds, ofrs, 0);
		bands_list_update_vals(data, mbds);
		fit_progress("Done", data);
		break;

	default:
		return;
	}

	glh  = gtk_tree_selection_get_selected_rows(sel, NULL);
	if (glh != NULL) {
		for (gl = g_list_first(glh); gl != NULL; gl = g_list_next(gl)) {
			if (!gtk_tree_model_get_iter(ofr_store, &iter, gl->data))
				continue;

			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);
			if (ofr == NULL)
				continue;

			ofr_store_set_row_vals(GTK_LIST_STORE(ofr_store), &iter, ofr);
		}

		g_list_foreach(glh, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(glh);
	} else {
		gboolean valid;

		valid = gtk_tree_model_get_iter_first (ofr_store, &iter);
		while (valid) {
			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);

			if (ofr != NULL)
				ofr_store_set_row_vals(GTK_LIST_STORE(ofr_store), &iter, ofr);

			valid = gtk_tree_model_iter_next (ofr_store, &iter);
		}
	}

	if (sob_list != NULL)
		sob_list_update_vals(sob_list);

	g_list_free(ofrs);
}

void mband_fit_zpoints_action(GtkAction *action, gpointer data)
{
	fit_cb (data, FIT_ZPOINTS, NULL);
}

void mband_fit_zp_wtrans_action(GtkAction *action, gpointer data)
{
	fit_cb (data, FIT_ZP_WTRANS, NULL);
}

void mband_fit_trans_action(GtkAction *action, gpointer data)
{
	fit_cb (data, FIT_TRANS, NULL);
}

void mband_fit_allsky_action(GtkAction *action, gpointer data)
{
	fit_cb (data, FIT_ALL_SKY, NULL);
}

static void mbds_ofr_to_list(GtkWidget *dialog, GtkWidget *list)
{
	struct mband_dataset *mbds;
	GList *sl;
	struct o_frame *ofr;
	GtkTreeIter iter;
	GtkTreeModel *ofr_model;

	mbds = g_object_get_data(G_OBJECT(dialog), "mbds");
	g_return_if_fail(mbds != NULL);

	ofr_model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));

	sl = mbds->ofrs;
	while(sl != NULL) {
		ofr = O_FRAME(sl->data);
		sl = g_list_next(sl);
		if (ofr->skip)
			continue;

		gtk_list_store_append(GTK_LIST_STORE(ofr_model), &iter);
		ofr_store_set_row_vals(GTK_LIST_STORE(ofr_model), &iter, ofr);
	}
}


void ofr_bpress_cb(GtkTreeView *ofr_list, GdkEventButton *event, gpointer data)
{
	int x, y;
	struct o_frame *ofr;
	GtkTreeModel *ofr_store;
	GtkTreePath *path;
	GtkTreeIter iter;

	g_return_if_fail(event->window == gtk_tree_view_get_bin_window(ofr_list));

	x = floor(event->x);
	y = floor(event->y);

	if (gtk_tree_view_get_path_at_pos(ofr_list, x, y, &path, NULL, NULL, NULL)) {

		ofr_store = gtk_tree_view_get_model(ofr_list);
		if (gtk_tree_model_get_iter(ofr_store, &iter, path)) {

			gtk_tree_model_get(ofr_store, &iter, 0, &ofr, -1);
			g_return_if_fail(ofr != NULL);

			mb_rebuild_sob_list(data, ofr->sol);
		}
	}
}

static void mb_rebuild_ofr_list(gpointer dialog)
{
	GtkListStore *ofr_store;
	GtkWidget *ofr_list;
	GtkScrolledWindow *scw;
	char *titles[] = {"Object", "Band", "Status", "Zpoint", "Err", "Fitted",
			  "Outliers", "MEU", "Airmass", "MJD", NULL};
	int i;
	GtkTreeViewColumn *column;

	ofr_list = g_object_get_data(G_OBJECT(dialog), "ofr_list");
	if (ofr_list == NULL) {
		scw = g_object_get_data(G_OBJECT(dialog), "ofr_scw");
		g_return_if_fail(scw != NULL);

		ofr_store = gtk_list_store_new(11, G_TYPE_POINTER,
					       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					       G_TYPE_STRING, G_TYPE_STRING);

		ofr_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ofr_store));
		g_object_unref(ofr_store);

		for (i = 0; i < 10; i++) {
			column = gtk_tree_view_column_new_with_attributes(titles[i],
									  gtk_cell_renderer_text_new(),
									  "text", i + 1, NULL);

			gtk_tree_view_column_set_sort_column_id(column, i + 1);
			gtk_tree_view_append_column(GTK_TREE_VIEW(ofr_list), column);
		}

		gtk_scrolled_window_add_with_viewport(scw, ofr_list);
		g_object_ref(ofr_list);
		g_object_set_data_full(G_OBJECT(dialog), "ofr_list",
					 ofr_list, (GDestroyNotify) gtk_widget_destroy);

		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ofr_list)),
					    GTK_SELECTION_MULTIPLE);

		g_signal_connect(G_OBJECT(ofr_list), "button-press-event",
				 G_CALLBACK(ofr_bpress_cb), dialog);

		gtk_widget_show(ofr_list);
	} else {
		ofr_store = GTK_LIST_STORE (gtk_tree_view_get_model(GTK_TREE_VIEW(ofr_list)));
		gtk_list_store_clear(ofr_store);
	}

	mbds_ofr_to_list(dialog, ofr_list);
}


static void bands_store_set_row_vals(GtkListStore *bands_list, GtkTreeIter *iter, int band,
				     struct mband_dataset *mbds)
{
	char c[50 * 10];
	int i;

	memset(c, 0, sizeof(c));
	snprintf(c, 40, "%s", mbds->trans[band].bname);
	d4_printf("%s\n", c);
	if (mbds->trans[band].c1 >= 0 && mbds->trans[band].c2 >= 0) {
		snprintf(c+50, 40, "%s-%s", mbds->trans[mbds->trans[band].c1].bname,
			 mbds->trans[mbds->trans[band].c2].bname);
		if (mbds->trans[band].kerr < BIG_ERR) {
			snprintf(c+100, 40, "%.3f/%.3f", mbds->trans[band].k,
				 mbds->trans[band].kerr);
		} else {
			c[100] = 0;
		}
	} else {
		c[50] = 0;
		c[100] = 0;
	}
	if (mbds->trans[band].zzerr < BIG_ERR) {
		snprintf(c+150, 40, "%.3f/%.3f", mbds->trans[band].zz,
			 mbds->trans[band].zzerr);
		snprintf(c+200, 40, "%.3f", mbds->trans[band].am);
		snprintf(c+250, 40, "%.3f/%.3f", -mbds->trans[band].e,
			 mbds->trans[band].eerr);
		snprintf(c+300, 40, "%.2f", mbds->trans[band].eme1);
	} else {
		c[150] = c[200] = c[250] = c[300] = 0;
	}

	gtk_list_store_set(bands_list, iter, 0, &mbds->trans[band], -1);
	for (i = 0; i < 6; i++) {
		gtk_list_store_set(bands_list, iter, i + 1, c + 50 * i, -1);
	}
}

static void bands_list_update_vals(GtkWidget *dialog, struct mband_dataset *mbds)
{
	GtkWidget *list;
	GtkListStore *store;
	GtkTreeIter iter;
	int i;

	list = g_object_get_data(G_OBJECT(dialog), "bands_list");
	g_return_if_fail(mbds != NULL);
	g_return_if_fail(list != NULL);

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

	gtk_list_store_clear(store);
	for (i = 0; i < mbds->nbands; i++) {

		gtk_list_store_append(store, &iter);
		bands_store_set_row_vals(store, &iter, i, mbds);
	}
}

static void mbds_bands_to_list(GtkWidget *dialog, GtkWidget *list)
{
	struct mband_dataset *mbds;

	mbds = g_object_get_data(G_OBJECT(dialog), "mbds");
	if (mbds == NULL)
		return;

	bands_list_update_vals(dialog, mbds);
}

static void mb_rebuild_bands_list(gpointer dialog)
{
	GtkWidget *list;
	GtkScrolledWindow *scw;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	char *titles[] = {"Band", "Color", "Color coeff",
			  "All-sky zeropoint", "Mean airmass",
			  "Extinction coeff", "Extinction me1", NULL};
	int i;

	list = g_object_get_data(G_OBJECT(dialog), "bands_list");
	if (list == NULL) {
		scw = g_object_get_data(G_OBJECT(dialog), "bands_scw");
		g_return_if_fail(scw != NULL);

		store = gtk_list_store_new(8, G_TYPE_POINTER,
					   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

		list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		g_object_unref(store);

		for (i = 0; i < 7; i++) {
			column = gtk_tree_view_column_new_with_attributes(titles[i],
									  gtk_cell_renderer_text_new(),
									  "text", i + 1, NULL);

			gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
		}

		gtk_scrolled_window_add_with_viewport(scw, list);
		g_object_ref(list);
		g_object_set_data_full(G_OBJECT(dialog), "bands_list",
					 list, (GDestroyNotify) gtk_widget_destroy);
		gtk_widget_show(list);
	} else {
		store = GTK_LIST_STORE (gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
		gtk_list_store_clear(store);
	}

	mbds_bands_to_list(dialog, list);
}

static void sob_store_set_row_vals(GtkListStore *sob_list, GtkTreeIter *iter, struct star_obs *sob)
{
	char c[50 * 15];
	double se;
	int flags, n, i;

	snprintf(c, 40, "%s", sob->cats->name);
	switch(CATS_TYPE(sob->cats)) {
	case CAT_STAR_TYPE_APSTD:
		strcpy(c+50, "Std");
		break;
	case CAT_STAR_TYPE_APSTAR:
		strcpy(c+50, "Tgt");
		break;
	case CAT_STAR_TYPE_CAT:
		strcpy(c+50, "Obj");
		break;
	case CAT_STAR_TYPE_SREF:
		strcpy(c+50, "Field");
		break;
	default:
		c[50] = 0;
		break;
	}
	snprintf(c+100, 40, "%s", sob->ofr->trans->bname);

	c[150] = c[200] = 0;
	if (CATS_TYPE(sob->cats) == CAT_STAR_TYPE_APSTD) {
		if (sob->ofr->band >= 0 && sob->ost->smagerr[sob->ofr->band] < BIG_ERR) {
			snprintf(c+150, 40, "%5.2f", sob->ost->smag[sob->ofr->band]);
			snprintf(c+200, 40, "%.3f", sob->ost->smagerr[sob->ofr->band]);
		} else if (sob->ofr->band >= 0 && sob->err < BIG_ERR) {
			snprintf(c+150, 40, "[%5.2f]", sob->mag);
			snprintf(c+200, 40, "%.3f", sob->err);
		}
	} else {
		if (sob->ofr->band >= 0 && sob->err < BIG_ERR) {
			snprintf(c+150, 40, "[%5.2f]", sob->mag);
			snprintf(c+200, 40, "%.3f", sob->err);
		}
	}
	snprintf(c+250, 40, "%6.2f", sob->imag);
	snprintf(c+300, 40, "%.3f", sob->imagerr);
	if ((sob->ofr->zpstate & ZP_STATE_M) >= ZP_ALL_SKY && sob->weight > 0.0) {
		snprintf(c+350, 40, "%7.3f", sob->residual);
		se = fabs(sob->residual * sqrt(sob->nweight));
		snprintf(c+400, 40, "%.3f", se);
		snprintf(c+450, 40, "%s", se > OUTLIER_THRESHOLD ? "Y" : "N");
	} else {
		c[350] = c[400] = c[450] = 0;
	}
	degrees_to_dms_pr(c+500, sob->cats->ra / 15.0, 2);
	degrees_to_dms_pr(c+550, sob->cats->dec, 1);
	i = 0; n = 0;
	flags = sob->flags & (CPHOT_MASK) & ~(CPHOT_NO_COLOR);
	while (cat_flag_names[i] != NULL) {
		if (flags & 0x01)
			n += snprintf(c+600+n, 100-n, "%s ", cat_flag_names[i]);
		if (n >= 100)
			break;
		flags >>= 1;
		i++;
	}

	gtk_list_store_set(sob_list, iter, 0, sob, -1);
	for (i = 0; i < 13; i++) {
		gtk_list_store_set(sob_list, iter, i + 1, c + 50 * i, -1);
	}
}

static void sob_list_update_vals(GtkWidget *sob_list)
{
	struct star_obs *sob;
	GtkTreeModel *store;
	GtkTreeIter iter;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(sob_list));

	if (gtk_tree_model_get_iter_first(store, &iter)) {
		do {
			gtk_tree_model_get(store, &iter, 0, &sob, -1);
			if (sob == NULL)
				continue;

			sob_store_set_row_vals(GTK_LIST_STORE(store), &iter, sob);
		} while (gtk_tree_model_iter_next(store, &iter));
	}
}

static void sobs_to_list(GtkWidget *list, GList *sol)
{
	GList *sl;
	struct star_obs *sob;
	GtkTreeModel *store;
	GtkTreeIter iter;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(list));

	sl = sol;
	while (sl != NULL) {
		sob = STAR_OBS(sl->data);
		sl = g_list_next(sl);

		gtk_list_store_append(GTK_LIST_STORE(store), &iter);
		sob_store_set_row_vals(GTK_LIST_STORE(store), &iter, sob);
	}
}

/* FIXME */
static void mb_rebuild_sob_list(gpointer dialog, GList *sol)
{
	GtkWidget *list;
	GtkScrolledWindow *scw;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	int i;
	char *titles[] = {"Name", "Type", "Band", "Smag", "Err", "Imag", "Err", "Residual",
			  "Std Error", "Outlier", "R.A.", "Declination", "Flags", NULL};

	list = g_object_get_data(G_OBJECT(dialog), "sob_list");
	if (list == NULL) {
		scw = g_object_get_data(G_OBJECT(dialog), "sob_scw");
		g_return_if_fail(scw != NULL);

		store = gtk_list_store_new(14, G_TYPE_POINTER,
					   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					   G_TYPE_STRING, G_TYPE_STRING);

		list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		g_object_unref(store);

		for (i = 0; i < 13; i++) {
			column = gtk_tree_view_column_new_with_attributes(titles[i],
									  gtk_cell_renderer_text_new(),
									  "text", i + 1, NULL);

			gtk_tree_view_column_set_sort_column_id(column, i + 1);
			gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
		}

		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)),
					    GTK_SELECTION_MULTIPLE);


		gtk_scrolled_window_add_with_viewport(scw, list);
		g_object_ref(list);
		g_object_set_data_full(G_OBJECT(dialog), "sob_list",
					 list, (GDestroyNotify) gtk_widget_destroy);

		gtk_widget_show(list);
	} else {
		store = GTK_LIST_STORE (gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
		gtk_list_store_clear(store);
	}

	sobs_to_list(list, sol);
}

void add_to_mband(gpointer dialog, char *fn)
{
	struct mband_dataset *mbds;
	FILE * inf;
	struct stf *stf;
	int ret, n=0;

//	d1_printf("loading report file: %s\n", fn);
	inf = fopen(fn, "r");
	if (inf == NULL) {
		mbds_printf(dialog, "Cannot open file %s for reading: %s\n",
			   fn, strerror(errno));
		error_beep();
		return;
	}

	mbds = g_object_get_data(G_OBJECT(dialog), "mbds");
	if (mbds == NULL) {
		mbds = mband_dataset_new();
		d3_printf("mbds: %p\n", mbds);
		mband_dataset_set_bands_by_string(mbds, P_STR(AP_BANDS_SETUP));
		g_object_set_data_full(G_OBJECT(dialog), "mbds",
					 mbds, (GDestroyNotify)(mband_dataset_release));
	}
	while (	(stf = stf_read_frame(inf)) != NULL ) {
//		d3_printf("----------------------------------read stf\n");
//		stf_fprint(stderr, stf, 0, 0);
		ret = mband_dataset_add_stf(mbds, stf);
		if (ret >= 0) {
			n++;
		}
		mbds_printf(dialog, "%d frames read", n);
		while (gtk_events_pending ())
			gtk_main_iteration ();

	}
	if (n == 0) {
		error_beep();
		mbds_printf(dialog, "%s: %s", fn, last_err());
		return;
	} else {
		mbds_printf(dialog, "Dataset has %d observation(s) %d object(s), %d frame(s)\n",
			    g_list_length(mbds->sobs), g_list_length(mbds->ostars),
			    g_list_length(mbds->ofrs));
	}

	mb_rebuild_ofr_list(dialog);
	mb_rebuild_bands_list(dialog);
}


void stf_to_mband(gpointer dialog, struct stf *stf)
{
	struct mband_dataset *mbds;
	int ret;


	mbds = g_object_get_data(G_OBJECT(dialog), "mbds");
	if (mbds == NULL) {
		mbds = mband_dataset_new();
		d3_printf("mbds: %p\n", mbds);
		mband_dataset_set_bands_by_string(mbds, P_STR(AP_BANDS_SETUP));
		g_object_set_data_full(G_OBJECT(dialog), "mbds",
					 mbds, (GDestroyNotify)(mband_dataset_release));
	}
	ret = mband_dataset_add_stf(mbds, stf);
	if (ret == 0) {
		error_beep();
		mbds_printf(dialog, "%s", last_err());
		return;
	} else {
		mbds_printf(dialog, "Dataset has %d observation(s) %d object(s), %d frame(s)\n",
			    g_list_length(mbds->sobs), g_list_length(mbds->ostars),
			    g_list_length(mbds->ofrs));
	}
	mb_rebuild_ofr_list(dialog);
	mb_rebuild_bands_list(dialog);
}



void mbds_to_mband(gpointer dialog, struct mband_dataset *nmbds)
{
	struct mband_dataset *mbds;

	mbds = g_object_get_data(G_OBJECT(dialog), "mbds");
	if (mbds != NULL) {
		GtkWidget *list;
		list = g_object_get_data(G_OBJECT(dialog), "ofr_list");
		if (list) {
//		gtk_widget_hide(list);
			g_object_set_data(G_OBJECT(dialog), "ofr_list", NULL);
		}
		list = g_object_get_data(G_OBJECT(dialog), "sob_list");
		if (list) {
//		gtk_widget_hide(list);
			g_object_set_data(G_OBJECT(dialog), "sob_list", NULL);
		}
		list = g_object_get_data(G_OBJECT(dialog), "bands_list");
		if (list) {
//		gtk_widget_hide(list);
			g_object_set_data(G_OBJECT(dialog), "bands_list", NULL);
		}
		g_object_set_data(G_OBJECT(dialog), "mbds", NULL);
	}
	nmbds->ref_count ++;
	g_object_set_data_full(G_OBJECT(dialog), "mbds",
				 nmbds, (GDestroyNotify)(mband_dataset_release));
	mbds_printf(dialog, "Dataset has %d observation(s) %d object(s), %d frame(s)\n",
		    g_list_length(nmbds->sobs), g_list_length(nmbds->ostars),
		    g_list_length(nmbds->ofrs));
	mb_rebuild_ofr_list(dialog);
	mb_rebuild_bands_list(dialog);
}




gboolean mband_window_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_hide(widget);
//	g_object_set_data(G_OBJECT(data), "mband_window", NULL);
	return TRUE;
}

void mband_close_action(GtkAction *action, gpointer data)
{
	mband_window_delete(data, NULL, NULL);
}

/* name, stock id, label, accel, tooltip, callback */
static GtkActionEntry mband_menu_actions[] = {
	{ "mband-file",   NULL, "_File" },
	{ "mband-edit",   NULL, "_Edit" },
	{ "mband-reduce", NULL, "_Reduce" },
	{ "mband-plot",   NULL, "_Plot" },

        /* File */
        { "mband-file-add", NULL, "_Add to Dataset", "<control>O", NULL, G_CALLBACK (file_add_mband_action) },
	{ "mband-file-save", NULL, "_Save Dataset", "<control>S", NULL, G_CALLBACK (mband_save_dataset_action) },
	{ "mband-file-close", NULL, "_Close Dataset", "<control>W", NULL, G_CALLBACK (mband_close_dataset_action) },
	{ "mband-report-aavso", NULL, "Report _Targets in AAVSO Format", NULL, NULL, G_CALLBACK(mband_report_action) },
	{ "mband-close", NULL, "Close Window", "<control>Q", NULL, G_CALLBACK(mband_close_action) },

	/* Edit */
	{ "mband-select-all", NULL, "Select _All", "<control>A", NULL, G_CALLBACK(mband_select_all_action) },
	{ "mband-unselect-all", NULL, "_Unselect All", "<control>U", NULL, G_CALLBACK(mband_unselect_all_action) },
	{ "mband-hide-selected", NULL, "_Hide Selected", "H", NULL, G_CALLBACK(mband_hide_action) },
	{ "mband-unhide-all", NULL, "Unhi_de All", "<shift>H", NULL, G_CALLBACK(mband_unhide_action) },

	/* Reduce */
	{ "mband-fit-zpoints", NULL, "Fit _Zero Points with Null Coefficients", NULL, NULL, G_CALLBACK(mband_fit_zpoints_action) },
	{ "mband-fit-zp-wtrans", NULL, "Fit Zero Points with _Current Coefficients", NULL, NULL, G_CALLBACK(mband_fit_zp_wtrans_action) },
	{ "mband-fit-trans", NULL, "Fit Zero Points and _Transformation Coefficients", NULL, NULL, G_CALLBACK(mband_fit_trans_action) },
	{ "mband-fit-allsky", NULL, "Fit Extinction and All-Sky Zero Points", NULL, NULL, G_CALLBACK(mband_fit_allsky_action) },

	/* Plot */
	{ "mband-plot-residuals-magnitude", NULL, "_Residuals vs Magnitude", NULL, NULL, G_CALLBACK(mband_plot_resmag_action) },
	{ "mband-plot-residuals-color", NULL, "Residuals vs _Color", NULL, NULL, G_CALLBACK(mband_plot_rescol_action) },
	{ "mband-plot-errors-magnitude", NULL, "Standard _Errors vs Magnitude", NULL, NULL, G_CALLBACK(mband_plot_errmag_action) },
	{ "mband-plot-errors-color", NULL, "_Standard Errors vs Color", NULL, NULL, G_CALLBACK(mband_plot_errcol_action) },
	{ "mband-plot-zp-airmass", NULL, "_Zeropoints vs Airmass", NULL, NULL, G_CALLBACK(mband_plot_zpairmass_action) },
	{ "mband-plot-zp-time", NULL, "Zeropoints vs _Time", NULL, NULL, G_CALLBACK(mband_plot_zptime_action) },
	{ "mband-plot-magnitude-time", NULL, "Star _Magnitude vs Time", NULL, NULL, G_CALLBACK(mband_plot_magtime_action) },
};

/* create the menu bar */
static GtkWidget *get_main_menu_bar(GtkWidget *window)
{
	GtkWidget *ret;
	GtkUIManager *ui;
	GError *error;
	GtkActionGroup *action_group;
	static char *mband_ui =
		"<menubar name='mband-menubar'>"
		"  <!-- File -->"
		"  <menu name='mband-file' action='mband-file'>"
		"    <menuitem name='mband-file-add' action='mband-file-add'/>"
		"    <menuitem name='mband-file-save' action='mband-file-save'/>"
		"    <menuitem name='mband-file-close' action='mband-file-close'/>"
		"    <separator name='separator1'/>"
		"    <menuitem name='mband-report-aavso' action='mband-report-aavso'/>"
		"    <separator name='separator2'/>"
		"    <menuitem name='mband-close' action='mband-close'/>"
		"  </menu>"
		"  <!-- Edit -->"
		"  <menu name='mband-edit' action='mband-edit'>"
		"    <menuitem name='mband-select-all' action='mband-select-all'/>"
		"    <menuitem name='mband-unselect-all' action='mband-unselect-all'/>"
		"    <menuitem name='mband-hide-selected' action='mband-hide-selected'/>"
		"    <menuitem name='mband-unhide-all' action='mband-unhide-all'/>"
		"  </menu>"
		"  <!-- Reduce -->"
		"  <menu name='mband-reduce' action='mband-reduce'>"
		"    <menuitem name='mband-fit-zpoints' action='mband-fit-zpoints'/>"
		"    <menuitem name='mband-fit-zp-wtrans' action='mband-fit-zp-wtrans'/>"
		"    <menuitem name='mband-fit-trans' action='mband-fit-trans'/>"
		"    <menuitem name='mband-fit-allsky' action='mband-fit-allsky'/>"
		"  </menu>"
		"  <!-- Plot -->"
		"  <menu name='mband-plot' action='mband-plot'>"
		"    <menuitem name='mband-plot-residuals-magnitude' action='mband-plot-residuals-magnitude'/>"
		"    <menuitem name='mband-plot-residuals-color' action='mband-plot-residuals-color'/>"
		"    <menuitem name='mband-plot-errors-magnitude' action='mband-plot-errors-magnitude'/>"
		"    <menuitem name='mband-plot-errors-color' action='mband-plot-errors-color'/>"
		"    <separator name='separator1'/>"
		"    <menuitem name='mband-plot-zp-airmass' action='mband-plot-zp-airmass'/>"
		"    <menuitem name='mband-plot-zp-time' action='mband-plot-zp-time'/>"
		"    <separator name='separator2'/>"
		"    <menuitem name='mband-plot-magnitude-time' action='mband-plot-magnitude-time'/>"
		"  </menu>"
		"</menubar>";


	action_group = gtk_action_group_new ("MBActions");
	gtk_action_group_add_actions (action_group, mband_menu_actions,
				      G_N_ELEMENTS (mband_menu_actions), window);

	ui = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui, action_group, 0);

	error = NULL;
	gtk_ui_manager_add_ui_from_string (ui, mband_ui, strlen(mband_ui), &error);
	if (error) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		return NULL;
	}

	ret = gtk_ui_manager_get_widget (ui, "/mband-menubar");

        /* Make sure that the accelerators work */
	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (ui));

	g_object_ref (ret);
	g_object_unref (ui);

	return ret;
}

/* create / open the guiding dialog */
void mband_open_cb(gpointer data, guint action, GtkWidget *menu_item)
{
	GtkWidget *window = data;
	GtkWidget *dialog, *vb, *menubar;

	dialog = g_object_get_data(G_OBJECT(window), "mband_window");
	if (dialog == NULL) {
		dialog = create_mband_dialog();
		g_object_ref(dialog);
		g_object_set_data_full(G_OBJECT(window), "mband_window", dialog,
				       (GDestroyNotify) g_object_unref);
		g_object_set_data(G_OBJECT(dialog), "in_window", window);
		vb = g_object_get_data(G_OBJECT(dialog), "mband_vbox");
		g_signal_connect(G_OBJECT(dialog), "delete_event",
				 G_CALLBACK(mband_window_delete), window);
		menubar = get_main_menu_bar(dialog);
		gtk_box_pack_start(GTK_BOX(vb), menubar, FALSE, TRUE, 0);
		gtk_box_reorder_child(GTK_BOX(vb), menubar, 0);
		gtk_widget_show(menubar);
		if (action)
			gtk_widget_show(dialog);
	} else if (action) {
		gtk_widget_show(dialog);
		gdk_window_raise(dialog->window);
	}
}
