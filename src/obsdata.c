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

/* Observation data handling */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "gcx.h"
#include "catalogs.h"
#include "obsdata.h"
#include "params.h"

#include "sidereal_time.h"

/* extract the catalog name (the part before ":") from the name and return 
 * a pointer to it (a terminating 0 is added). Also update name
 * to point to the object name proper */
char *extract_catname(char *text, char **oname)
{
	char *catn = NULL, *n;
	n = text;
	while (*n != 0) {
		if (!isspace(*n)) {
			catn = n;
			break;
		}
		n++;
	}
	while (*n != 0) {
		if (*n == ':') {
			break;
		}
		n++;
	}
	if (*n == 0) {
		*oname = catn;
		return NULL;
	} else {
		*n = 0;
		n++;
	}
	while (*n != 0) {
		if (!isspace(*n)) {
			break;
		}
		n++;
	}
	*oname = n;
	return catn;
}

/* replace a malloced string with the argument */
void replace_strval(char **str, char *val)
{
	if (*str)
		free(*str);
	*str = val;
}

/* get a catalog object cats. the object is specified by 
 * a "catalog:name" string. If the catalog part is not present, 
 * edb is searched.
 * return cats if successfull, NULL if some error occured.
 */

struct cat_star *get_object_by_name(char *name)
{
	char *catn, *oname;
	struct catalog *cat;
	struct cat_star *cats;
	int ret;

	if (name == NULL)
		return NULL;
	catn = extract_catname(name, &oname);
	d3_printf("catn '%s' name '%s'\n", catn, oname);
	if (oname == NULL)
		return NULL;
	if (catn == NULL) {
		cat = open_catalog("local");
		if (cat == NULL)
			return NULL;
		if (cat->cat_get == NULL)
			return NULL;
		ret = (*(cat->cat_get))(&cats, cat, oname, 1);
		close_catalog(cat);
		if (ret > 0) {
			return cats;
		}
		cat = open_catalog("edb");
		if (cat == NULL)
			return NULL;
		if (cat->cat_get == NULL)
			return NULL;
		ret = (*(cat->cat_get))(&cats, cat, oname, 1);
		close_catalog(cat);
	} else {
		cat = open_catalog(catn);
		if (cat == NULL)
			return NULL;
		if (cat->cat_get == NULL)
			return NULL;
		ret = (*(cat->cat_get))(&cats, cat, oname, 1);
		close_catalog(cat);
	}
	if (ret <= 0)
		return NULL;
	return cats;
}


/* get a catalog object into the obs. the object is specified by 
 * a "catalog:name" string. If the catalog part is not present, 
 * edb is searched.
 * return 0 if successfull, -1 if some error occured.
 */
int obs_set_from_object(struct obs_data *obs, char *name)
{
	struct cat_star *cats;
	cats = get_object_by_name(name);
	if (cats == NULL)
		return -1;
	obs->ra = cats->ra;
	obs->dec = cats->dec;
	obs->equinox = cats->equinox;
	replace_strval(&(obs->objname), strdup(name));
	cat_star_release(cats);
	return 0;
}

/* object creation/destruction */
struct obs_data * obs_data_new(void)
{
	struct obs_data *obs;
	obs = calloc(1, sizeof(struct obs_data));
	if (obs == NULL)
		return NULL;
	obs->ref_count = 1;
	obs->rot = 0.0;
	return obs;
}

void obs_data_ref(struct obs_data *obs)
{
	if (obs == NULL)
		return;
	obs->ref_count ++;
}

void obs_data_release(struct obs_data *obs)
{
	if (obs == NULL)
		return;
	if (obs->ref_count < 1)
		err_printf("obs_data_release: obs_data has ref_count of %d\n", obs->ref_count);
	if (obs->ref_count <= 1) {
		if (obs->objname)
			free(obs->objname);
		if (obs->filter)
			free(obs->filter);
		if (obs->comment)
			free(obs->filter);
		free(obs);
	} else {
		obs->ref_count --;
	}
}

/* Add obs and information to the frame */
/* If obs is null, only the global obs data (from PAR) is added */

void ccd_frame_add_obs_info(struct ccd_frame *fr, struct obs_data *obs)
{
	char lb[128];
	char deg[64];

	if (obs != NULL) {
		if (obs->ra != 0.0 || obs->rot != 0.0 || obs->dec != 0.0) {
			fr->fim.xref = obs->ra;
			fr->fim.yref = obs->dec;
			fr->fim.rot = obs->rot;
			fr->fim.wcsset = WCS_INITIAL;

			degrees_to_dms_pr(deg, obs->ra / 15.0, 2);
			sprintf(lb, "'%s'", deg);
			fits_add_keyword(fr, P_STR(FN_RA), lb);

			degrees_to_dms_pr(deg, obs->dec, 1);
			sprintf(lb, "'%s'", deg);
			fits_add_keyword(fr, P_STR(FN_DEC), lb);
		}
		fr->fim.equinox = obs->equinox;

		if (obs->filter != NULL) {
			snprintf(lb, 127, "'%s'", obs->filter);
			fits_add_keyword(fr, P_STR(FN_FILTER), lb);
		}
		if (obs->objname != NULL) {
			snprintf(lb, 127, "'%s'", obs->objname);
			fits_add_keyword(fr, P_STR(FN_OBJECT), lb);
		}
		degrees_to_dms_pr(deg, obs->ra / 15.0, 2);
		sprintf(lb, "'%s'", deg);
		fits_add_keyword(fr, P_STR(FN_OBJCTRA), lb);

		degrees_to_dms_pr(deg, obs->dec, 1);
		sprintf(lb, "'%s'", deg);
		fits_add_keyword(fr, P_STR(FN_OBJCTDEC), lb);

		sprintf(lb, "%20.1f / EQUINOX OF COORDINATES", obs->equinox);
		fits_add_keyword(fr, P_STR(FN_EQUINOX), lb);
	}

	snprintf(lb, 127, "'%s'", P_STR(OBS_TELESCOP));
	fits_add_keyword(fr, P_STR(FN_TELESCOP), lb);

	snprintf(lb, 127, "'%s'", P_STR(OBS_FOCUS));
	fits_add_keyword(fr, P_STR(FN_FOCUS), lb);

	sprintf(lb, "%20.1f / TELESCOPE APERTURE IN CM", P_DBL(OBS_APERTURE));
	fits_add_keyword(fr, P_STR(FN_APERTURE), lb);

	sprintf(lb, "%20.1f / TELESCOPE FOCAL LENGTH IN CM", P_DBL(OBS_FLEN));
	fits_add_keyword(fr, P_STR(FN_FLEN), lb);

	snprintf(lb, 127, "'%s'", P_STR(OBS_OBSERVER));
	fits_add_keyword(fr, P_STR(FN_OBSERVER), lb);

	degrees_to_dms_pr(deg, P_DBL(OBS_LATITUDE), 1);
	sprintf(lb, "'%s'", deg);
	fits_add_keyword(fr, P_STR(FN_LATITUDE), lb);

	degrees_to_dms_pr(deg, P_DBL(OBS_LONGITUDE), 1);
	sprintf(lb, "'%s'                         ", deg);
	sprintf(lb+20, " / WESTERN LONGITUDE");
	fits_add_keyword(fr, P_STR(FN_LONGITUDE), lb);

	sprintf(lb, "%20.1f / OBSERVING SITE ALTITUDE (M)", P_DBL(OBS_ALTITUDE));
	fits_add_keyword(fr, P_STR(FN_ALTITUDE), lb);

}

/* look for CD_ keywords; return 1 if found (and update fim) */
static int scan_for_CD(struct ccd_frame *fr, struct wcs *fim)
{
	double cd[2][2];
	int ret = 0;
	double D, ra, rb, r;

	if (fits_get_double(fr, "CD1_1", &cd[0][0]) > 0) {
		ret = 1;
	} else {
		cd[0][0] = 0;
	}
	if (fits_get_double(fr, "CD1_2", &cd[0][1]) > 0) {
		ret = 1;
	} else {
		cd[0][1] = 0;
	}
	if (fits_get_double(fr, "CD2_1", &cd[1][0]) > 0) {
		ret = 1;
	} else {
		cd[1][0] = 0;
	}
	if (fits_get_double(fr, "CD2_2", &cd[1][1]) > 0) {
		ret = 1;
	} else {
		cd[1][1] = 0;
	}

	if (!ret)
		return 0;

	D = cd[0][0] * cd[1][1] - cd[1][0] * cd[0][1];
	d4_printf("CD D=%8g\n", D);
	if (fabs(D) < 1e-30) {
		err_printf("CD matrix is singular!\n");
		return 0;
	}

	if (cd[1][0] > 0) {
		ra = atan2(cd[1][0], cd[0][0]);
	} else if (cd[1][0] < 0) {
		ra = atan2(-cd[1][0], -cd[0][0]);
	} else {
		ra = 0;
	}

	if (cd[0][1] > 0) {
		rb = atan2(cd[0][1], -cd[1][1]);
	} else if (cd[0][1] < 0) {
		rb = atan2(-cd[0][1], cd[1][1]);
	} else {
		rb = 0;
	}

	r = (ra + rb) / 2;

	d4_printf("ra=%.8g, rb=%.8g, r=%.8g\n", raddeg(ra), raddeg(rb), raddeg(r));

	if (fabs(cos(r)) < 1e-30) {
		err_printf("90.000000 degrees rotation!\n");
		return 0;
	}

	D = sqrt((sqr(cd[0][0]) + sqr(cd[1][1])) / 2);

	fim->xinc = cd[0][0] > 0 ? D / cos(r) : -D / cos(r);
	fim->yinc = cd[1][1] > 0 ? D / cos(r) : -D / cos(r);

	fim->pc[0][0] = cd[0][0] / fim->xinc;
	fim->pc[1][0] = cd[1][0] / fim->yinc;
	fim->pc[0][1] = cd[0][1] / fim->xinc;
	fim->pc[1][1] = cd[1][1] / fim->yinc;
	fim->rot = raddeg(r);

	d4_printf("pc   %.8g %.8g\n", fim->pc[0][0], fim->pc[0][1]);
	d4_printf("pc   %.8g %.8g\n", fim->pc[1][0], fim->pc[1][1]);

	if (ret) {
		fim->flags |= WCS_USE_LIN;
	}

	return 1;

}

/* look for PC_ keywords; return 1 if found (and update fim) */
static int scan_for_PC(struct ccd_frame *fr, struct wcs *fim)
{
	double pc[2][2];
	int ret = 0;
	double D;

	if (fits_get_double(fr, P_STR(FN_CROTA1), &fim->rot) <= 0)
		fim->rot = 0;

	if ( (fits_get_double(fr, P_STR(FN_CDELT1), &fim->xinc) <= 0) ||
	     (fits_get_double(fr, P_STR(FN_CDELT2), &fim->yinc) <= 0) ) {
		return 0;
	}

	if (fits_get_double(fr, "PC1_1", &pc[0][0]) > 0) {
		ret = 1;
	} else {
		pc[0][0] = 1;
	}
	if (fits_get_double(fr, "PC1_2", &pc[0][1]) > 0) {
		ret = 1;
	} else {
		pc[0][1] = 0;
	}
	if (fits_get_double(fr, "PC2_1", &pc[1][0]) > 0) {
		ret = 1;
	} else {
		pc[1][0] = 0;
	}
	if (fits_get_double(fr, "PC2_2", &pc[1][1]) > 0) {
		ret = 1;
	} else {
		pc[1][1] = 1;
	}


	D = pc[0][0] * pc[1][1] - pc[1][0] * pc[0][1];
	d4_printf("PC D=%8g\n", D);
	if (fabs(D) < 1e-30) {
		err_printf("PC matrix is singular!\n");
		return 0;
	}

	if (ret) {
//		fim->flags |= WCS_USE_LIN;
	}

	fim->pc[0][0] = pc[0][0];
	fim->pc[1][0] = pc[1][0];
	fim->pc[0][1] = pc[0][1];
	fim->pc[1][1] = pc[1][1];
	fim->rot = raddeg(atan2(pc[0][1], pc[0][0]));

	return 1;
}


/* read the wcs fields from the fits header lines
 * using parametrised field names */

void rescan_fits_wcs(struct ccd_frame *fr, struct wcs *fim)
{
	g_return_if_fail(fr != NULL);
	g_return_if_fail(fim != NULL);

	fim->wcsset = WCS_INITIAL;
	fim->flags &= ~WCS_HINTED;
	fim->flags &= ~WCS_USE_LIN;

	if (fits_get_double(fr, P_STR(FN_CRPIX1), &fim->xrefpix) <= 0)
		fim->wcsset = WCS_INVALID;
	if (fits_get_double(fr, P_STR(FN_CRPIX2), &fim->yrefpix) <= 0)
		fim->wcsset = WCS_INVALID;
	if (fits_get_double(fr, P_STR(FN_CRVAL1), &fim->xref) <= 0)
		fim->wcsset = WCS_INVALID;
	if (fits_get_double(fr, P_STR(FN_CRVAL2), &fim->yref) <= 0)
		fim->wcsset = WCS_INVALID;
	if (fits_get_double(fr, P_STR(FN_EQUINOX), &fim->equinox) <= 0) 
		fim->equinox = 2000;
	if (!scan_for_CD(fr, fim))
		if (!scan_for_PC(fr, fim)) 
			fim->wcsset = WCS_INVALID;

	d3_printf("wcs status is: %d\n", fim->wcsset);
}

/* read the exp fields from the fits header lines
 * using parametrised field names */
void rescan_fits_exp(struct ccd_frame *fr, struct exp_data *exp)
{
	int np = 0;
	g_return_if_fail(fr != NULL);
	g_return_if_fail(exp != NULL);

	if (fits_get_int(fr, P_STR(FN_BINX), &exp->bin_x) > 0)
		exp->datavalid = 1;
	if (fits_get_int(fr, P_STR(FN_BINY), &exp->bin_y) > 0)
		exp->datavalid = 1;
	if (fits_get_double(fr, P_STR(FN_ELADU), &exp->scale) > 0) {
		d1_printf("using eladu=%.1f from %s\n", exp->scale, P_STR(FN_ELADU));
		np ++;
	}
	if (fits_get_double(fr, P_STR(FN_RDNOISE), &exp->rdnoise) > 0) {
		d1_printf("using rdnoise=%.1f from %s\n", exp->rdnoise, P_STR(FN_RDNOISE));
		np ++;
	}
	if (fits_get_double(fr, P_STR(FN_FLNOISE), &exp->flat_noise) > 0) {
		d1_printf("using flnoise=%.1f from %s\n", exp->flat_noise, P_STR(FN_FLNOISE));
	}
	exp->bias = 0;
	if (fits_get_double(fr, P_STR(FN_DCBIAS), &exp->bias) > 0) {
		d1_printf("using dcbias=%.1f from %s\n", exp->bias, P_STR(FN_DCBIAS));
	}
//	fits_get_double(fr, P_STR(FN_AIRMASS), &exp->airmass);
	if (fits_get_int(fr, P_STR(FN_SKIPX), &fr->x_skip) <= 0)
		fr->x_skip = 0;
	if (fits_get_int(fr, P_STR(FN_SKIPY), &fr->y_skip) <= 0)
		fr->y_skip = 0;
	if (np < 2) {
		info_printf("Warning: using default values for noise model\n");
	}

}

/* push the noise data from exp into the header fields of fr */
void noise_to_fits_header(struct ccd_frame *fr, struct exp_data *exp)
{
	char lb[128];
	sprintf(lb, "%20.3f / ELECTRONS / ADU", exp->scale);
	fits_add_keyword(fr, P_STR(FN_ELADU), lb);
	sprintf(lb, "%20.3f / READ NOISE IN ADUs", exp->rdnoise);
	fits_add_keyword(fr, P_STR(FN_RDNOISE), lb);
	sprintf(lb, "%20.3f / MULTIPLICATIVE NOISE COEFF", exp->flat_noise);
	fits_add_keyword(fr, P_STR(FN_FLNOISE), lb);
	sprintf(lb, "%20.3f / BIAS IN ADUs", exp->bias);
	fits_add_keyword(fr, P_STR(FN_DCBIAS), lb);
}


/* create a jdate from the calendar date and fractional hours.
 m is between 1 and 12, d between 1 and 31.*/
double make_jdate(int y, int m, int d, double hours)
{
	int jdi, i, j, k;
	double jd;

	if (y < 100)
		y += 1900;

// do the julian date (per hsaa p107)
	i = y;
	j = m;
	k = d;
	jdi = k - 32075 + 1461 * (i + 4800 + (j - 14) / 12) / 4
		+ 367 * (j - 2 - (j - 14) / 12 * 12) / 12
		- 3 * (( i + 4900 + (j - 14) / 12) / 100) / 4;

	jd = jdi - 0.5 + hours / 24.0;
	return jd;
}

/* try to get the jdate from the header fields */
double frame_jdate(struct ccd_frame *fr)
{
	double v;
	char date[64];
	char *time;
	int y, m, d;
	double hours;

	if (fits_get_string(fr, P_STR(FN_DATE_OBS), date, 63) <= 0) {
		if (fits_get_double(fr, P_STR(FN_MJD), &v) > 0) {
			d1_printf("using mjd=%.14g from %s\n", v, P_STR(FN_MJD));
			return mjd_to_jd(v);
		} else if (fits_get_double(fr, P_STR(FN_JDATE), &v) > 0) {
			d1_printf("using jdate=%.14g from %s\n", v, P_STR(FN_JDATE));
			return v;
		} else {
			err_printf("no date/time found in %s, %s or %s\n",
				   P_STR(FN_DATE_OBS), P_STR(FN_MJD), P_STR(FN_JDATE));
			return 0.0;
		}		
	} else {
		d1_printf("using %s='%s' for date/time\n", P_STR(FN_DATE_OBS), date);
		if ((sscanf(date, "%d-%d-%d", &y, &m, &d) != 3) &&
		    (sscanf(date, "%d/%d/%d", &y, &m, &d) != 3)) {
			err_printf("error parsing date\n");
			return 0.0;
		}
		time = strstr(date, "T");
		if (time != NULL && time[0] != 0) {
			d3_printf("Time string \n");
			d3_printf("is: '%s'\n", time+1);
			if (dms_to_degrees(time+1, &hours)) {
				err_printf("error_parsing time %s, using partial date\n",
					   time+1);
				return make_jdate(y, m, d, 0.0);
			} else {
				return make_jdate(y, m, d, hours);
			}
		} else {
			d3_printf("Looking for TIME_OBS \n");
			d3_printf("TIME_OBS field is %s\n", P_STR(FN_TIME_OBS));
			if (fits_get_string(fr, P_STR(FN_TIME_OBS), date, 63) <= 0) {
				err_printf("no time info found in %s\n", P_STR(FN_TIME_OBS));
				return make_jdate(y, m, d, 0.0);
			} else {
				d1_printf("using %s='%s' for time\n", P_STR(FN_TIME_OBS), date);
				if (dms_to_degrees(date, &hours)) {
					err_printf("error_parsing time %s, "
						   "using partial date\n", date);
					return make_jdate(y, m, d, 0.0);
				} else {
					return make_jdate(y, m, d, hours);
				}
			}
		}
	}
}

void date_time_from_jdate(double jd, char *date, int n)
{
	struct timeval tv;
	struct tm *t;

	jdate_to_timeval(jd, &tv);
	t = gmtime((time_t *)&((tv.tv_sec)));

	snprintf(date, n, "'%d-%02d-%02dT%02d:%02d:%05.2f'", 1900 + t->tm_year, t->tm_mon + 1, 
		t->tm_mday, t->tm_hour, t->tm_min, 1.0 * t->tm_sec +
		1.0 * tv.tv_usec / 1000000);
	
}


double calculate_airmass(double ra, double dec, double jd, double lat, double lng)
{
	double alt, az, airm;

	get_hrz_from_equ_sidereal_time (ra, dec, lng, lat,
					get_apparent_sidereal_time(jd),
					&alt, &az);

	airm = airmass(alt);
	airm = 0.001 * floor(airm * 1000);

//	d3_printf("alt=%.4f az=%.4f sidereal=%.4f airmass=%.4f\n", alt, az, 
//		  get_apparent_sidereal_time(jd) - lng/15, airm);
	return airm;
} 

/* try to compute airmass from the frame header information 
 * return 0.0 if airmass couldn't be calculated */
double frame_airmass(struct ccd_frame *fr, double ra, double dec) 
{
	double v;
	double lat, lng, jd;
	char dms[64];

	if (fits_get_double(fr, P_STR(FN_AIRMASS), &v) > 0) {
		return v;
	}

	if (fits_get_string(fr, P_STR(FN_LATITUDE), dms, 63) <= 0) 
		return 0.0;
	if (dms_to_degrees(dms, &lat))
		return 0.0;

	if (fits_get_string(fr, P_STR(FN_LONGITUDE), dms, 63) <= 0) 
		return 0.0;
	if (dms_to_degrees(dms, &lng))
		return 0.0;

	if (fits_get_double(fr, P_STR(FN_MJD), &v) > 0) {
		jd = mjd_to_jd(v);
	} else if (fits_get_double(fr, P_STR(FN_JDATE), &v) > 0) {
		jd = v;
	} else {
		return 0.0;
	}

	return calculate_airmass(ra, dec, jd, lat, lng);
}

double timeval_to_jdate(struct timeval *tv)
{
	struct tm *t;
	double jd;


	t = gmtime((time_t *)&(tv->tv_sec));
// do the julian date (per hsaa p107)
	jd = make_jdate(1900 + t->tm_year, t->tm_mon + 1, t->tm_mday, 
			t->tm_hour + t->tm_min / (60.0) 
			+ t->tm_sec / (3600.0) + tv->tv_usec / 
			(1000000.0 * 3600.0));
	return jd;
}

void jdate_to_timeval(double jd, struct timeval *tv)
{
	struct timeval btv;
	double bjd;
	long dsec, dusec;

	btv.tv_sec = 0;
	btv.tv_usec = 0;

	bjd = timeval_to_jdate(&btv);

	if (bjd > jd) {
		err_printf("jdate_to_timeval: trying to convert jdate:%.14g "
			   "from before Jan1, 1970\n", jd);
		return;
	}
	dsec = (jd - bjd) * 86400 + 0.5;
	dusec = ((jd - bjd) * 86400 - dsec) * 1000000;
	tv->tv_sec = dsec;
	tv->tv_usec = dusec;
}



/* return the hour angle for the given obs coordinates, 
 * uses the parameter geo location values and current time */
double obs_current_hour_angle(struct obs_data *obs)
{
	double jd, ha;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	jd = timeval_to_jdate(&tv);

	ha = get_apparent_sidereal_time(jd) - (P_DBL(OBS_LONGITUDE) + obs->ra) / 15.0;
	if (ha > 12)
		ha -= 24;
	d3_printf("current ha = %f\n", ha);
	return ha;
}

/* return the airmass for the given obs coordinates, 
 * uses the parameter geo location values and current time */
double obs_current_airmass(struct obs_data *obs)
{
	double jd, airm;
	struct timeval tv;
	double alt, az;

	gettimeofday(&tv, NULL);
	jd = timeval_to_jdate(&tv);

	get_hrz_from_equ_sidereal_time (obs->ra, obs->dec, P_DBL(OBS_LONGITUDE), 
					P_DBL(OBS_LATITUDE),
					get_apparent_sidereal_time(jd),
					&alt, &az);
	airm = airmass(alt);
	d3_printf("current airmass = %f\n", airm);
	return airm;
}

void wcs_to_fits_header(struct ccd_frame *fr)
{
	char lb[80];
	char deg[80];
	if (fr->fim.wcsset <= WCS_INITIAL) 
		return;

	fits_add_keyword(fr, "CTYPE1", "'RA---TAN'");
	fits_add_keyword(fr, "CTYPE2", "'DEC--TAN'");

	sprintf(lb, "%20.8f / ", fr->fim.xref);
	fits_add_keyword(fr, P_STR(FN_CRVAL1), lb);

	sprintf(lb, "%20.8f / ", fr->fim.yref);
	fits_add_keyword(fr, P_STR(FN_CRVAL2), lb);

	sprintf(lb, "%20.8f / ", fr->fim.xinc);
	fits_add_keyword(fr, P_STR(FN_CDELT1), lb);

	sprintf(lb, "%20.8f / ", fr->fim.yinc);
	fits_add_keyword(fr, P_STR(FN_CDELT2), lb);

	sprintf(lb, "%20.8f / ", fr->fim.xrefpix);
	fits_add_keyword(fr, P_STR(FN_CRPIX1), lb);

	sprintf(lb, "%20.8f / ", fr->fim.yrefpix);
	fits_add_keyword(fr, P_STR(FN_CRPIX2), lb);

	sprintf(lb, "%20.8f / ", fr->fim.rot);
	fits_add_keyword(fr, P_STR(FN_CROTA1), lb);

	sprintf(lb, "%20.1f / ", 1.0 * fr->fim.equinox);
	fits_add_keyword(fr, P_STR(FN_EQUINOX), lb);

	degrees_to_dms_pr(deg, fr->fim.xref / 15.0, 2);
	sprintf(lb, "'%s'", deg);
	fits_add_keyword(fr, P_STR(FN_RA), lb);

	degrees_to_dms_pr(deg, fr->fim.yref, 1);
	sprintf(lb, "'%s'", deg);
	fits_add_keyword(fr, P_STR(FN_DEC), lb);

	sprintf(lb, "%20.8f / IMAGE SCALE IN SECONDS PER PIXEL", 
		3600.0 * fabs(fr->fim.xinc));
	fits_add_keyword(fr, P_STR(FN_SECPIX), lb);

}
