#ifndef __UCAC4_H
#define __UCAC4_H

#include <stdint.h>

/* Raw structures are read herein,  so the following structure  */
/* must be packed on byte boundaries:                           */
#pragma pack( 1)

/* Changes from UCAC-3:  ra_sigma, dec_sigma, mag_sigma, pm_ra_sigma &
pm_dec_sigma were int16_ts in UCAC3. pm_ra & pm_dec were int32_ts.
n_cats_total has been removed for UCAC4,  as has SuperCOSMOS data.
APASS mags were added. The two Yale flags were combined,  to save a
byte.  The ra/dec and proper motion sigmas are now offset by 128;
i.e.,  a value of -128 would indicate a zero sigma.
*/

#define UCAC4_STAR struct ucac4_binary_format

UCAC4_STAR {
   int32_t ra, spd;         /* RA/dec at J2000.0,  ICRS,  in milliarcsec */
   uint16_t mag1, mag2;     /* UCAC fit model & aperture mags, .001 mag */
   uint8_t mag_sigma;
   uint8_t obj_type, double_star_flag;
   int8_t ra_sigma, dec_sigma;    /* sigmas in RA and dec at central epoch */
   uint8_t n_ucac_total;     /* Number of UCAC observations of this star */
   uint8_t n_ucac_used;      /* # UCAC observations _used_ for this star */
   uint8_t n_cats_used;      /* # catalogs (epochs) used for prop motion */
   uint16_t epoch_ra;        /* Central epoch for mean RA, minus 1900, .01y */
   uint16_t epoch_dec;       /* Central epoch for mean DE, minus 1900, .01y */
   int16_t pm_ra;            /* prop motion, .1 mas/yr = .01 arcsec/cy */
   int16_t pm_dec;           /* prop motion, .1 mas/yr = .01 arcsec/cy */
   int8_t pm_ra_sigma;       /* sigma in same units */
   int8_t pm_dec_sigma;
   uint32_t twomass_id;           /* 2MASS pts_key star identifier */
   uint16_t mag_j, mag_h, mag_k;  /* 2MASS J, H, K_s mags,  in millimags */
   uint8_t icq_flag[3];
   uint8_t e2mpho[3];          /* 2MASS error photometry (in centimags) */
   uint16_t apass_mag[5];      /* in millimags */
   uint8_t apass_mag_sigma[5]; /* also in millimags */
   uint8_t yale_gc_flags;      /* Yale SPM g-flag * 10 + c-flag */
   uint32_t catalog_flags;
   uint8_t leda_flag;          /* LEDA galaxy match flag */
   uint8_t twomass_ext_flag;   /* 2MASS extended source flag */
   uint32_t id_number;
   uint16_t ucac2_zone;
   uint32_t ucac2_number;
   };
#pragma pack( )

/* need zone & running number together with cat data */
struct ucac4_star {
	UCAC4_STAR cat;
	int zone;
	int number;
};

int ucac4_search(double ra, double dec, double w, double h,
		 void *buf, int sz, char *path);

#endif
