/* astro.c -- astronomical calculations

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>

Copyright (C) 2015 Matei Conovici <mconovici@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <math.h>

#include "astro.h"


/* puts a large angle in the correct range 0 - 360 degrees */
double
range_degrees (double angle)
{
	double temp;
	double range;

	if (angle >= 0.0 && angle < 360.0)
		return angle;

	temp = (int)(angle / 360);
	if (angle < 0.0)
		temp --;

	temp *= 360;
	range = angle - temp;

	return range;
}


#define TERMS 63
#define LN_NUTATION_EPOCH_THRESHOLD 0.1

struct astro_nutation_arguments
{
	double D;
	double M;
	double MM;
	double F;
	double O;
};

struct astro_nutation_coefficients
{
	double longitude1;
	double longitude2;
	double obliquity1;
	double obliquity2;
};

/* arguments and coefficients taken from table 21A on page 133 */

const static struct astro_nutation_arguments nutation_arguments[TERMS] = {
    {0.0,	0.0,	0.0,	0.0,	1.0},
    {-2.0,	0.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	0.0,	2.0},
    {0.0,	1.0,	0.0,	0.0,	0.0},
    {0.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	1.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	2.0,	1.0},
    {0.0,	0.0,	1.0,	2.0,	2.0},
    {-2.0,	-1.0,	0.0,	2.0,	2.0},
    {-2.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	0.0,	0.0,	2.0,	1.0},
    {0.0,	0.0,	-1.0,	2.0,	2.0},
    {2.0,	0.0,	0.0,	0.0,	0.0},
    {0.0,	0.0,	1.0,	0.0,	1.0},
    {2.0,	0.0,	-1.0,	2.0,	2.0},
    {0.0,	0.0,	-1.0,	0.0,	1.0},
    {0.0,	0.0,	1.0,	2.0,	1.0},
    {-2.0,	0.0,	2.0,	0.0,	0.0},
    {0.0,	0.0,	-2.0,	2.0,	1.0},
    {2.0,	0.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	2.0,	2.0,	2.0},
    {0.0,	0.0,	2.0,	0.0,	0.0},
    {-2.0,	0.0,	1.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	2.0,	0.0},
    {-2.0,	0.0,	0.0,	2.0,	0.0},
    {0.0,	0.0,	-1.0,	2.0,	1.0},
    {0.0,	2.0,	0.0,	0.0,	0.0},
    {2.0,	0.0,	-1.0,	0.0,	1.0},
    {-2.0,	2.0,	0.0,	2.0,	2.0},
    {0.0,	1.0,	0.0,	0.0,	1.0},
    {-2.0,	0.0,	1.0,	0.0,	1.0},
    {0.0,	-1.0,	0.0,	0.0,	1.0},
    {0.0,	0.0,	2.0,	-2.0,	0.0},
    {2.0,	0.0,	-1.0,	2.0,	1.0},
    {2.0,	0.0,	1.0,	2.0,	2.0},
    {0.0,	1.0,	0.0,	2.0,	2.0},
    {-2.0,	1.0,	1.0,	0.0,	0.0},
    {0.0,	-1.0,	0.0,	2.0,	2.0},
    {2.0,	0.0,	0.0,	2.0,	1.0},
    {2.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	0.0,	2.0,	2.0,	2.0},
    {-2.0,	0.0,	1.0,	2.0,	1.0},
    {2.0,	0.0,	-2.0,	0.0,	1.0},
    {2.0,	0.0,	0.0,	0.0,	1.0},
    {0.0,	-1.0,	1.0,	0.0,	0.0},
    {-2.0,	-1.0,	0.0,	2.0,	1.0},
    {-2.0,	0.0,	0.0,	0.0,	1.0},
    {0.0,	0.0,	2.0,	2.0,	1.0},
    {-2.0,	0.0,	2.0,	0.0,	1.0},
    {-2.0,	1.0,	0.0,	2.0,	1.0},
    {0.0,	0.0,	1.0,	-2.0,	0.0},
    {-1.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	1.0,	0.0,	0.0,	0.0},
    {1.0,	0.0,	0.0,	0.0,	0.0},
    {0.0,	0.0,	1.0,	2.0,	0.0},
    {0.0,	0.0,	-2.0,	2.0,	2.0},
    {-1.0,	-1.0,	1.0,	0.0,	0.0},
    {0.0,	1.0,	1.0,	0.0,	0.0},
    {0.0,	-1.0,	1.0,	2.0,	2.0},
    {2.0,	-1.0,	-1.0,	2.0,	2.0},
    {0.0,	0.0,	3.0,	2.0,	2.0},
    {2.0,	-1.0,	0.0,	2.0,	2.0}};

const static struct astro_nutation_coefficients nutation_coefficients[TERMS] = {
    {-171996.0,	-174.2,	92025.0,8.9},
    {-13187.0,	-1.6,  	5736.0,	-3.1},
    {-2274.0, 	 0.2,  	977.0,	-0.5},
    {2062.0,   	0.2,    -895.0,    0.5},
    {1426.0,    -3.4,    54.0,    -0.1},
    {712.0,    0.1,    -7.0,    0.0},
    {-517.0,    1.2,    224.0,    -0.6},
    {-386.0,    -0.4,    200.0,    0.0},
    {-301.0,    0.0,    129.0,    -0.1},
    {217.0,    -0.5,    -95.0,    0.3},
    {-158.0,    0.0,    0.0,    0.0},
    {129.0,	0.1,	-70.0,	0.0},
    {123.0,	0.0,	-53.0,	0.0},
    {63.0,	0.0,	0.0,	0.0},
    {63.0,	1.0,	-33.0,	0.0},
    {-59.0,	0.0,	26.0,	0.0},
    {-58.0,	-0.1,	32.0,	0.0},
    {-51.0,	0.0,	27.0,	0.0},
    {48.0,	0.0,	0.0,	0.0},
    {46.0,	0.0,	-24.0,	0.0},
    {-38.0,	0.0,	16.0,	0.0},
    {-31.0,	0.0,	13.0,	0.0},
    {29.0,	0.0,	0.0,	0.0},
    {29.0,	0.0,	-12.0,	0.0},
    {26.0,	0.0,	0.0,	0.0},
    {-22.0,	0.0,	0.0,	0.0},
    {21.0,	0.0,	-10.0,	0.0},
    {17.0,	-0.1,	0.0,	0.0},
    {16.0,	0.0,	-8.0,	0.0},
    {-16.0,	0.1,	7.0,	0.0},
    {-15.0,	0.0,	9.0,	0.0},
    {-13.0,	0.0,	7.0,	0.0},
    {-12.0,	0.0,	6.0,	0.0},
    {11.0,	0.0,	0.0,	0.0},
    {-10.0,	0.0,	5.0,	0.0},
    {-8.0,	0.0,	3.0,	0.0},
    {7.0,	0.0,	-3.0,	0.0},
    {-7.0,	0.0,	0.0,	0.0},
    {-7.0,	0.0,	3.0,	0.0},
    {-7.0,	0.0,	3.0,	0.0},
    {6.0,	0.0,	0.0,	0.0},
    {6.0,	0.0,	-3.0,	0.0},
    {6.0,	0.0,	-3.0,	0.0},
    {-6.0,	0.0,	3.0,	0.0},
    {-6.0,	0.0,	3.0,	0.0},
    {5.0,	0.0,	0.0,	0.0},
    {-5.0,	0.0,	3.0,	0.0},
    {-5.0,	0.0,	3.0,	0.0},
    {-5.0,	0.0,	3.0,	0.0},
    {4.0,	0.0,	0.0,	0.0},
    {4.0,	0.0,	0.0,	0.0},
    {4.0,	0.0,	0.0,	0.0},
    {-4.0,	0.0,	0.0,	0.0},
    {-4.0,	0.0,	0.0,	0.0},
    {-4.0,	0.0,	0.0,	0.0},
    {3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0}};

/* nutation cache values */
static double c_JD = 0.0, c_longitude = 0.0, c_obliquity = 0.0, c_ecliptic = 0.0;

/* Calculate nutation of longitude and obliquity in degrees from Julian Ephemeris Day */
/* Chapter 21 pg 131-134 Using Table 21A */

static void
astro_get_nutation (double JD, double *longitude, double *obliquity, double *ecliptic)
{

	double D,M,MM,F,O,T,T2,T3;
	double coeff_sine, coeff_cos;
	int i;

	/* should we bother recalculating nutation */
	if (fabs(JD - c_JD) > LN_NUTATION_EPOCH_THRESHOLD)
	{
		/* set the new epoch */
		c_JD = JD;

		/* set ecliptic */
		c_ecliptic = 23.0 + 26.0 / 60.0 + 27.407 / 3600.0;

		/* get julian ephemeris day */
		/* JDE = get_jde (JD); Not Sure */

		/* calc T */
		T = (JD - 2451545.0)/36525;
		T2 = T * T;
		T3 = T2 * T;

		/* calculate D,M,M',F and Omega */
		D = 297.85036 + 445267.111480 * T - 0.0019142 * T2 + T3 / 189474.0;
		M = 357.52772 + 35999.050340 * T - 0.0001603 * T2 - T3 / 300000.0;
		MM = 134.96298 + 477198.867398 * T + 0.0086972 * T2 + T3 / 56250.0;
		F = 93.2719100 + 483202.017538 * T - 0.0036825 * T2 + T3 / 327270.0;
		O = 125.04452 - 1934.136261 * T + 0.0020708 * T2 + T3 / 450000.0;

		/* convert to radians */
		D = degrad (D);
		M = degrad (M);
		MM = degrad (MM);
		F = degrad (F);
		O = degrad (O);

		/* calc sum of terms in table 21A */
		for (i=0; i< TERMS; i++)
		{
			/* calc coefficients of sine and cosine */
			coeff_sine = (nutation_coefficients[i].longitude1 + (nutation_coefficients[i].longitude2 * T));
			coeff_cos = (nutation_coefficients[i].obliquity1 + (nutation_coefficients[i].obliquity2 * T));

			/* sum the arguments */
			if (nutation_arguments[i].D != 0)
			{
				c_longitude += coeff_sine * (sin (nutation_arguments[i].D * D));
				c_obliquity += coeff_cos * (cos (nutation_arguments[i].D * D));
			}
			if (nutation_arguments[i].M != 0)
			{
				c_longitude += coeff_sine * (sin (nutation_arguments[i].M * M));
				c_obliquity += coeff_cos * (cos (nutation_arguments[i].M * M));
			}
			if (nutation_arguments[i].MM != 0)
			{
				c_longitude += coeff_sine * (sin (nutation_arguments[i].MM * MM));
				c_obliquity += coeff_cos * (cos (nutation_arguments[i].MM * MM));
			}
			if (nutation_arguments[i].F != 0)
			{
				c_longitude += coeff_sine * (sin (nutation_arguments[i].F * F));
				c_obliquity += coeff_cos * (cos (nutation_arguments[i].F * F));
			}
			if (nutation_arguments[i].O != 0)
			{
				c_longitude += coeff_sine * (sin (nutation_arguments[i].O * O));
				c_obliquity += coeff_cos * (cos (nutation_arguments[i].O * O));
			}
		}

		/* change to arcsecs */
		c_longitude /= 10000;
		c_obliquity /= 10000;

		/* change to degrees */
		c_longitude /= (60 * 60);
		c_obliquity /= (60 * 60);
		c_ecliptic += c_obliquity;
	}

	/* return results */
	*longitude = c_longitude;
	*obliquity = c_obliquity;
	*ecliptic = c_ecliptic;
}


/* Calculate the mean sidereal time at the meridian of Greenwich of a given date. */
/* Formula 11.1, 11.4 pg 83 */

double
astro_get_mean_sidereal_time (double JD)
{
	double sidereal;
	double T;

	T = (JD - 2451545.0) / 36525.0;

	/* calc mean angle */
	sidereal = 280.46061837 + (360.98564736629 * (JD - 2451545.0)) +
		(0.000387933 * T * T) - (T * T * T / 38710000.0);

	/* add a convenient multiple of 360 degrees */
	sidereal = range_degrees(sidereal);

	/* change to hours */
	sidereal *= 24.0 / 360.0;

	return sidereal;
}

/* Calculate the apparent sidereal time at the meridian of Greenwich of a given date. */
/* Formula 11.1, 11.4 pg 83 */
double
astro_get_apparent_sidereal_time (double JD)
{
	double correction, hours, sidereal;
	double longitude, obliquity, ecliptic;

	/* get the mean sidereal time */
	sidereal = astro_get_mean_sidereal_time (JD);

	/* add corrections for nutation in longitude and for the true obliquity of
	   the ecliptic */
	astro_get_nutation (JD, &longitude, &obliquity, &ecliptic);

	correction = (longitude / 15.0 * DCOS(obliquity));

	/* value is in degrees so change it to hours and add to mean sidereal time */
	hours = (24.0 / 360.0) * correction;

	sidereal += hours;

	return sidereal;
}


/* these functions have been hacked by rcorlan to change the parameter types
 * sidereal is the GAST */
void
astro_get_hrz_from_equ_sidereal_time (double objra, double objdec,
				      double lng, double lat,
				      double sidereal, double *alt, double *az)
{
	double H, ra, latitude, declination, A, Ac, As, h, Z, Zs;

	/* change sidereal_time from hours to radians*/
	sidereal *= 2.0 * PI / 24.0;

	/* calculate hour angle of object at observers position */
	ra = degrad (objra);
	H = sidereal - degrad (lng) - ra;

	/* hence formula 12.5 and 12.6 give */
	/* convert to radians - hour angle, observers latitude, object declination */
	latitude = degrad (lat);
	declination = degrad (objdec);

	/* formula 12.6 *; missuse of A (you have been warned) */
	A = sin (latitude) * sin (declination) + cos (latitude) * cos (declination) * cos (H);
	h = asin (A);

	/* covert back to degrees */
	*alt = raddeg (h);

	/* zenith distance, Telescope Control 6.8a */
	Z = acos (A);

	/* is'n there better way to compute that? */
	Zs = sin (Z);

	/* sane check for zenith distance; don't try to divide by 0 */

	if (Zs < 1e-5)
	{
		if (lat > 0)
			*az = 180;
		else
			*az = 0;
		return;
	}

	/* formulas TC 6.8d Taff 1991, pp. 2 and 13 - vector transformations */
	As = (cos (declination) * sin (H)) / Zs;
	Ac = (sin (latitude) * cos (declination) * cos (H) - cos (latitude) *
	      sin (declination)) / Zs;

	// don't blom at atan2
	if (fabs(As) < 1e-5)
	{
		*az = 0;
		return;
	}
	A = atan2 (As, Ac);

	// normalize A
	A = (A < 0) ? 2 * M_PI + A : A;

	/* covert back to degrees */
	*az = range_degrees(raddeg (A));
}

/*
* Transform an objects horizontal coordinates into equatorial coordinates
* for the given GAST and observers position.
*/
void
astro_get_equ_from_hrz_sidereal_time (double alt, double az,
				      double lng, double lat, double sidereal,
				      double *ra, double *dec)
{
	double H, longitude, declination, latitude, A, h;

	/* change observer/object position into radians */

	/* object alt/az */
	A = degrad (az);
	h = degrad (alt);

	/* observer long / lat */
	longitude = degrad (lng);
	latitude = degrad (lat);

	/* equ on pg89 */
	H = atan2 (sin (A), ( cos(A) * sin (latitude) + tan(h) * cos (latitude)));
	declination = sin(latitude) * sin(h) - cos(latitude) * cos(h) * cos(A);
	declination = asin (declination);

	/* get ra = sidereal - longitude + H and change sidereal to radians */
	sidereal *= 2.0 * M_PI / 24.0;

	*ra = raddeg (sidereal - H - longitude);
	*dec = raddeg (declination);
}


/*
* Calculate the adjustment in altitude of a body due to atmosphric
* refraction. This value varies over altitude, pressure and temperature.
*
* Note: Default values for pressure and teperature are 1010 and 10
* respectively.
*/

/* fixed some deg/rad stuff by rcorlan */
double
astro_get_refraction_adj_apparent (double altitude, double atm_pres, double temp)
{
	double R;

	/* equ 16.3 */
	R = 1 / tan(degrad(altitude + (7.31 / (altitude + 4.4))));
	R -= 0.06 * sin(degrad(14.7 * (R / 60.0) + 13.0));

	/* take into account of atm press and temp */
	R *= ((atm_pres / 1010) * (283 / (273 + temp)));

	/* convert from arcminutes to degrees */
	R /= 60.0;

	return R;
}

/* hacked by rcorlan to return the adjustment fron true altitude */
double
astro_get_refraction_adj_true (double altitude, double atm_pres, double temp)
{
	double R;

	/* equ 16.4 */
	R = 1.02 / tan(degrad(altitude + (10.3 / (altitude + 5.11))));

	/* take into account of atm press and temp */
	R *= ((atm_pres / 1010) * (283 / (273 + temp)));

	/* convert from arcminutes to degrees */
	R /= 60.0;

	return R;
}

/*
 * Copyright (c) 1990 by Craig Counterman. All rights reserved.
 *
 * This software may be redistributed freely, not sold.
 * This copyright notice and disclaimer of warranty must remain
 *    unchanged.
 *
 * No representation is made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty, to the extent permitted by applicable law.
 *
 * Rigorous precession. From Astronomical Ephemeris 1989, p. B18
 *
 * 96-06-20 Hayo Hase <hase@wettzell.ifag.de>: theta_a corrected
 */


/* corrects ra and dec, both in degrees, for precession from epoch 1 to epoch 2.
 * the epochs are given by their year numbers, respectively.
 * N.B. ra and dec are modifed IN PLACE.
 */

void
astro_precess_hiprec (double epo1, double epo2, double *ra, double *dec)
{
	double zeta_A, z_A, theta_A;
	double T;
	double A, B, C;
	double alpha, delta;
	double alpha_in, delta_in;
	double from_equinox, to_equinox;
	double alpha2000, delta2000;

	/* convert mjds to years;
	 * avoid the remarkably expensive calls to mjd_year()
	 */
	from_equinox = epo1;
	to_equinox = epo2;

	alpha_in = *ra;
	delta_in = *dec;

	/* precession progresses about 1 arc second in .047 years */
	/* From from_equinox to 2000.0 */
	if (fabs (from_equinox-2000.0) > .02) {
		T = (from_equinox - 2000.0)/100.0;
		zeta_A  = 0.6406161* T + 0.0000839* T*T + 0.0000050* T*T*T;
		z_A     = 0.6406161* T + 0.0003041* T*T + 0.0000051* T*T*T;
		theta_A = 0.5567530* T - 0.0001185* T*T - 0.0000116* T*T*T;

		A = DSIN(alpha_in - z_A) * DCOS(delta_in);
		B = DCOS(alpha_in - z_A) * DCOS(theta_A) * DCOS(delta_in)
			+ DSIN(theta_A) * DSIN(delta_in);
		C = -DCOS(alpha_in - z_A) * DSIN(theta_A) * DCOS(delta_in)
			+ DCOS(theta_A) * DSIN(delta_in);

		alpha2000 = range_degrees (DATAN2(A,B) - zeta_A);
		delta2000 = DASIN(C);
	} else {
		/* should get the same answer, but this could improve accruacy */
		alpha2000 = alpha_in;
		delta2000 = delta_in;
	};


	/* From 2000.0 to to_equinox */
	if (fabs (to_equinox - 2000.0) > .02) {
		T = (to_equinox - 2000.0)/100.0;
		zeta_A  = 0.6406161* T + 0.0000839* T*T + 0.0000050* T*T*T;
		z_A     = 0.6406161* T + 0.0003041* T*T + 0.0000051* T*T*T;
		theta_A = 0.5567530* T - 0.0001185* T*T - 0.0000116* T*T*T;

		A = DSIN(alpha2000 + zeta_A) * DCOS(delta2000);
		B = DCOS(alpha2000 + zeta_A) * DCOS(theta_A) * DCOS(delta2000)
			- DSIN(theta_A) * DSIN(delta2000);
		C = DCOS(alpha2000 + zeta_A) * DSIN(theta_A) * DCOS(delta2000)
			+ DCOS(theta_A) * DSIN(delta2000);

		alpha = range_degrees (DATAN2(A,B) + z_A);
		delta = DASIN(C);
	} else {
		/* should get the same answer, but this could improve accruacy */
		alpha = alpha2000;
		delta = delta2000;
	};

	*ra = alpha;
	*dec = delta;
}

/* given apparent altitude find airmass.
 * R.H. Hardie, 1962, `Photoelectric Reductions', Chapter 8 of Astronomical
 * Techniques, W.A. Hiltner (Ed), Stars and Stellar Systems, II (University
 * of Chicago Press: Chicago), pp178-208.
 */

// compute airmass for aa apparent altitude (degrees)
double
astro_airmass (double aa)
{
	double sm1;	/* secant zenith angle, minus 1 */

	/* degenerate near or below horizon */
	if (aa < (3.0))
	    aa = (3.0);
	aa = degrad(aa);

	sm1 = 1.0 / sin(aa) - 1.0;
	return 1.0 + sm1*(0.9981833 - sm1*(0.002875 + 0.0008083*sm1));
}



static double precs[10] = {1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001,
			   0.0000001, 0.00000001, 0.000000001};

/* transform degrees into minutes/seconds format
 * with variable precision (decimals of seconds)
 */
void
astro_degrees_to_dms_pr(char *lb, double deg, int prec)
{
	int d1, d2, d3, d4;
	char fbuf[32];

	if (prec > 9)
		prec = 9;
	if (prec < 0)
		prec = 0;

	if (deg > 0)
		deg += precs[prec] / 7200;
	else
		deg -= precs[prec] / 7200;

//	d3_printf("format is %s\n", fbuf);
	if (deg >= 0.0) {
		d1 = floor (deg);
		deg = (deg - d1) * 60.0;
		d2 = floor (deg);
		deg = ((deg - d2) * 60.0);
		d3 = floor(deg);
		if (prec == 0)
			sprintf(lb, "%02d:%02d:%02d", d1, d2, d3);
		else {
			deg = ((deg - d3) / precs[prec]);
			d4 = floor(deg);
			sprintf(fbuf, "%%02d:%%02d:%%02d.%%0%dd", prec);
			sprintf(lb, fbuf, d1, d2, d3, d4);
		}
	} else {
		deg = -deg;
		d1 = floor (deg);
		deg = (deg - d1) * 60.0;
		d2 = floor (deg);
		deg = ((deg - d2) * 60.0);
		d3 = floor(deg);
		if (prec == 0)
			sprintf(lb, "-%02d:%02d:%02d", d1, d2, d3);
		else {
			deg = ((deg - d3) / precs[prec]);
			d4 = floor(deg);
			sprintf(fbuf, "-%%02d:%%02d:%%02d.%%0%dd", prec);
			sprintf(lb, fbuf, d1, d2, d3, d4);
		}
	}
//	d3_printf("into %s\n", lb);
}

/* transform degrees into minutes/seconds format
 * with a precision of 2 (decimals of seconds)
 */
void
astro_degrees_to_dms(char *lb, double deg)
{
	astro_degrees_to_dms_pr(lb, deg, 2);
}


// transform a string d:m:s coordinate to a float angle (in degrees)
// for ra, must be multiplied by 15
int
astro_dms_to_degrees(char *decs, double *dec)
{
	char *endp;
	double d = 0.0, m= 0.0, s = 0.0;
	int i;
	double sign = 1.0;

	for (i=0; decs[i] == ' ' && decs[i] != 0; i++)
		;
	if (decs[i] == '-') {
		sign = -1.0;
		i++;
	}

	d = strtod(decs+i, &endp);
	if (endp == decs+i) {
		return -1;
	}

	for (i = endp - decs; decs[i] && !isdigit(decs[i]); i++);
	m = strtod(decs+i, &endp);
	if (endp == decs+i) {
		*dec = d * sign;
		return 0;
	}

	for (i = endp - decs; decs[i] && !isdigit(decs[i]); i++);
	s = strtod(decs+i, &endp);
	if (endp == decs+i) {
		*dec = (d + m / 60.0) * sign;
		return 0;
	}

	*dec = (d + m / 60.0 + s / 3600.0) * sign;
	return 0;
}
