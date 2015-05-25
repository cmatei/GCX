#ifndef __ASTRO_H
#define __ASTRO_H

#ifndef PI
#define PI 3.141592653589793
#endif

#define JD2000 2451545.0

#define degrad(x)       ((x)*PI/180.)
#define raddeg(x)       ((x)*180./PI)
#define hrdeg(x)        ((x)*15.)
#define deghr(x)        ((x)/15.)
#define hrrad(x)        degrad(hrdeg(x))
#define radhr(x)        deghr(raddeg(x))

#define DCOS(x)         cos(degrad(x))
#define DSIN(x)         sin(degrad(x))
#define DASIN(x)        raddeg(asin(x))
#define DATAN2(y,x)     raddeg(atan2((y),(x)))


extern double astro_get_apparent_sidereal_time     (double JD);
extern double astro_get_mean_sidereal_time         (double JD);

extern void   astro_get_hrz_from_equ_sidereal_time (double objra, double objdec,
						    double lng, double lat,
						    double sidereal, double *alt, double *az);
extern void   astro_get_equ_from_hrz_sidereal_time (double alt, double az, double lng, double lat,
						    double sidereal,
						    double *ra, double *dec);

extern double astro_get_refraction_adj_true        (double altitude, double atm_pres, double temp);
extern double astro_get_refraction_adj_apparent    (double altitude, double atm_pres, double temp);


extern void   astro_precess_hiprec                 (double epo1, double epo2, double *ra, double *dec);

extern double astro_airmass                        (double aa);

extern void   astro_degrees_to_dms_pr              (char *lb, double deg, int prec);
extern void   astro_degrees_to_dms                 (char *lb, double deg);

extern int    astro_dms_to_degrees                 (char *decs, double *dec);

extern double astro_range_degrees                  (double angle);


#endif
