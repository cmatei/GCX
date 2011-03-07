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

/* functions for querying on-line catalogs */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
//#include <glib.h>
#include <glob.h>

#include "gcx.h"
#include "gsc/gsc.h"
#include "catalogs.h"
#include "gui.h"
#include "sourcesdraw.h"
#include "params.h"
#include "wcs.h"
#include "symbols.h"
#include "recipy.h"
#include "misc.h"
#include "query.h"
#include "interface.h"

char *query_catalog_names[] = CAT_QUERY_NAMES;

enum {
	QTABLE_UCAC2,
	QTABLE_UCAC2_BSS,
	QTABLE_GSC2,
	QTABLE_USNOB,
	QTABLE_GSC_ACT,
	QTABLES,
};

char *table_names[] = {"I_289_out", "I_294_ucac2bss", "I_271_out", "I_284_out",
		       "I_255_out"};

static int detabify(char *line, int cols[], int n)
{
	int nc = 0;
	int i;

	i = 0;
	cols[nc++] = 0;
	while (line[i] && nc < n) {
		if (line[i] == '\t') {
			line[i] = 0;
			cols[nc++] = i+1;
		}
		i++;
	}
	return nc;
}

static struct cat_star * parse_cat_line_ucac2_main(char *line)
{
	struct cat_star * cats;
	int cols[32];
	int nc;
	double u, v, w, x;
	double uc, m, j, k;
	char *endp;
	int ret = 0;

	nc = detabify(line, cols, 32);

	d4_printf("[ucac2-%d]%s\n", nc, line);

	if (nc < 10)
		return NULL;

	u = strtod(line+cols[1], &endp);
	if (line+cols[1] == endp)
		return NULL;
	v = strtod(line+cols[2], &endp);
	if (line+cols[2] == endp)
		return NULL;

	cats = cat_star_new();
	snprintf(cats->name, CAT_STAR_NAME_SZ, "ucac%s", line+cols[0]);
	cats->ra = u;
	cats->dec = v;
	cats->equinox = 2000.0;
	cats->flags = CAT_STAR_TYPE_SREF;

	uc = strtod(line+cols[5], &endp);
	cats->mag = uc;

	u = strtod(line+cols[3], &endp);
	v = strtod(line+cols[4], &endp);
	u *= cos(cats->dec);
	if (u > 0 && v > 0) {
		cats->perr = sqrt(sqr(u) + sqr(v)) / 1000.0;
	} else {
		cats->perr = BIG_ERR;
	}

	if (nc > 12) {
		m = strtod(line+cols[10], &endp);
		j = strtod(line+cols[11], &endp);
		k = strtod(line+cols[12], &endp);
		if (m > 0.0 && j > 0.0 && k > 0.0) {
			if (-1 == asprintf(&cats->comments, "p=u 2mass=%.0f", m))
				cats->comments = NULL;
			if (uc == 0)
				ret = asprintf(&cats->smags, "j=%.3f k=%.3f", j, k);
			else
				ret = asprintf(&cats->smags, "uc=%.3f j=%.3f k=%.3f", uc, j, k);
		}
	}
	if (ret == -1)
		cats->smags = NULL;
	w = strtod(line+cols[8], &endp);
	if (line+cols[8] == endp)
		return cats;
	x = strtod(line+cols[9], &endp);
	if (line+cols[9] == endp)
		return cats;

	cats->flags |= CAT_ASTROMET;

	cats->astro = calloc(1, sizeof(struct cats_astro));
	g_return_val_if_fail(cats->astro != NULL, cats);

	cats->astro->epoch = 2000.0;
	cats->astro->ra_err = u;
	cats->astro->dec_err = v;
	cats->astro->ra_pm = w;
	cats->astro->dec_pm = x;
	cats->astro->flags = ASTRO_HAS_PM;

	cats->astro->catalog = strdup("ucac2");
	return cats;
}

static struct cat_star * parse_cat_line_ucac2_bss(char *line)
{
	struct cat_star * cats;
	int cols[32];
	int nc;
	double u, v, w, x;
	double uc, m, j, bv, h, t1, t2;
	char *endp;
	char buf[256];
	int n;

//	d3_printf("%s", line);

	nc = detabify(line, cols, 32);

	d4_printf("[ucac2bss-%d]%s\n", nc, line);

	if (nc < 17)
		return NULL;

	u = strtod(line+cols[2], &endp);
	if (line+cols[1] == endp)
		return NULL;
	v = strtod(line+cols[3], &endp);
	if (line+cols[2] == endp)
		return NULL;

	cats = cat_star_new();
	snprintf(cats->name, CAT_STAR_NAME_SZ, "ucac2bss%s", line+cols[1]);

	/* coordinates and errors */
	cats->ra = u;
	cats->dec = v;
	cats->equinox = 2000.0;
	cats->flags = CAT_STAR_TYPE_SREF;

	u = strtod(line+cols[4], &endp);
	v = strtod(line+cols[5], &endp);
	if (u > 0 && v > 0) {
		cats->perr = sqrt(sqr(u) + sqr(v)) / 1000.0;
	} else {
		cats->perr = BIG_ERR;
	}

	/* magnitudes and aliases */
	uc = strtod(line+cols[10], &endp);
	cats->mag = uc;

	m = strtod(line+cols[13], &endp);
	j = strtod(line+cols[11], &endp);
	bv = strtod(line+cols[9], &endp);
	h = strtod(line+cols[14], &endp);
	t1 = strtod(line+cols[16], &endp);
	t2 = strtod(line+cols[17], &endp);
	n = 0;
	if (t1 > 0 || t2 > 0) {
		n = snprintf(buf, 255, "tycho2=%04.0f-%04.0f ", t1, t2);
	}
	if (h > 0) {
		n += snprintf(buf+n, 255-n, "hip=%.0f ", h);
	}
	if (m > 0) {
		n += snprintf(buf+n, 255-n, "2mass=%.0f ", m);
	}
	if (n)
		cats->comments = strdup(buf);
	n = 0;
	if (uc > 0) {
		n = snprintf(buf, 255, "v=%.3f ", uc);
	}
	if (bv > 0) {
		n += snprintf(buf+n, 255-n, "b-v=%.3f ", bv);
	}
	if (j > 0) {
		n += snprintf(buf+n, 255-n, "j=%.3f ", j);
	}
	if (n)
		cats->smags = strdup(buf);

	/* proper motions */
	w = strtod(line+cols[7], &endp);
	if (line+cols[8] == endp)
		return cats;
	x = strtod(line+cols[8], &endp);
	if (line+cols[9] == endp)
		return cats;

	cats->flags |= CAT_ASTROMET;

	cats->astro = calloc(1, sizeof(struct cats_astro));
	g_return_val_if_fail(cats->astro != NULL, cats);

	cats->astro->epoch = 2000.0;
	cats->astro->ra_err = u * cos(cats->dec);
	cats->astro->dec_err = v;
	cats->astro->ra_pm = w;
	cats->astro->dec_pm = x;
	cats->astro->flags = ASTRO_HAS_PM;

	cats->astro->catalog = strdup("ucac2bss");
	return cats;
}

static struct cat_star * parse_cat_line_gsc2(char *line)
{
	struct cat_star * cats;
	int cols[32];
	int nc;
	double u, v;
	double fr, fre, bj, bje, class;
	char *endp;
	char buf[256];
	int n;

	nc = detabify(line, cols, 32);

	d4_printf("[gsc2-%d]%s\n", nc, line);

	if (nc < 8)
		return NULL;

	u = strtod(line+cols[1], &endp);
	if (line+cols[1] == endp)
		return NULL;
	v = strtod(line+cols[2], &endp);
	if (line+cols[2] == endp)
		return NULL;

	cats = cat_star_new();
	snprintf(cats->name, CAT_STAR_NAME_SZ, "%s", line+cols[0]);
	cats->ra = u;
	cats->dec = v;
	cats->equinox = 2000.0;
	cats->flags = CAT_STAR_TYPE_SREF;

	fr = strtod(line+cols[3], &endp);
	fre = strtod(line+cols[4], &endp);
	bj = strtod(line+cols[5], &endp);
	bje = strtod(line+cols[6], &endp);
	class = strtod(line+cols[7], &endp);

	if (fr > 0)
		cats->mag = fr;
	else if (bj > 0)
		cats->mag = bj;

	cats->perr = BIG_ERR;
	if (-1 == asprintf(&cats->comments, "p=g gsc2_class=%.0f", class))
		cats->comments = NULL;
	n = 0;
	if (fr > 0) {
		if (fre > 0)
			n = snprintf(buf, 255, "rf=%.3f/%.3f ", fr, fre);
		else
			n = snprintf(buf, 255, "rf=%.3f ", fr);
	}
	if (bj > 0) {
		if (bje > 0)
			n += snprintf(buf+n, 255-n, "bj=%.3f/%.3f ", bj, bje);
		else
			n += snprintf(buf+n, 255-n, "bj=%.3f ", bj);
	}
	if (n)
		cats->smags = strdup(buf);
	cats->flags |= CAT_ASTROMET;
	return cats;
}

static struct cat_star * parse_cat_line_gsc_act(char *line)
{
	struct cat_star * cats;
	int cols[32];
	int nc;
	double u, v;
	double p, pe, ep, class;
	char *endp;
	int ret = 0;

	nc = detabify(line, cols, 32);

	d4_printf("[gsc_act-%d]%s\n", nc, line);

	if (nc < 9)
		return NULL;

	u = strtod(line+cols[1], &endp);
	if (line+cols[1] == endp)
		return NULL;
	v = strtod(line+cols[2], &endp);
	if (line+cols[2] == endp)
		return NULL;

	cats = cat_star_new();
	memmove(line+cols[0], line+cols[0]+1, 4);
	*(line+cols[0]+4) = 0;
	p = strtod(line+cols[0]+5, &endp);
	snprintf(cats->name, CAT_STAR_NAME_SZ, "%s-%04.0f", line+cols[0], p);
	cats->ra = u;
	cats->dec = v;
	cats->equinox = 2000.0;
	cats->flags = CAT_STAR_TYPE_SREF;
	cats->perr = strtod(line+cols[3], &endp);
	if (cats->perr == 0.0)
		cats->perr = BIG_ERR;

	p = strtod(line+cols[4], &endp);
	pe = strtod(line+cols[5], &endp);
	class = strtod(line+cols[7], &endp);

	cats->mag = p;

	if (-1 == asprintf(&cats->comments, "p=G gsc_class=%.0f", class))
		cats->comments = NULL;
	if (p > 0) {
		if (pe > 0)
			ret = asprintf(&cats->smags, "p=%.3f/%.3f ", p, pe);
		else
			ret = asprintf(&cats->smags, "p=%.3f ", p);
	}
	if (ret == -1)
		cats->smags = NULL;

	cats->flags |= CAT_ASTROMET;
	ep = strtod(line+cols[8], &endp);
	if (ep <= 0.0)
		return cats;

	cats->astro = calloc(1, sizeof(struct cats_astro));
	g_return_val_if_fail(cats->astro != NULL, cats);

	cats->astro->epoch = ep;
	if (cats->perr < BIG_ERR) {
		cats->astro->ra_err = 707 * cats->perr;
		cats->astro->dec_err = 707 * cats->perr;
	} else {
		cats->astro->ra_err = 1000 * BIG_ERR;
		cats->astro->dec_err = 1000 * BIG_ERR;
	}
	return cats;
}


static struct cat_star * parse_cat_line_usnob(char *line)
{
	struct cat_star * cats;
	int cols[32];
	char buf[256];
	int nc;
	double u, v, w, x;
	double in, b1, b2, r1, r2;
	char *endp;
	int n;

	d4_printf("%s", line);

	nc = detabify(line, cols, 32);

	d4_printf("[usnob-%d]%s\n", nc, line);

	if (nc < 11)
		return NULL;

	u = strtod(line+cols[1], &endp);
	if (line+cols[1] == endp)
		return NULL;
	v = strtod(line+cols[2], &endp);
	if (line+cols[2] == endp)
		return NULL;

	cats = cat_star_new();
	snprintf(cats->name, CAT_STAR_NAME_SZ, "%s", line+cols[0]);
	cats->ra = u;
	cats->dec = v;
	cats->equinox = 2000.0;
	cats->flags = CAT_STAR_TYPE_SREF;

	u = strtod(line+cols[3], &endp);
	v = strtod(line+cols[4], &endp);
	if (u > 0 && v > 0) {
		cats->perr = sqrt(sqr(u) + sqr(v)) / 1000.0;
	} else {
		cats->perr = BIG_ERR;
	}

	if (nc >= 14) {
		b2 = strtod(line+cols[11], &endp);
		r2 = strtod(line+cols[12], &endp);
		in = strtod(line+cols[13], &endp);
	} else {
		b2 = r2 = in = 0.0;
	}
	b1 = strtod(line+cols[9], &endp);
	r1 = strtod(line+cols[10], &endp);

	if (r1 > 0)
		cats->mag = r1;
	else if (r2 > 0)
		cats->mag = r2;
	else if (b1 > 0)
		cats->mag = b1;
	else if (b2 > 0)
		cats->mag = r2;
	else
		cats->mag = 20.0;

	if (r1 > 0 && r2 > 0)
		r1 = (r1 + r2) / 2;
	else
		r1 = (r1 + r2);
	if (b1 > 0 && b2 > 0)
		b1 = (b1 + b2) / 2;
	else
		b1 = (b1 + b2);

	n = 0;
	if (b1 > 0) {
		n = snprintf(buf, 255, "bj=%.3f ", b1);
	}
	if (r1 > 0) {
		n += snprintf(buf+n, 255-n, "rf=%.3f ", b1);
	}
	if (in > 0) {
		n += snprintf(buf+n, 255-n, "n=%.3f ", in);
	}
	if (n)
		cats->smags = strdup(buf);

	cats->comments = strdup("p=b");

	w = strtod(line+cols[6], &endp);
	if (line+cols[8] == endp)
		return cats;
	x = strtod(line+cols[7], &endp);
	if (line+cols[9] == endp)
		return cats;

	cats->flags |= CAT_ASTROMET;

	cats->astro = calloc(1, sizeof(struct cats_astro));
	g_return_val_if_fail(cats->astro != NULL, cats);

	cats->astro->epoch = 2000.0;
	cats->astro->ra_err = u;
	cats->astro->dec_err = v;
	if (w != 0.0 || x != 0.0) {
		cats->astro->ra_pm = w*cos(cats->dec);
		cats->astro->dec_pm = x;
		cats->astro->flags = ASTRO_HAS_PM;
	}

	cats->astro->catalog = strdup("usnob");
	return cats;
}


static struct cat_star * parse_cat_line(char *line, int tnum)
{
	switch(tnum) {
	case QTABLE_UCAC2:
		return parse_cat_line_ucac2_main(line);
	case QTABLE_UCAC2_BSS:
		return parse_cat_line_ucac2_bss(line);
	case QTABLE_GSC2:
		return parse_cat_line_gsc2(line);
	case QTABLE_USNOB:
		return parse_cat_line_usnob(line);
	case QTABLE_GSC_ACT:
		return parse_cat_line_gsc_act(line);
	default:
		return NULL;
	}
}

static int table_code(char *table)
{
	int i;
	char *n, *t;
	d3_printf("searching table %s", table);
	for (i = 0; i < QTABLES; i++) {
		t = table;
		n = table_names[i];
		while (*t && *n && (*n == *t)) {
			n++; t++;
		}
		if (*n)
			continue;
		if ((*t == '_') || (isalnum(*t)))
			continue;
		break;
	}
	if (i == QTABLES) {
		d4_printf("n|%s| t|%s|\n", n, t);
		return -1;
	} else {
		d4_printf("table: %d -- %s", i, t);
		return i;
	}
}


/* return a list of catalog stars obtained by querying an on-line catalog */
GList *query_catalog(char *catalog, double ra, double dec, double w, double h,
		     int (* progress)(char *msg, void *data), void *data)
{
	FILE *vq;
	char cmd[1024];
	int tnum = -1;
	GList *cat=NULL;
	fd_set fds;
	struct timeval tv;
	int n;
	char *line, *p;
	int ret;
	size_t ll;
	int qstat;
	struct cat_star *cats;
	int pabort;
	char prp[256];

	ll = 256;
	line = malloc(256);

	if (catalog == NULL) {
		err_printf("bad catalog %d\n", catalog);
		return NULL;
	}

	snprintf(cmd, 1023, "%s -mime=tsv <<====\n"
		 "-source=%s\n"
		 "-c=%.4f %+.4f\n"
		 "-c.bm=%.1fx%.1f\n"
		 "-out=*\n"
		 "-out.max=%d\n"
		 "====\n",
		 P_STR(QUERY_VIZQUERY), catalog,
		 ra, dec, w, h, P_INT(QUERY_MAX_STARS));

	d3_printf("vizquery: %s", cmd);

	/* cmatei: FIXME

	   vq != NULL if vizquery cannot be run, because the shell
	   itself DOES run. We then go in a rather infinite loop
	   below, due to suboptimal error handling :-) */
	vq = popen(cmd, "r");
	if (vq == NULL) {
		err_printf("cannot run vizquery (%s)\n", strerror(errno));
		return NULL;
	}

	if (progress) {
		snprintf(prp, 255, "Connecting to CDS for %s:\n"
			 "ra=%.4f dec=%.4f w=%.0f' h=%.0f' max_stars=%d\n",
			 catalog, ra, dec, w, h, P_INT(QUERY_MAX_STARS));
		(* progress)(prp, data);
	}


	qstat = 0;
	n = 0;
	do {
		FD_ZERO(&fds);
		FD_SET(fileno(vq), &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 200000;
		ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
		if (ret == 0 || errno || !FD_ISSET(fileno(vq), &fds)) {
			if (progress) {
				pabort = (* progress)(".", data);
				if (pabort)
					break;
			}
			continue;
		}
		ret = getline(&line, &ll, vq);
		if (ret <= 0)
			continue;
		n++;
		d4_printf("%s", line);
		switch(qstat) {
		case 0:		/* look for a table name */
			if (!strncasecmp(line, "#Title", 6)) {
				line[0] = '\n';
				if (progress) {
					(* progress)(line, data);
				}
			}
			if ((!strncasecmp(line, "#++++", 5))
			    || (!strncasecmp(line, "#****", 5))) {
				if (progress) {
					(* progress)(line, data);
				}
				break;
			}
			if (strncasecmp(line, "#Table", 6))
				break;
			p = line+6;
			while (*p && isspace(*p))
				p++;
			tnum = table_code(p);
			if (tnum < 0) {
				if (progress) {
					(* progress)("Skipping unknown table ", data);
					(* progress)(p, data);
				}
				break;
			} else {
				qstat = 1;
			}
			break;
		case 1:
			if (strncasecmp(line, "--", 2))
				break;
			qstat = 2;
			break;
		case 2:
			if ((!strncasecmp(line, "#++++", 5))
			    || (!strncasecmp(line, "#****", 5))) {
				if (progress) {
					(* progress)(line, data);
				}
				qstat = 0;
				break;
			}
			cats = parse_cat_line(line, tnum);
			if (cats) {
				cat = g_list_prepend(cat, cats);
			} else {
				qstat = 0;
			}
			break;
		}
		if (progress && (n > 0) && ((n % 100) == 0)) {
			if ((n % 5000) == 0) {
				snprintf(prp, 32, "*\n");
			} else {
				snprintf(prp, 32, "*");
			}
			pabort = (* progress)(prp, data);
			if (pabort)
				break;
		}
	} while (ret >= 0);
	if (progress) {
		(* progress)("\n", data);
	}
	pclose(vq);
	free(line);
	return cat;
}

static int progress_pr(char *msg, void *data)
{
	info_printf(msg);
	return 0;
}


/* create a rcp using starts returned by a catalog query */
int make_cat_rcp(char *obj, char *catalog, double box, FILE *outf, double mag_limit)
{
	GList *tsl = NULL;
	struct cat_star *cats;
	struct stf *st, *stf;
	char ras[64], decs[64];

	cats = get_object_by_name(obj);
	cats->flags = (cats->flags & ~CAT_STAR_TYPE_MASK) | CAT_STAR_TYPE_APSTAR;
	if (cats == NULL) {
		err_printf("make_cat_rcp: cannot find object %s\n", obj);
		return -1;
	}

	tsl = query_catalog("usnob", cats->ra, cats->dec, box, box, progress_pr, NULL);

	d3_printf("query_catalog: got %d stars\n", g_list_length(tsl));

//	tsl = merge_star(tsl, cats);

	if (cats->perr < 0.2) {
		degrees_to_dms_pr(ras, cats->ra / 15.0, 3);
		degrees_to_dms_pr(decs, cats->dec, 2);
	} else {
		degrees_to_dms_pr(ras, cats->ra / 15.0, 2);
		degrees_to_dms_pr(decs, cats->dec, 1);
	}
	st = stf_append_string(NULL, SYM_OBJECT, cats->name);
	stf_append_string(st, SYM_RA, ras);
	stf_append_string(st, SYM_DEC, decs);
	stf_append_double(st, SYM_EQUINOX, cats->equinox);
//	stf_append_string(st, SYM_COMMENTS, "generated from tycho2");

	stf = stf_append_list(NULL, SYM_RECIPE, st);
//	stf_append_string(stf, SYM_SEQUENCE, "tycho2");
	st = stf_append_glist(stf, SYM_STARS, tsl);

	stf_fprint(outf, stf, 0, 0);

	cat_star_release(cats);
	fflush(outf);
	return 0;
}

static void delete_cdsquery(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_return_if_fail(data != NULL);
	gtk_object_set_data(GTK_OBJECT(data), "cdsquery", NULL);
}

static int logw_print(char *msg, void *data)
{
	GtkWidget *logw = data;
	GtkWidget *text;
	GtkToggleButton *stopb;

	text = gtk_object_get_data(GTK_OBJECT(logw), "query_log_text");
	g_return_val_if_fail(text != NULL, 0);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text), GTK_WRAP_CHAR);

	gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
					 msg, -1);

	while (gtk_events_pending())
		gtk_main_iteration();

	stopb = gtk_object_get_data(GTK_OBJECT(logw), "query_stop_toggle");
	if (gtk_toggle_button_get_active(stopb)) {
		gtk_toggle_button_set_active(stopb, 0);
		return 1;
	}
	return 0;
}


void cds_query_cb(gpointer window, guint action, GtkWidget *menu_item)
{
	GtkWidget *logw;
	struct wcs *wcs;
	struct image_channel *i_ch;
	double w, h;
	GList *tsl = NULL;

	g_return_if_fail(action < QUERY_CATALOGS);

	i_ch = gtk_object_get_data(GTK_OBJECT(window), "i_channel");

	if (i_ch == NULL || i_ch->fr == NULL) {
		err_printf_sb2(window, "Load a frame or create a new one before loading stars");
		error_beep();
		return;
	}
	wcs = gtk_object_get_data(GTK_OBJECT(window), "wcs_of_window");
	if (wcs == NULL || wcs->wcsset == WCS_INVALID) {
		err_printf_sb2(window, "Set an initial WCS before loading stars");
		error_beep();
		return;
	}
	w = 60.0*fabs(i_ch->fr->w * wcs->xinc);
	h = 60.0*fabs(i_ch->fr->h * wcs->yinc);
	clamp_double(&w, 1.0, 2 * P_DBL(QUERY_MAX_RADIUS));
	clamp_double(&h, 1.0, 2 * P_DBL(QUERY_MAX_RADIUS));

	logw = create_query_log_window();
	gtk_object_set_data_full(GTK_OBJECT(window), "cdsquery",
				 logw, (GtkDestroyNotify)(gtk_widget_destroy));
	gtk_object_set_data(GTK_OBJECT(logw), "im_window", window);
	gtk_signal_connect (GTK_OBJECT (logw), "delete_event",
			    GTK_SIGNAL_FUNC (delete_cdsquery), window);
	gtk_widget_show(logw);

	tsl = query_catalog(query_catalog_names[action], wcs->xref, wcs->yref,
			    w, h, logw_print, logw);

	info_printf_sb2(window, "Received %d %s stars", g_list_length(tsl),
			query_catalog_names[action]);
	gtk_object_set_data(GTK_OBJECT(window), "cdsquery", NULL);

	merge_cat_star_list_to_window(window, tsl);
	gtk_widget_queue_draw(window);
	g_list_free(tsl);
}
