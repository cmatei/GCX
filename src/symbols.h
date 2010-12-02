#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/* symbols we use in recipe and photometry reports */

typedef enum {
	SYM_NULL,
	SYM_RECIPE,
	SYM_STARS,
	SYM_OBSERVATION,
	SYM_AP_PAR,
	SYM_TRANSFORM,
	SYM_RA,
	SYM_DEC,
	SYM_EQUINOX,
	SYM_JDATE,
	SYM_TELESCOPE,
	SYM_APERTURE,
	SYM_EXPTIME,
	SYM_SNS_TEMP,
	SYM_FILTER,
	SYM_COMMENTS,
	SYM_NAME,
	SYM_TYPE,
	SYM_MAG,
	SYM_SMAGS,
	SYM_IMAGS,
	SYM_FLAGS,
	SYM_RESIDUAL,
	SYM_NOISE,
	SYM_SKY,
	SYM_PHOTON,
	SYM_READ,
	SYM_ELADU,
	SYM_FLAT,
	SYM_SCINT,
	SYM_R1,
	SYM_R2,
	SYM_R3,
	SYM_SKY_METHOD,
	SYM_SIGMAS,
	SYM_CENTERED,
	SYM_UNDEF_ERR,
	SYM_NOT_FOUND,
	SYM_HAS_BADPIX,
	SYM_BRIGHT,
	SYM_FIELD,
	SYM_ASTROM,
	SYM_PHOT,
	SYM_VAR,
	SYM_STD,
	SYM_OBJECT,
	SYM_WEIGHT,
	SYM_TGT,
	SYM_TARGET,
	SYM_MJD,
	SYM_AIRMASS,
	SYM_DRA,
	SYM_DDEC,
	SYM_SMAG,
	SYM_IMAG,
	SYM_SERR,
	SYM_IERR,
	SYM_TABLEHEAD,
	SYM_COLLIST,
	SYM_NO_COLOR,
	SYM_CATALOG,
	SYM_TRANSFORMED,
	SYM_ALL_SKY,
	SYM_SEQUENCE,
	SYM_LATITUDE,
	SYM_LONGITUDE,
	SYM_ALTITUDE,
	SYM_OBSERVER,
	SYM_INVALID,
	SYM_STDERR,
	SYM_BAND,
	SYM_ZP,
	SYM_ZPERR,
	SYM_ZPME1,
	SYM_COLOR,
	SYM_CCOEFF,
	SYM_CCERR,
	SYM_ECOEFF,
	SYM_ECERR,
	SYM_ECME1,
	SYM_PERR,
	SYM_CENTROID,
	SYM_X,
	SYM_XERR,
	SYM_Y,
	SYM_YERR,
	SYM_DX,
	SYM_DY,
	SYM_AP_SHAPE,
	SYM_RES_STATS,
	SYM_OUTLIERS,
	SYM_DIFFAM,
	SYM_LAST
} RcpSymbol;

extern char *symname[];

#define FLAGNAME_ASTROM "astrom"
#define FLAGNAME_VAR "var"
#define FLAGNAME_CENTERED "centered"
#define FLAGNAME_NOT_FOUND "not_found"
#define FLAGNAME_UNDEF_ERR "undef_err"
#define FLAGNAME_HAS_BADPIX "has_badpix"
#define FLAGNAME_BRIGHT "bright"
#define FLAGNAME_NO_COLOR "no_color"
#define FLAGNAME_TRANSFORMED "transformed"
#define FLAGNAME_ALL_SKY "all_sky"
#define FLAGNAME_INVALID "invalid"
#define FLAGNAME_ASTRO_TARGET "astro_target"

#define FLAG_NAMES_INIT {"","","","",\
			"", FLAGNAME_ASTROM, "", FLAGNAME_VAR, \
			FLAGNAME_CENTERED, FLAGNAME_NOT_FOUND, FLAGNAME_UNDEF_ERR, \
			FLAGNAME_HAS_BADPIX,\
			FLAGNAME_BRIGHT, FLAGNAME_NO_COLOR, \
			FLAGNAME_TRANSFORMED, FLAGNAME_ALL_SKY, \
			FLAGNAME_INVALID, NULL }


#define SYM_NAMES_INIT {\
	"null",\
	"recipy",\
	"stars",\
	"observation",\
	"ap_par",\
	"transform",\
	"ra",\
	"dec",\
	"equinox",\
	"jdate",\
	"telescope",\
	"aperture",\
	"exptime",\
	"sns_temp",\
	"filter",\
	"comments",\
	"name",\
	"type",\
	"mag",\
	"smags",\
	"imags",\
	"flags",\
	"residual",\
	"noise",\
	"sky",\
	"photon",\
	"read",\
	"eladu",\
	"flat",\
	"scint",\
	"r1",\
	"r2",\
	"r3",\
	"sky_method",\
	"sigmas",\
	FLAGNAME_CENTERED,\
	FLAGNAME_UNDEF_ERR,\
	FLAGNAME_NOT_FOUND,\
	FLAGNAME_HAS_BADPIX,\
	FLAGNAME_BRIGHT,\
	"field",\
	FLAGNAME_ASTROM,\
	"phot",\
	FLAGNAME_VAR,\
	"std",\
	"object",\
	"weight",\
	"tgt",\
	"target",\
	"mjd",\
	"airmass",\
	"dra", \
	"ddec",\
	"smag",\
	"imag",\
	"serr",\
	"ierr",\
	"tablehead",\
	"collist",\
	FLAGNAME_NO_COLOR,\
	"catalog",\
	FLAGNAME_TRANSFORMED,\
	FLAGNAME_ALL_SKY,\
	"sequence",\
	"latitude",\
	"longitude",\
	"altitude",\
	"observer",\
	FLAGNAME_INVALID,\
	"stderr",\
	"band",\
	"zp",\
	"zperr",\
	"zpme1",\
	"color",\
	"ccoeff",\
	"ccerr",\
	"ecoeff",\
	"ecerr",\
	"ecme1",\
	"perr",\
	"centroid",\
	"xc",\
	"xerr",\
	"yc",\
	"yerr",\
	"dx",\
	"dy",\
	"ap_shape",\
	"res_stats",\
	"outliers",\
	"diffam"\
}


#endif
