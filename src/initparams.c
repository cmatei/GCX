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
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "gcx.h"
#include "gcximageview.h"
#include "params.h"
#include "catalogs.h"
#include "sourcesdraw.h"

/* functions used for parameter table initialisation */

/* find an attachemnt node (last of parent's children)
 * return -1 if no suitable point is found, or the node.
 * a link to the new item is added in the found node
 */
static int add_link(GcxPar parent, GcxPar p)
{
	int point;
	if (parent == PAR_NULL) {
		point = PAR_FIRST;
		while (PAR(point)->next != PAR_NULL)
			point = PAR(point)->next;
		PAR(point)->next = p;
		return point;
	}
	if (PAR_TYPE(parent) != PAR_TREE) {
		err_printf("add_par: parent %d is not a tree!\n", parent);
		return -1;
	}
	if (PAR(parent)->child == PAR_NULL) {
		PAR(parent)->child = p;
		return parent;
	}
	point = PAR(parent)->child;
	while (PAR(point)->next != PAR_NULL)
		point = PAR(point)->next;
//	d3_printf("chaining to %d\n", point);
	PAR(point)->next = p;
	return point;
}

static void add_par_tree(GcxPar p, GcxPar parent, char *name, char *comment)
{
	int point;

	if (p >= PAR_TABLE_SIZE || parent >= PAR_TABLE_SIZE) {
		err_printf("add_par: bad par[%d] index \n", p);
		return;
	}
	point = add_link(parent, p);
	if (point <= 0) {
		err_printf("add_par: cannot find link point for [%d]\n", p);
		return;
	}
	PAR(p)->parent = (parent);
	PAR(p)->child = PAR_NULL;
	PAR(p)->next = PAR_NULL;
	PAR(p)->flags = PAR_TREE;
	PAR(p)->name = name;
	PAR(p)->comment = comment;
}

static void add_par_int(GcxPar p, GcxPar parent, int format, char *name, char *comment, int v)
{
	int point;

	if (p >= PAR_TABLE_SIZE || parent >= PAR_TABLE_SIZE) {
		err_printf("add_par: bad par index\n");
		return;
	}
	point = add_link(parent, p);
	if (point <= 0) {
		err_printf("add_par: cannot find link point for [%d]\n", p);
		return;
	}
	PAR(p)->parent = (parent);
	PAR(p)->child = (PAR_NULL);
	PAR(p)->next = PAR_NULL;
	PAR(p)->flags = PAR_INTEGER | format;
	PAR(p)->name = name;
	PAR(p)->comment = comment;
	P_INT(p) = v;
	PAR(p)->defval.i = v;
}

static void add_par_double(GcxPar p, GcxPar parent, int format, char *name, char *comment,
			   double v)
{
	int point;

	if (p >= PAR_TABLE_SIZE || parent >= PAR_TABLE_SIZE) {
		err_printf("add_par: bad par index\n");
		return;
	}
	point = add_link(parent, p);
	if (point <= 0) {
		err_printf("add_par: cannot find link point for [%d]\n", p);
		return;
	}
	PAR(p)->parent = (parent);
	PAR(p)->child = (PAR_NULL);
	PAR(p)->next = PAR_NULL;
	PAR(p)->flags = PAR_DOUBLE | format;
	PAR(p)->name = name;
	PAR(p)->comment = comment;
	P_DBL(p) = v;
	PAR(p)->defval.d = v;
}

static void add_par_string(GcxPar p, GcxPar parent, int format, char *name, char *comment,
			   char *val)
{
	int point;

//	d3_printf("add_par_string %s %d [%d]\n", name, p, parent);

	if (p >= PAR_TABLE_SIZE || parent >= PAR_TABLE_SIZE) {
		err_printf("add_par: bad par index\n");
		return;
	}
	point = add_link(parent, p);
	if (point <= 0) {
		err_printf("add_par: cannot find link point for [%d]\n", p);
		return;
	}
	PAR(p)->parent = (parent);
	PAR(p)->child = PAR_NULL;
	PAR(p)->next = PAR_NULL;
	PAR(p)->flags = PAR_STRING | format;
	PAR(p)->name = name;
	PAR(p)->comment = comment;
	if (P_STR(p) != NULL) {
		free(P_STR(p));
		P_STR(p) = NULL;
	}
	if (PAR(p)->defval.s != NULL) {
		free(PAR(p)->defval.s);
		PAR(p)->defval.s = NULL;
	}
	P_STR(p) = malloc(strlen(val)+1);
	if (P_STR(p) != NULL)
		strcpy(P_STR(p), val);
	PAR(p)->defval.s = malloc(strlen(val)+1);
	if (PAR(p)->defval.s != NULL)
		strcpy(PAR(p)->defval.s, val);
}

/*
 * make an int parameter show as a choices list
 * choices must be valid for the lifetime of the
 * program; static constants are good*/
static void set_par_choices(GcxPar p, char ** choices)
{
	int i = 0;
	if (PAR_TYPE(p) != PAR_INTEGER) {
		err_printf("cannot add choices to a non-int par\n");
		return;
	}
	PAR(p)->choices = choices;
	PAR(p)->flags &= ~PAR_FORMAT_FM;
	PAR(p)->flags |= FMT_OPTION;
	while (choices[i] != NULL)
		i++;
	clamp_int(&(P_INT(p)), 0, i);
}

/* set a par's description text (retains a reference to text)*/
static void set_par_description(GcxPar p, char * text)
{
	PAR(p)->description = text;
}



/* the initialisation proper */

void init_ptable(void)
{
/* first the trees */
	add_par_tree(PAR_FILES, PAR_NULL,
		     "file", "File and Device Options");
	add_par_tree(PAR_FITS_FIELDS, PAR_FILES,
		     "fits", "Fits Field Names");
	add_par_tree(PAR_OBS_DEFAULTS, PAR_NULL,
		     "obs", "General Observation Setup Data");
	add_par_tree(PAR_INDI, PAR_NULL,
		     "indi", "INDI connection options");
	add_par_tree(PAR_TELE, PAR_NULL,
		     "tel", "Telescope control options");
	add_par_tree(PAR_GUIDE, PAR_NULL,
		     "guide", "Guiding options");
	add_par_tree(PAR_CCDRED, PAR_NULL,
		     "ccdred", "CCD Reduction options");
	add_par_tree(PAR_STAR_DET, PAR_NULL,
		     "stars", "Star Detection and Search Options");
	add_par_tree(PAR_WCS_OPTIONS, PAR_NULL,
		     "wcs", "Wcs Fitting Options");
	add_par_tree(PAR_APHOT, PAR_NULL,
		     "aphot", "Aperture Photometry Options");
	add_par_tree(PAR_MBAND, PAR_NULL,
		     "mframe", "Multi-Frame Photometry Options");
	add_par_tree(PAR_STAR_DISPLAY, PAR_NULL,
		     "display", "Star Display Options");
	add_par_tree(PAR_STAR_COLORS, PAR_STAR_DISPLAY,
		     "color", "Star Display Colors");
	add_par_tree(PAR_STAR_SHAPES, PAR_STAR_DISPLAY,
		     "shape", "Star Display Shapes");
	add_par_tree(PAR_SYNTH, PAR_NULL,
		     "synth", "Synthetic Star Generation Options");
	add_par_tree(PAR_QUERY, PAR_NULL,
		     "query", "On-line Resources");


/* now leaves */
/* leaves for query */
	add_par_string(QUERY_VIZQUERY, PAR_QUERY, 0, "vizquery",
		       "Vizquery command", "vizquery");
	set_par_description(QUERY_VIZQUERY,
			    "The command used to download catalog stars (normally "
			    "vizquery from the cdstools package available from CDS. "
			    "See file README.vizquery and the cds site for more details. "
		);
	add_par_int(QUERY_MAX_STARS, PAR_QUERY, 0, "maxstars",
		    "Maximum Catalog Stars", 5000);
	set_par_description(QUERY_MAX_STARS,
			    "The maximum number of starts the program will download from "
			    "a field star catalog." );
	add_par_double(QUERY_MAX_RADIUS, PAR_QUERY, PREC_1, "maxradius",
		       "Maximum Radius for Catalog Stars", 20.0);
	set_par_description(QUERY_MAX_RADIUS,
			    "The maximum radius from the frame center "
			    "in which we look for catalog stars. The "
			    "actual radius depends on the frame size; this "
			    "is a global limit. In minutes of arc.");

	add_par_string(QUERY_WGET, PAR_QUERY, 0, "wget", "Wget command", "wget");
	set_par_description(QUERY_WGET, "Path to the wget program.");

	add_par_string(QUERY_SKYVIEW_RUNQUERY_URL, PAR_QUERY, 0, "skyview_runquery_url",
		       "SkyView request URL",
		       "http://skyview.gsfc.nasa.gov/cgi-bin/runquery.pl");
	set_par_description(QUERY_SKYVIEW_RUNQUERY_URL,
			    "The URL for the SkyView request form.");

	add_par_string(QUERY_SKYVIEW_TEMPSPACE_URL, PAR_QUERY, 0, "skyview_tempspace_url",
		       "SkyView result URL",
		       "http://skyview.gsfc.nasa.gov/tempspace/fits/");
	set_par_description(QUERY_SKYVIEW_TEMPSPACE_URL,
			    "The URL for the SkyView result data directory. Gcx will "
			    "look here for the file <resultname>.fits.");

	add_par_string(QUERY_SKYVIEW_DIR, PAR_QUERY, 0, "skyview_dir",
		       "SkyView download directory", "/tmp/");
	set_par_description(QUERY_SKYVIEW_DIR,
			    "Download directory for SkyView FITS files.");

	add_par_int(QUERY_SKYVIEW_KEEPFILES, PAR_QUERY, FMT_BOOL, "skyview_keepfiles",
		    "Keep downloaded SkyView files", 1);
	set_par_description(QUERY_SKYVIEW_KEEPFILES,
			    "Keep downloaded SkyView files.");

	add_par_int(QUERY_SKYVIEW_PIXELS, PAR_QUERY, 0, "skyview_pixels",
		    "SkyView image size in pixels", 800);
	set_par_description(QUERY_SKYVIEW_PIXELS,
			    "SkyView image size in pixels.");

/* leaves for stardet */
	add_par_double(SD_SNR, PAR_STAR_DET, PREC_1, "det_snr",
		       "Star Detection SNR", 9.0);
	set_par_description(SD_SNR,
			    "A star must be above this threshold above the backround"
			    " to be considered "
			    "for detection. Expressed in sigmas of the image. "
			    "Useful range between 3 and 24.");
	add_par_int(SD_MAX_STARS, PAR_STAR_DET, 0, "det_maxstars",
		    "Maximum Detected Stars", 200);
	set_par_description(SD_MAX_STARS,
			    "The maximum number of starts the program will extract from "
			    "an image. If more are found, only the brightest are kept." );
	add_par_int(SD_GSC_MAX_STARS, PAR_STAR_DET, 0, "cat_maxstars",
		    "Maximum Catalog Stars", 300);
	set_par_description(SD_GSC_MAX_STARS,
			    "The maximum number of starts the program will extract from "
			    "a field star catalog. If more are found, only the "
			    "brightest are kept." );
	add_par_double(SD_GSC_MAX_MAG, PAR_STAR_DET, PREC_1, "cat_maxmag",
		       "Faintest Magnitude for Catalog Stars", 16.0);
	add_par_double(SD_GSC_MAX_RADIUS, PAR_STAR_DET, PREC_1, "cat_maxradius",
		       "Maximum Radius for Catalog Stars", 120.0);
	set_par_description(SD_GSC_MAX_RADIUS,
			    "The maximum radius from the frame center "
			    "in which we look for catalog stars. The "
			    "actual radius depends on the frame size; this "
			    "is a global limit. In minutes of arc.");

/* fits fields */
	add_par_string(FN_CRPIX1, PAR_FITS_FIELDS, 0, "crpix1",
		       "X coordinate of reference pixel", "CRPIX1");
	add_par_string(FN_CRPIX2, PAR_FITS_FIELDS, 0, "crpix2",
		       "Y coordinate of reference pixel", "CRPIX2");
	add_par_string(FN_CRVAL1, PAR_FITS_FIELDS, 0, "crval1",
		       "WCS R.A. of reference pixel", "CRVAL1");
	add_par_string(FN_CRVAL2, PAR_FITS_FIELDS, 0, "crval2",
		       "WCS Dec of reference pixel", "CRVAL2");
	add_par_string(FN_CDELT1, PAR_FITS_FIELDS, 0, "cdelt1",
		       "Degrees/pixel in R.A.", "CDELT1");
	add_par_string(FN_CDELT2, PAR_FITS_FIELDS, 0, "cdelt2",
		       "Degrees/pixel in Dec", "CDELT2");
	add_par_string(FN_CROTA1, PAR_FITS_FIELDS, 0, "crota1",
		       "Field rotation", "CROTA1");
	add_par_string(FN_EQUINOX, PAR_FITS_FIELDS, 0, "equinox",
		       "Equinox of WCS", "EQUINOX");
	add_par_string(FN_EPOCH, PAR_FITS_FIELDS, 0, "epoch",
		       "Equinox of WCS", "EPOCH");
	add_par_string(FN_OBJECT, PAR_FITS_FIELDS, 0, "object",
		       "Object name", "OBJECT");
	add_par_string(FN_OBJCTRA, PAR_FITS_FIELDS, 0, "objctra",
		       "Object R.A.", "OBJCTRA");
	add_par_string(FN_OBJCTDEC, PAR_FITS_FIELDS, 0, "objctdec",
		       "Object Dec", "OBJCTDEC");
	add_par_string(FN_SECPIX, PAR_FITS_FIELDS, 0, "secpix",
		       "Image scale", "SECPIX");
	add_par_string(FN_EFP_MIN1, PAR_FITS_FIELDS, 0, "efpmin1",
		       "First horizontal effective pixel", "EFP-MIN1");
	add_par_string(FN_EFP_RNG1, PAR_FITS_FIELDS, 0, "efprng1",
		       "Number of horizontal effective pixels", "EFP-RNG1");
	add_par_string(FN_EFP_MIN2, PAR_FITS_FIELDS, 0, "efpmin2",
		       "First vertical effective pixel", "EFP-MIN2");
	add_par_string(FN_EFP_RNG2, PAR_FITS_FIELDS, 0, "efprng2",
		       "Number of vertical effective pixels", "EFP-RNG2");
	add_par_string(FN_OVS_MIN1, PAR_FITS_FIELDS, 0, "ovsmin1",
		       "First horizontal overscan pixel", "OVS-MIN1");
	add_par_string(FN_OVS_RNG1, PAR_FITS_FIELDS, 0, "ovsrng1",
		       "Number of horizontal overscan pixels", "OVS-RNG1");
	add_par_string(FN_OVS_MIN2, PAR_FITS_FIELDS, 0, "ovsmin2",
		       "First vertical overscan pixel", "OVS-MIN2");
	add_par_string(FN_OVS_RNG2, PAR_FITS_FIELDS, 0, "ovsrng2",
		       "Number of vertical overscan pixels", "OVS-RNG2");
	add_par_string(FN_RA, PAR_FITS_FIELDS, 0, "ra",
		       "Field center R.A.", "RA");
	add_par_string(FN_DEC, PAR_FITS_FIELDS, 0, "dec",
		       "Field center Dec", "DEC");
	add_par_string(FN_FILTER, PAR_FITS_FIELDS, 0, "filter",
		       "Filter name", "FILTER");
	add_par_string(FN_EXPTIME, PAR_FITS_FIELDS, 0, "exptime",
		       "Exposure time", "EXPTIME");
	add_par_string(FN_JDATE, PAR_FITS_FIELDS, 0, "jdate",
		       "Julian date of observation", "JDATE");
	add_par_string(FN_MJD, PAR_FITS_FIELDS, 0, "mjd",
		       "Modified julian date of observation", "MJD");
	add_par_string(FN_DATE_OBS, PAR_FITS_FIELDS, 0, "dateobs",
		       "Date/time of observation", "DATE-OBS");
	add_par_string(FN_TIME_OBS, PAR_FITS_FIELDS, 0, "timeobs",
		       "Time of observation", "TIME-OBS");
	add_par_string(FN_TELESCOP, PAR_FITS_FIELDS, 0, "telescop",
		       "Telescope name", "TELESCOP");
	add_par_string(FN_FOCUS, PAR_FITS_FIELDS, 0, "focus",
		       "Focus designation", "FOCUS");
	add_par_string(FN_APERTURE, PAR_FITS_FIELDS, 0, "aperture",
		       "Telescope aperture", "APERT");
	add_par_string(FN_FLEN, PAR_FITS_FIELDS, 0, "flen",
		       "Focal length", "FLEN");
	add_par_string(FN_INSTRUME, PAR_FITS_FIELDS, 0, "instrument",
		       "Instrument name", "INSTRUME");
	add_par_string(FN_OBSERVER, PAR_FITS_FIELDS, 0, "observer",
		       "Observer name", "OBSERVER");
	add_par_string(FN_LATITUDE, PAR_FITS_FIELDS, 0, "latitude",
		       "Latitude of observing site", "LAT-OBS");
	add_par_string(FN_LONGITUDE, PAR_FITS_FIELDS, 0, "longitude",
		       "Longitude of observing site", "LONG-OBS");
	add_par_string(FN_ALTITUDE, PAR_FITS_FIELDS, 0, "altitude",
		       "Altitude of observing site", "ALT-OBS");
	add_par_string(FN_AIRMASS, PAR_FITS_FIELDS, 0, "airmass",
		       "Airmass", "AIRMASS");
	add_par_string(FN_SNSTEMP, PAR_FITS_FIELDS, 0, "snstemp",
		       "Sensor temperature", "CCDTEMP");
	add_par_string(FN_BINX, PAR_FITS_FIELDS, 0, "binx",
		       "Horisontal binning", "CCDBIN1");
	add_par_string(FN_BINY, PAR_FITS_FIELDS, 0, "biny",
		       "Vertical binning", "CCDBIN2");
	add_par_string(FN_ELADU, PAR_FITS_FIELDS, 0, "eladu",
		       "Electrons per ADU", "ELADU");
	add_par_string(FN_DCBIAS, PAR_FITS_FIELDS, 0, "dcbias",
		       "Average bias of frame", "DCBIAS");
	add_par_string(FN_RDNOISE, PAR_FITS_FIELDS, 0, "rdnoise",
		       "Readout noise", "RDNOISE");
	add_par_string(FN_FLNOISE, PAR_FITS_FIELDS, 0, "flnoise",
		       "Multiplicative noise coefficient", "FLNOISE");
	add_par_string(FN_CFA_FMT, PAR_FITS_FIELDS, 0, "cfa_fmt",
		       "Color-field-array layout", "CFA_FMT");
	add_par_string(FN_WHITEBAL, PAR_FITS_FIELDS, 0, "whitebal",
		       "White balance calibration", "WHITEBAL");



/* star display colors/sizes */
	add_par_int(SDISP_SIMPLE_SHAPE, PAR_STAR_SHAPES, 0, "simple",
		    "Autodected Stars Shape", STAR_SHAPE_SQUARE);
	set_par_choices(SDISP_SIMPLE_SHAPE, star_shapes);
	add_par_int(SDISP_SIMPLE_COLOR, PAR_STAR_COLORS, FMT_OPTION, "simple",
		    "Autodected Stars Color", PAR_COLOR_GREEN);
	set_par_choices(SDISP_SIMPLE_COLOR, star_colors);

	add_par_int(SDISP_SREF_SHAPE, PAR_STAR_SHAPES, 0, "field",
		    "Field Stars Shape", STAR_SHAPE_DIAMOND);
	set_par_choices(SDISP_SREF_SHAPE, star_shapes);
	add_par_int(SDISP_SREF_COLOR, PAR_STAR_COLORS, FMT_OPTION, "field",
		    "Field Stars Color", PAR_COLOR_RED);
	set_par_choices(SDISP_SREF_COLOR, star_colors);

	add_par_int(SDISP_APSTD_SHAPE, PAR_STAR_SHAPES, 0, "apstd",
		    "Standard Stars Shape", STAR_SHAPE_APHOT);
	set_par_choices(SDISP_APSTD_SHAPE, star_shapes);
	add_par_int(SDISP_APSTD_COLOR, PAR_STAR_COLORS, FMT_OPTION, "apstd",
		    "Standard Stars Color", PAR_COLOR_RED);
	set_par_choices(SDISP_APSTD_COLOR, star_colors);

	add_par_int(SDISP_APSTAR_SHAPE, PAR_STAR_SHAPES, 0, "target",
		    "Target Stars Shape", STAR_SHAPE_CROSS);
	set_par_choices(SDISP_APSTAR_SHAPE, star_shapes);
	add_par_int(SDISP_APSTAR_COLOR, PAR_STAR_COLORS, FMT_OPTION, "target",
		    "Target Stars Color", PAR_COLOR_RED);
	set_par_choices(SDISP_APSTAR_COLOR, star_colors);

	add_par_int(SDISP_CAT_SHAPE, PAR_STAR_SHAPES, 0, "catalog",
		    "Catalog Objects Shape", STAR_SHAPE_DIAMOND);
	set_par_choices(SDISP_CAT_SHAPE, star_shapes);
	add_par_int(SDISP_CAT_COLOR, PAR_STAR_COLORS, FMT_OPTION, "catalog",
		    "Catalog Objects Color", PAR_COLOR_YELLOW);
	set_par_choices(SDISP_CAT_COLOR, star_colors);

	add_par_int(SDISP_USEL_SHAPE, PAR_STAR_SHAPES, 0, "usel",
		    "User-Marked Stars Shape", STAR_SHAPE_CIRCLE);
	set_par_choices(SDISP_USEL_SHAPE, star_shapes);
	add_par_int(SDISP_USEL_COLOR, PAR_STAR_COLORS, FMT_OPTION, "usel",
		    "User-Marked Stars Color", PAR_COLOR_GREEN);
	set_par_choices(SDISP_USEL_COLOR, star_colors);

	add_par_int(SDISP_ALIGN_SHAPE, PAR_STAR_SHAPES, 0, "align",
		    "Align Stars Shape", STAR_SHAPE_CIRCLE);
	set_par_choices(SDISP_ALIGN_SHAPE, star_shapes);
	add_par_int(SDISP_ALIGN_COLOR, PAR_STAR_COLORS, FMT_OPTION, "align",
		    "Align Stars Color", PAR_COLOR_ORANGE);
	set_par_choices(SDISP_ALIGN_COLOR, star_colors);

	add_par_int(SDISP_SELECTED_COLOR, PAR_STAR_COLORS, FMT_OPTION, "selected",
		    "Selected Stars Color", PAR_COLOR_WHITE);
	set_par_choices(SDISP_SELECTED_COLOR, star_colors);


	/* star labels */
	add_par_int(LABEL_APSTAR, PAR_STAR_DISPLAY, FMT_BOOL, "target_labels",
		    "Label Target Stars", 1);
	set_par_description(LABEL_APSTAR,
		    "Show name labels for target stars");
	add_par_int(LABEL_CAT, PAR_STAR_DISPLAY, FMT_BOOL, "cat_labels",
		    "Label Catalog Objects", 0);
	set_par_description(LABEL_CAT,
		    "Show name labels for catalog objects");
	add_par_int(LABEL_APSTD, PAR_STAR_DISPLAY, FMT_BOOL, "std_labels",
		    "Label Standard Stars", 1);
	set_par_description(LABEL_APSTD,
		    "Show magitude labels (with the decimal point omitted) for standard stars");
	add_par_string(LABEL_APSTD_BAND, PAR_STAR_DISPLAY, 0, "std_label_band",
		    "Standard Star Label Band", "v");
	set_par_description(LABEL_APSTD_BAND,
		    "The band from which the magnitude used for "
			    "standard star labeling is taken");


/* star display opts */
	add_par_int(DO_MIN_STAR_SZ, PAR_STAR_DISPLAY, 0, "min_sz",
		    "Minimum star size", 1);
	set_par_description(DO_MIN_STAR_SZ,
			    "The minimum radius of a star symbol on screen, "
			    "in pixels");
	add_par_int(DO_MAX_STAR_SZ, PAR_STAR_DISPLAY, 0, "max_sz",
		    "Maximum display star size", 18);
	set_par_description(DO_MAX_STAR_SZ,
			    "The maximum radius of a star symbol on screen, "
			    "in pixels.");
	add_par_int(DO_DEFAULT_STAR_SZ, PAR_STAR_DISPLAY, 0, "default_sz",
		    "Default display star size", 6);
	set_par_description(DO_DEFAULT_STAR_SZ,
			    "The default radius of a star symbol on screen, "
			    "used when no magnitude information is known; in pixels.");
	add_par_double(DO_PIXELS_PER_MAG, PAR_STAR_DISPLAY, PREC_2, "pix_per_mag",
		       "Display pixels per magnitude", 1.0);
	set_par_description(DO_PIXELS_PER_MAG,
			    "The amount the star symbols increase in radius "
			    "for an increase in brightness of 1 magnitude; in pixels");
	add_par_double(DO_MAXMAG, PAR_STAR_DISPLAY, PREC_1, "maxmag",
		       "Brightest symbol magnitude", 5);
	set_par_description(DO_MAXMAG,
			    "The magnitude at which we stop increasing "
			    "star symbol sizes");
	add_par_double(DO_DLIM_MAG, PAR_STAR_DISPLAY, PREC_1, "dlimmag",
		       "Display limiting magnitude", 17.0);
	set_par_description(DO_DLIM_MAG,
			    "Magnitude of the faintest displayed objects");
	add_par_int(DO_DLIM_FAINTER, PAR_STAR_DISPLAY, FMT_BOOL, "dlimfainter",
		    "Show fainter stars", 0);
	set_par_description(DO_DLIM_FAINTER,
		    "Show stars fainter than the display limiting magnitude"
		    " (at minimum size)");
	add_par_int(DO_ZOOM_SYMBOLS, PAR_STAR_DISPLAY, FMT_BOOL, "zoom",
		    "Zoom star shapes", 1);
	set_par_description(DO_ZOOM_SYMBOLS,
			    "Scale star shapes when the image is zoomed. "
			    "Regardless of the value of this parameter, "
			    "aphot shapes are always zoomed, while "
			    "blob symbols never are." );
	add_par_int(DO_ZOOM_LIMIT, PAR_STAR_DISPLAY, 0, "zoom_limit",
		    "Star shapes zoom limit", 4);
	set_par_description(DO_ZOOM_LIMIT,
			    "The maximum scale factor applied to star shapes. "
			    "This limit does not apply to aphot shapes.");
	add_par_int(DO_SHOW_DELTAS, PAR_STAR_DISPLAY, FMT_BOOL, "plot_err",
		    "Plot position errors", 1);
	set_par_description(DO_SHOW_DELTAS,
			    "Draw the amount the apertures were moved from their "
			    "catalog position when stars are centered.");
//	add_par_double(DO_STAR_SZ_FWHM_FACTOR, PAR_STAR_DISPLAY, PREC_1, "fwhm_sz_fact",
//		       "Size of star in FWHMs", 3.0);
//	set_par_description(DO_STAR_SZ_FWHM_FACTOR,
//			    "Not used, deprecated");
	add_par_double(WCS_PLOT_ERR_SCALE, PAR_STAR_DISPLAY, PREC_1, "plot_err_scale",
		       "Plot error scale", 100);
	set_par_description(WCS_PLOT_ERR_SCALE,
			    "The amount by which the position errors are multiplied "
			    "when the position error plot is generated."
			);

/* wcs options */

	add_par_double(FIT_SCALE_TOL, PAR_WCS_OPTIONS, PREC_2, "fit_scale",
		       "Scale tolerance", 0.1);
	set_par_description(FIT_SCALE_TOL,
			    "The maximum amount by which the image scale "
			    "is changed during the fit. A value of 0.1 "
			    "implies that scales between 0.9 and 1.1 are accepted. "
			    "Using a lower value for this parameter will decrease "
			    "the running time and reduce the chances for an incorrect fit. ");
	add_par_double(FIT_MATCH_TOL, PAR_WCS_OPTIONS, PREC_1, "fit_tol",
		       "Star position tolerance", 1.5);
	set_par_description(FIT_MATCH_TOL,
			    "The maxiumum position error, in pixels, between "
			    "a star and a catalog position at which we still "
			    "consider that the two match. It is unadvisable to increase "
			    "this parameter much without a coresponding "
			    "increase in the minimum number of pairs, or the chances "
			    "for incorrect fits are greatly augumented."
			    );

	add_par_double(FIT_ROT_TOL, PAR_WCS_OPTIONS, PREC_1, "fit_rot",
		       "Rotation tolerance", 180.0);
	set_par_description(FIT_ROT_TOL,
			    "Maximum accepted field rotation, in degress "
			    "A value of 180 will match fields with any rotation. "
			    "Using a lower value for this parameter will decrease "
			    "the running time and reduce the chances for an incorrect fit. ");
	add_par_int(FIT_MIN_PAIRS, PAR_WCS_OPTIONS, 0, "fit_min_pairs",
		    "Minimum number of pairs", 5);
	set_par_description(FIT_MIN_PAIRS,
			    "When the algorithm finds a solution with at least "
			    "this many pairs, it considers it has found a good match "
			    "and stops. Using less than 5 pairs will likely give many "
			    "incorrect fits "
			    "in crowded fields"
			    );
	add_par_int(FIT_MAX_SKIP, PAR_WCS_OPTIONS, 0, "max_skip",
		    "Max skip", 20);
	set_par_description(FIT_MAX_SKIP,
			    "The maximum number of stars that don't seem to have a match in the "
			    "catalog and are skipped during the search"
			    );
	add_par_int(FIT_MAX_PAIRS, PAR_WCS_OPTIONS, 0, "max_pairs",
		    "Maximum pairs", 50);
	set_par_description(FIT_MAX_PAIRS,
			    "Maximum number of pairs fitted. After finding this many, "
			    "the algorithm stops.");
	add_par_double(FIT_MIN_AB_DIST, PAR_WCS_OPTIONS, 0, "min_ab_dist",
		       "Min A-B distance ", 100.0);
	set_par_description(FIT_MIN_AB_DIST,
			    "The minimum distance between two stars to be used "
			    "as an initial wcs hypothesis. "
			    );
	add_par_double(WCS_ERR_VALIDATE, PAR_WCS_OPTIONS, PREC_2, "validate_err",
		       "Max error for WCS validation", 1.5);
	set_par_description(WCS_ERR_VALIDATE,
			    "Maximum RMS position error of all pairs "
			    "at which the WCS is validated. Expressed in pixels.");

	add_par_int(WCS_PAIRS_VALIDATE, PAR_WCS_OPTIONS, 0, "validate_pairs",
		    "Min pairs for WCS validation", 3);
	set_par_description(WCS_PAIRS_VALIDATE,
			    "The minimum number of pairs needed to validate "
			    "a WCS solution." );

	add_par_double(WCS_MAX_SCALE_CHANGE, PAR_WCS_OPTIONS, PREC_4, "max_s_chg",
		       "Scale change per iteration", 0.02);
	set_par_description(WCS_MAX_SCALE_CHANGE,
			    "Maximum scale change accepted when iterating the WCS "
			    "solution. This is an internal parameter few people "
			    "(if any) will want to change.");

	add_par_double(WCS_SEC_PER_PIX, PAR_WCS_OPTIONS, PREC_2, "default_scale",
		       "Default image scale", 1.5);
	set_par_description(WCS_SEC_PER_PIX,
			    "Default image scale used for setting the initial "
			    "WCS when this information is not available from "
			    "the frame header. In arc seconds per pixel"
			);

	add_par_int(WCSFIT_MODEL, PAR_WCS_OPTIONS, FMT_OPTION, "plate_model",
		       "Plate Model", PAR_WCSFIT_MODEL_AUTO);
	set_par_choices(WCSFIT_MODEL, wcsfit_models);
	set_par_description(WCSFIT_MODEL,
			    "Model used for plate to projection plane transformations. "
			    "Simple consists of translation, scale and rotation; "
			    "Linear is a general affine transformation, which includes shear; "
			    "Auto selects a plate model based on the number of star pairs."
			);
	add_par_int(WCS_REFRACTION_EN, PAR_WCS_OPTIONS, FMT_BOOL, "refraction",
		       "Calculate Refraction", 0);
	set_par_description(WCS_REFRACTION_EN,
			    "Enable the calculation of refracted positions of stars. "
			    "If the time and/or geographical coordinates aren't precise, "
			    "this option is better turned off."
		);


/* obs */
	add_par_double(OBS_LATITUDE, PAR_OBS_DEFAULTS, FMT_DEC, "latitude",
		       "Latitude of observing site", 44.430556);
	set_par_description(OBS_LATITUDE,
			    "Used to annotate frames' fits header, and for "
			    "calculation of objects' hour angle and airmass. "
			    "Set in decimal degress.");
	add_par_double(OBS_LONGITUDE, PAR_OBS_DEFAULTS, FMT_DEC, "longitude",
		       "Longitude of observing site", -26.113888);
	set_par_description(OBS_LONGITUDE,
			    "Used to annotate frames' fits header, and for "
			    "calculation of objects' hour angle and airmass. "
			    "Set in decimal degress, E longitudes negative.");
	add_par_double(OBS_ALTITUDE, PAR_OBS_DEFAULTS, PREC_1, "altitude",
		       "Altitude of observing site", 75);
	set_par_description(OBS_ALTITUDE,
			    "Used to annotate frames' fits header, and for "
			    "scintillation calculations; In meters above sea level.");
	add_par_string(OBS_OBSERVER, PAR_OBS_DEFAULTS, 0, "observer",
		       "Observer name", "R. Corlan");
	set_par_description(OBS_OBSERVER,
			    "Used to annotate frames' fits header. ");
	add_par_string(OBS_OBSERVER_CODE, PAR_OBS_DEFAULTS, 0, "obscode",
		       "Observer code", "CXR");
	set_par_description(OBS_OBSERVER_CODE,
			    "Used to annotate AAVSO format reports. ");
	add_par_string(OBS_TELESCOP, PAR_OBS_DEFAULTS, PREC_1, "telescope",
		       "Telescope name", "SCT");
	set_par_description(OBS_TELESCOP,
			    "Used to annotate frames' fits header. ");
	add_par_double(OBS_FLEN, PAR_OBS_DEFAULTS, PREC_1, "flen",
		       "Telescope focal length", 200);
	set_par_description(OBS_FLEN,
			    "Used to annotate frames' fits header, and for "
			    "calculating the initial WCS image scale "
			    "(together with the camera pixel size). In centimeters." );
	add_par_int(OBS_FLIPPED, PAR_OBS_DEFAULTS, FMT_BOOL, "flipped",
		       "Flipped field", 0);
	set_par_description(OBS_FLIPPED,
			    "Set the inital wcs as flipped (CDELT1 and CDELT2 "
			    "having opposite signs). The star matching algorithm "
			    "does not handle flips, so this has to be set right. "
			    "A normal (non-flipped) field has N up and W to the right."
		);
	add_par_double(OBS_APERTURE, PAR_OBS_DEFAULTS, PREC_1, "aperture",
		       "Telescope aperture", 30);
	set_par_description(OBS_APERTURE,
			    "Used to annotate frames' fits header, and for "
			    "estimating scintillation noise. In centimeters."
			);
	add_par_string(OBS_FOCUS, PAR_OBS_DEFAULTS, PREC_1, "focus",
		       "Focus designation", "Cassergrain");
	set_par_description(OBS_FOCUS,
			    "Used to annotate frames' fits header."
		);
	add_par_string(OBS_FILTER_LIST, PAR_OBS_DEFAULTS, PREC_1, "filters",
		       "List of filters", "v r b i RI GI BI clear");
	set_par_description(OBS_FILTER_LIST,
			    "Space-delimited list of filters used to set the "
			    "choices in the obscript dialog. They are also used "
			    "as default names for the filters in filter wheels "
			    "that don't supply their own names"
			    );

	add_par_int(FILE_DEFAULT_CFA, PAR_FILES, 0, "default_cfa",
		    "CFA to use with raw FITS files", PAR_DEFAULT_CFA_NONE);
	set_par_choices(FILE_DEFAULT_CFA, default_cfa);

	add_par_string(FILE_GPS_SERIAL, PAR_FILES, 0, "gps_serial",
		       "Serial line for gps receiver control", "/dev/ttyS0");
	add_par_string(FILE_UNCOMPRESS, PAR_FILES, 0, "uncompress",
		       "Command for uncompressing files", "zcat");
	set_par_description(FILE_UNCOMPRESS,
			    "The command used to uncompress files; it should "
			    "take the filename as an argument, and output "
			    "the uncompressed stream on stdout. The command "
			    "is called when trying to open files with the name "
			    "ending in .gz, .zip or .z");
	add_par_string(FILE_COMPRESS, PAR_FILES, 0, "compress",
		       "Command for compressing files", "gzip -f");
	set_par_description(FILE_COMPRESS,
			    "The command used to uncompress files; it should "
			    "take the filename as an argument, and replace the file "
			    "with the compressed version." );
	add_par_string(FILE_GNUPLOT, PAR_FILES, 0, "gnuplot",
		       "Gnuplot command", "gnuplot -persist");
	set_par_description(FILE_GNUPLOT,
			    "Command used to invoke gnuplot for plotting graphs in a window.");
	add_par_int(FILE_PLOT_TO_FILE, PAR_FILES, FMT_BOOL, "plot_to_file",
		       "Plot to file", 0);
	set_par_description(FILE_PLOT_TO_FILE,
			    "Create a gnuplot file instead of calling gnuplot directly.");
	add_par_string(MONO_FONT, PAR_FILES, 0, "mono_font",
		       "Monospaced Font",
		       "-misc-fixed-medium-r-*-*-*-120-*-*-*-*-*-*");
	set_par_description(MONO_FONT,
			    "The monospaced font used to display "
			    "FITS headers and help pages."
			    );
	add_par_string(FILE_PHOT_OUT, PAR_FILES, 0, "phot_out_fn",
		       "Phot report file", "phot.out");
	set_par_description(FILE_PHOT_OUT,
			    "Output file for obscript phot "
			    "commands."
			    );
	add_par_string(FILE_OBS_PATH, PAR_FILES, 0, "obs_path",
		       "Obs files search path", ".:../obs");
	set_par_description(FILE_OBS_PATH,
			    "Path (colon-delimited list of directories) "
			    "searched when opening observation script files."
		);
	add_par_string(FILE_RCP_PATH, PAR_FILES, 0, "rcp_path",
		       "Rcp files search path", ".:../rcp");
	set_par_description(FILE_RCP_PATH,
			    "Path (colon-delimited list of directories) "
			    "searched when opening recipe files."
		);
	add_par_string(FILE_GSC_PATH, PAR_FILES, 0, "gsc_path",
		       "GSC location", "/usr/share/gcx/catalogs/gsc");
	set_par_description(FILE_GSC_PATH,
			    "Location of gsc catalog files. Full "
			    "pathname of the toplevel gsc directory."
			    "Make sure all subdirs and files names use lower case.");
	add_par_string(FILE_TYCHO2_PATH, PAR_FILES, 0, "tycho2_path",
		       "Tycho2 location", "/usr/share/gcx/catalogs/tycho2");
	set_par_description(FILE_TYCHO2_PATH,
			    "Full pathname of the tycho2 catalog file.");

	add_par_string(FILE_UCAC4_PATH, PAR_FILES, 0, "ucac4_path",
		       "UCAC4 location", "/usr/share/gcx/catalogs/ucac4/u4b/");
	set_par_description(FILE_UCAC4_PATH,
			    "Full pathname of the UCAC4 catalog files.");

	add_par_string(FILE_CATALOG_PATH, PAR_FILES, 0, "catalog_path",
		       "Catalog files", "/usr/share/gcx/catalogs/*.gcx");
	set_par_description(FILE_CATALOG_PATH,
			    "Colon-delimited list of catalog files "
			    "to be searched or loaded into the in-memory catalog. "
			    "The elements of the list are glob- and tilde-expanded "
			    "so for instance ~/*.gcx will load all .gcx files from "
			    "the home directory. The files are expected to be in "
			    "gcx recipe format."
			    );
	add_par_int(FILE_PRELOAD_LOCAL, PAR_FILES, FMT_BOOL, "catalog_preload",
		       "Preload catalogs", 0);
	set_par_description(FILE_PRELOAD_LOCAL,
			    "Enable preloading the native format catalog files "
			    "into memory for faster searching. Preloading takes "
			    "significantly more time that searching the on-disk "
			    "files by name; it's recommended only for merging catalogs "
			    "or similar operations."
			    );

	add_par_string(FILE_EDB_DIR, PAR_FILES, 0, "edb_dir",
		       "Edb directory", "/usr/share/gcx/catalogs/edb");
	set_par_description(FILE_EDB_DIR,
			    "Directory containing edb object files.");

	add_par_string(FILE_AAVSOVAL, PAR_FILES, 0, "validation",
		       "Aavso validation file", "valnam.txt");
	set_par_description(FILE_AAVSOVAL,
			    "Location of aavso validation file. The validation file "
			    "is used to determine the designation given a star name. "
			    "Any of the validation file formats can be used, except "
			    "the 'short form by name'. The designation should start "
			    "in column 0 and the name in column 10.");
	add_par_int(FILE_UNSIGNED_FITS, PAR_FILES, FMT_BOOL, "unsigned_fits",
		       "Force unsigned FITS", 0);
	set_par_description(FILE_UNSIGNED_FITS,
			    "Force all values read from a fits file to be interpreted "
			    "as unsigned numbers (supporting broken files generated that way)."
		);
	add_par_int(FILE_WESTERN_LONGITUDES, PAR_FILES, FMT_BOOL, "west_long",
		       "Western longitudes", 1);
	set_par_description(FILE_WESTERN_LONGITUDES,
			    "Interpret fits header longitudes as western, rather than "
			    "eastern. Report file longitudes are always western. "
		);


#define TAB_FORMAT_DEF "name ra dec mjd smag \"v\" flags"
	add_par_string(FILE_TAB_FORMAT, PAR_FILES, 0, "tab_format",
		       "Report converter output format", TAB_FORMAT_DEF);
	set_par_description(FILE_TAB_FORMAT,
			    "The output format string used by the report converter. "
			    "See the on-line help for a description."
		);
	add_par_int(FILE_SAVE_MEM, PAR_FILES, FMT_BOOL, "mem_save",
		    "Auto-unload frames", 1);
	set_par_description(FILE_SAVE_MEM,
			    "Unload unmodified frames from the frame list whenever "
			    "possible to reduce memory usage. The frames will have "
			    "to be reloaded if needed again."
		);

	add_par_int(FILE_NEW_WIDTH, PAR_FILES, 0, "new_width",
		    "New frame width", 1024);
	set_par_description(FILE_NEW_WIDTH,
			    "The width of a newly-created blank frame in pixels."
		);

	add_par_int(FILE_NEW_HEIGHT, PAR_FILES, 0, "new_height",
		    "New frame height", 768);
	set_par_description(FILE_NEW_HEIGHT,
			    "The height of a newly-created blank frame in pixels."
		);

	add_par_double(FILE_NEW_SECPERPIX, PAR_FILES, PREC_2, "new_scale",
		    "New frame scale", 4);
	set_par_description(FILE_NEW_SECPERPIX,
			    "The default wcs scale of a newly-created blank frame "
			    "in arc seconds per pixel."
		);

/* aphot */
	add_par_double(AP_R1, PAR_APHOT, PREC_1, "r1",
		       "Radius of central aperture", 4);
	set_par_description(AP_R1,
			    "Radius, in pixels, of the central aperture from "
			    "which the star flux is measured."
		);
	add_par_double(AP_R2, PAR_APHOT, PREC_1, "r2",
		       "Inner radius of sky annulus ", 9);
		set_par_description(AP_R2,
			    "Inner radius, in pixels, of the annulus "
			    "surrounding the star from which the sky flux is "
			    "estimated."
		);
	add_par_double(AP_R3, PAR_APHOT, PREC_1, "r3",
		       "Outer radius of sky annulus ", 18);
	set_par_description(AP_R3,
			    "Outer radius, in pixels, of the annulus "
			    "surrounding the star from which the sky flux is "
			    "estimated."
		);
/*
	add_par_int(AP_SHAPE, PAR_APHOT, FMT_OPTION, "shape",
		    "Aperture shape", PAR_SHAPE_IRREG);
	set_par_choices(AP_SHAPE, ap_shapes);
	set_par_description(AP_SHAPE,
			    "Shape of central aperture. "
			    "whole-pixels includes in the central aperture the pixels "
			    "whose centeres fall inside the aperture radius; irreg-poligon "
			    "weights the pixels that intersect the aperture boundary "
			    "according to the distance between their centers and the "
			    "boundary, thus approximating the circular aperture with a "
			    "irregular polygon. The irreg-poligon method is significantly "
			    "more accurate than whole-pixels "
			    "and works best with integer aperture radiuses."
		);
*/
	add_par_int(AP_AUTO_CENTER, PAR_APHOT, FMT_BOOL, "auto_center",
		    "Center apertures", 1);
	set_par_description(AP_AUTO_CENTER,
			    "Try to center the measuring aperture on "
			    "the star before performing the measurement."
		);
	add_par_double(AP_MAX_CENTER_ERR, PAR_APHOT, PREC_2, "center_err",
		       "Max centering error", 1.0);
	set_par_description(AP_MAX_CENTER_ERR,
			    "The maximum amount a measuring aperture is moved "
			    "when auto-centering (in pixels)."
		);
	add_par_int(AP_DISCARD_UNLOCATED, PAR_APHOT, FMT_BOOL, "discard_unlocated",
		    "Discard unlocated standards", 1);
	set_par_description(AP_DISCARD_UNLOCATED,
			    "When this option is set, standard stars than cannot be located "
			    "within the specified error radius when centering apertures are "
			    "not used in the solution."
		);
	add_par_int(AP_MOVE_TARGETS, PAR_APHOT, FMT_BOOL, "move_targets",
		    "Move targets", 0);
	set_par_description(AP_MOVE_TARGETS,
			    "Update the world coordinates of stars that don't have the "
			    "Astrometric flag set when "
			    "their apertures are centered. Useful for moving targets "
			    "like asteroids or comets or instances where the coordinates "
			    "of the target are only approximately known."
		);
	add_par_double(AP_MAX_STD_RADIUS, PAR_APHOT, PREC_2, "std_max_distance",
		    "Max center distance", 1.0);
	set_par_description(AP_MAX_STD_RADIUS,
			    "The maximum distance from the frame center at which a "
			    "standard star must lie in order to be used in the photometry "
			    "solution. The distance is expressed relative to half the "
			    "frame's diagonal. A value of 1 will include all stars. "
			    "This option is useful when we expect large flat-fielding errors "
			    "or distorted star images at the frame's corners."
		);



	add_par_int(AP_SKY_METHOD, PAR_APHOT, 0, "sky_method",
		    "Sky method", PAR_SKY_METHOD_KAPPA_SIGMA);
	set_par_choices(AP_SKY_METHOD, sky_methods);
	set_par_description(AP_SKY_METHOD,
			    "Method used for sky estimation: Average uses the "
			    "average of the sky annulus pixels; it's not recommended, "
			    "as it will not reject nearby stars. "
			    "Median uses the median of the sky pixels; it's fast and "
			    "robust, but will not average out quantisation noise. "
			    "Kappa-Sigma computes the sky value by clipping off pixels "
			    "too far away from the median, and averaging the rest; the standard "
			    "deviation of the remaining pixels is calculated, and the "
			    "clipping/averaging is iterated until values converge; "
			    "Synthetic mode is calaulated as 3*median - 2*mean, where median "
			    "and mean are the statistics of the clipped "
			    "sky annulus. "
		);
	add_par_int(AP_SKY_GROW, PAR_APHOT, 0, "grow",
		    "Region growing", 2);
	set_par_description(AP_SKY_GROW,
			    "Amount, in pixels, by which regions containing peaks "
			    "are grown in order to exclude the tails of spurious "
			    "stars in the sky aperture. A value larger than 3 is silently "
			    "clamped at 3. Region growing only applies to the kappa-sigma and "
			    "synthetic mode sky methods."
		);

	add_par_double(AP_SIGMAS, PAR_APHOT, PREC_1, "sigmas",
		       "Sigmas", 3);
	set_par_description(AP_SIGMAS,
			    "Rejection limit of the mean-median, kappa-sigma and synthetic mode "
			    "sky estimation methods, in sigmas."
		);
/*	add_par_int(AP_MAX_ITER, PAR_APHOT, 0, "max_iter",
		       "Iteration limit", 20);
	set_par_description(AP_MAX_ITER,
			    "Maximum number of iterations of the kappa-sigma sky estimation "
			    "method."
		);
*/
	add_par_double(AP_SATURATION, PAR_APHOT, 0, "sat_limit",
		       "Saturation limit", 48000.0);
	set_par_description(AP_SATURATION,
			    "The value over which we mark the stars as being \"bright\", "
			    "i.e. possibly saturated.");
	add_par_string(AP_IBAND_NAME, PAR_APHOT, 0, "iband",
		       "Instrumental band",
		       "v");
	set_par_description(AP_IBAND_NAME,
			    "Name of band the stars are measured into. Used when the frame "
			    "being reduced does not specify a filter or the Force iband "
			    "option is set.");
	add_par_int(AP_FORCE_IBAND, PAR_APHOT, FMT_BOOL, "force_iband",
		       "Force iband ", 0);
	set_par_description(AP_FORCE_IBAND,
			    "Override the filter specification from the frame header "
			    "with the one set in the Instrumental band parameter.");
// 	add_par_string(AP_SBAND_NAME, PAR_APHOT, 0, "sband",
//		       "Standard band", "v");
//	set_par_description(AP_SBAND_NAME,
//			    "The name of the standard band the data is reduced to.");
//	add_par_string(AP_COLOR_NAME, PAR_APHOT, 0, "color",
//		       "Color index",
//		       "b-v");
//	set_par_description(AP_COLOR_NAME,
//			    "The color index used for transformation.");
//	add_par_int(AP_USE_STD_COLOR, PAR_APHOT, 0, "transform_color",
//		    "Color type",
//		    PAR_COLOR_TYPE_INSTRUMENTAL);
//	set_par_description(AP_USE_STD_COLOR,
//			    "The type of color (instrumental or standard) used "
//			    "for transformation." );
//	set_par_choices(AP_USE_STD_COLOR, color_types);
//	add_par_int(AP_FIT_COLTERM, PAR_APHOT, FMT_BOOL, "fit_colterm",
//		       "Fit color", 0);
//	set_par_description(AP_FIT_COLTERM,
//			    "Attempt to fit the color trasformation "
//			    "coefficient from the frame's data.");
//	add_par_double(AP_COLTERM, PAR_APHOT, 0, "colterm",
//		       "Color coefficient", 0.0);
//	set_par_description(AP_COLTERM,
//			    "The color transformation coefficient value. Used "
//			    "when fit color is off.");
//	add_par_double(AP_COLTERM_ERR, PAR_APHOT, 0, "colterm_err",
//		       "Relative error of color term", 0.0);
//	set_par_description(AP_COLTERM_ERR,
//			    "Error of color transformation coefficient.");
	add_par_double(AP_DEFAULT_STD_ERROR, PAR_APHOT, 0, "def_std_err",
		       "Default catalog error", 0.05);
	set_par_description(AP_DEFAULT_STD_ERROR,
			    "Default standard magnitude error used "
			    "when a value is not available.");
	add_par_double(AP_ALPHA, PAR_APHOT, 0, "alpha",
		       "Alpha", 2.0);
	set_par_description(AP_ALPHA,
			    "Parameter of the robust fitting alogorithm. It sets "
			    "the fwhm of the rejection function (in standard deviations).");
	add_par_double(AP_BETA, PAR_APHOT, 0, "beta",
		       "Beta", 2.0);
	set_par_description(AP_BETA,
			    "Parameter of the robust fitting alogorithm. It sets "
			    "the sharpness of the rejection function, with higher values "
			    "resulting in a sharper falloff.");
	add_par_double(AP_MERGE_POSITION_TOLERANCE, PAR_FILES, 0, "merge_tolerance",
		       "Merge positional tolerance ", 3.0);
	set_par_description(AP_MERGE_POSITION_TOLERANCE,
			    "The distance in arc seconds below which we consider "
			    "two stars to be the same object when merging recipe files."
			    );
/* mband */

	add_par_string(AP_BANDS_SETUP, PAR_MBAND, 0, "bands",
		       "Bands setup", "b(b-v) v(b-v) r(v-r) i(v-i)");
	set_par_description(AP_BANDS_SETUP,
			    "A string which specifies the color indices against which "
			    "we reduce various bands. It consists of a space-separated "
			    "list of specifiers of the form: band(b1-b2)[=k/kerr]. In this case, "
			    "\"band\" will "
			    "be reduced against the b1-b2 color index. The optional k and kerr "
			    "set the default transformation coefficient and it's error for the "
			    "specified band, like for example v(b-v)=0.06/0.002."
			    );
	add_par_double(MB_OUTLIER_THRESHOLD, PAR_MBAND, 0, "outlier",
		       "Outlier threshold ", 3.0);
	set_par_description(MB_OUTLIER_THRESHOLD,
			    "Threshold in standard error over which we flag stars as "
			    "\"outliers\" for easy identification. "
			    "Outliers are not excluded from the fit; "
			    "they are down-weighted by a factor depending "
			    "on the values of the alpha and beta robust fit parameters. "
			    );
	add_par_double(MB_ZP_OUTLIER_THRESHOLD, PAR_MBAND, 0, "zp_outlier",
		       "Zeropoint outlier threshold ", 2.0);
	set_par_description(MB_ZP_OUTLIER_THRESHOLD,
			    "Threshold in standard error over which an image frame "
			    "is marked as an outlier of the extinction fit. "
			    "The existance of an outlier frame usually shows that "
			    "transparency has had a significant change around that time. "
			    "As a result, calculation of all-sky zeropoints is inhibited "
			    "in the vicinity of an outlier frame, until a well-fitting frame "
			    "is encountered. "
			    );
	add_par_double(MB_MIN_AM_VARIANCE, PAR_MBAND, PREC_4, "min_am_variance",
		       "Minimum airmass variance ", 0.001);
	set_par_description(MB_MIN_AM_VARIANCE,
			    "The minimum variance of a data set's airmass for which we try to "
			    "fit an extinction coefficient. For datasets with lower variance, "
			    "the extinction coefficient is not considered - which doesn't "
			    "affect the result, as all frames are taken at nearly the same "
			    "airmass."
			);
	add_par_double(MB_MIN_COLOR_VARIANCE, PAR_MBAND, PREC_4, "min_color_variance",
		       "Minimum color variance ", 0.1);
	set_par_description(MB_MIN_COLOR_VARIANCE,
			    "The minimum variance of a data set's color index for which we try to "
			    "fit a transformation coefficient. For datasets with lower variance, "
			    "the transformation coefficient is not fitted, as the "
			    "possibility of obtaining a meaningless value is quite real."
			);
	add_par_double(MB_AIRMASS_BRACKET_OVSIZE, PAR_MBAND, 0, "airmass_range",
		       "Airmass range ", 2.0);
	set_par_description(MB_AIRMASS_BRACKET_OVSIZE,
			    "Relative size of the range of airmasses over which we calculate "
			    "frame zeropoints when doing all-sky reduction. It is expressed "
			    "relative to the airmass range of the valid standard frames. "
			    "For example, if the standard frames are taken at airmasses "
			    "between 1.5 and 2.0, an airmass range of 2.0 will enable "
			    "all-sky zeropoint calculations for frames with airmasses between "
			    "1.25 and 2.25."
		);
	add_par_double(MB_AIRMASS_BRACKET_MAX_OVSIZE, PAR_MBAND, 0, "airmass_max_range",
		       "Airmass maximum range ", 0.5);
	set_par_description(MB_AIRMASS_BRACKET_MAX_OVSIZE,
			    "The absolute maximum amount we add to or subtract from the "
			    "standard frames' airmass range when calculating the range in which "
			    "frames which are reduced all-sky must lie."
		);
	add_par_double(MB_LMAG_FROM_ZP, PAR_MBAND, 0, "lmag_from_zp",
		       "Limiting magnitude from zeropoint ", 7.0);
	set_par_description(MB_LMAG_FROM_ZP,
			    "The amount we subtract from the zeropoint of a frame "
			    "in order to obtain the limiting magnitude used for "
			    "reporting purposes."
		);

        /* ccdred */
	add_par_double(CCDRED_OVERSCAN_PEDESTAL, PAR_CCDRED, PREC_1, "overscan_pedestal",
		       "Overscan correction pedestal", 1000);

	add_par_int(CCDRED_EFPMIN1, PAR_CCDRED, PREC_1, "effective_min1",
		    "First horizontal effective pixel", 34);
	add_par_int(CCDRED_EFPRNG1, PAR_CCDRED, PREC_1, "effective_rng1",
		    "Number of horizontal effective pixels", 3364);
	add_par_int(CCDRED_EFPMIN2, PAR_CCDRED, PREC_1, "effective_min2",
		    "First vertical effective pixel", 28);
	add_par_int(CCDRED_EFPRNG2, PAR_CCDRED, PREC_1, "effective_rng2",
		    "Number of vertical effective pixels", 2542);

	add_par_int(CCDRED_OVSMIN1, PAR_CCDRED, PREC_1, "overscan_min1",
		    "First horizontal overscan pixel", 3500);
	add_par_int(CCDRED_OVSRNG1, PAR_CCDRED, PREC_1, "overscan_rng1",
		    "Number of horizontal overscan pixels", 50);
	add_par_int(CCDRED_OVSMIN2, PAR_CCDRED, PREC_1, "overscan_min2",
		    "First vertical overscan pixel", 50);
	add_par_int(CCDRED_OVSRNG2, PAR_CCDRED, PREC_1, "overscan_rng2",
		    "Number of vertical overscan pixels", 2400);


	add_par_int(CCDRED_DEMOSAIC_METHOD, PAR_CCDRED, 0, "demosaic_method",
		    "Method used for demosaic", PAR_DEMOSAIC_METHOD_BILINEAR);
	set_par_choices(CCDRED_DEMOSAIC_METHOD, demosaic_methods);

	add_par_int(CCDRED_WHITEBAL_METHOD, PAR_CCDRED, 0, "whitebal_method",
		    "Method used for white-balance correction", PAR_WHITEBAL_METHOD_NONE);
	set_par_choices(CCDRED_WHITEBAL_METHOD, whitebal_methods);

	add_par_int(CCDRED_ALIGN_METHOD, PAR_CCDRED, 0, "align_method",
		    "Method used for image alignment", PAR_ALIGN_SHIFT_ONLY);
	set_par_choices(CCDRED_ALIGN_METHOD, align_methods);
	set_par_description(CCDRED_ALIGN_METHOD,
			    "Method used for aligning images. Shift will only perform "
			    "translations whereas full will also account for scale and "
			    "rotations.");
	add_par_int(CCDRED_RESAMPLE_METHOD, PAR_CCDRED, 0, "resample_method",
		    "Resampling algorithm", PAR_RESAMPLE_MITCHELL);
	set_par_choices(CCDRED_RESAMPLE_METHOD, resample_methods);
	set_par_description(CCDRED_RESAMPLE_METHOD,
			    "Resampling algorithm used for full image alignment.");

	add_par_double(MIN_BG_SIGMAS, PAR_CCDRED, PREC_1, "min_bg_sigmas",
		       "Minimum backgound sigmas", 3.0);
	set_par_description(MIN_BG_SIGMAS,
			    "Minimum number of sigmas the background must have for "
			    "multiplicative background alignment to be performed "
			    "when stacking frames.");

	add_par_int(CCDRED_STACK_METHOD, PAR_CCDRED, 0, "stack_method",
		    "Method used for stacking frames", PAR_STACK_METHOD_KAPPA_SIGMA);
	set_par_choices(CCDRED_STACK_METHOD, stack_methods);

	add_par_int(CCDRED_STACK_FRAMING, PAR_CCDRED, 0, "stack_framing",
		    "Framing of the stacked result", PAR_STACK_FRAMING_MOSAIC);
	set_par_choices(CCDRED_STACK_FRAMING, stack_framing_options);
	set_par_description(CCDRED_STACK_FRAMING,
			    "Choice of frame size for the result of stacking and alignment "
			    "operations. With mosaic, a frame large enough to include all "
			    "subframes will be used, align will clip the stack to the size of "
			    "the alignment frame and intersect will only keep the rectangle "
			    "covered by all subframes.");

	add_par_int(CCDRED_ITER, PAR_CCDRED, 0, "iterations",
		    "Iteration limit for stacking operations", 4);
	add_par_double(CCDRED_SIGMAS, PAR_CCDRED, PREC_1, "sigmas",
		    "Clipping sigmas for mmedian and k-s", 2);
	add_par_int(CCDRED_AUTO, PAR_CCDRED, FMT_BOOL, "auto",
		    "Run CCD reductions automatically on frame display", 1);

	add_par_double(CCDRED_BADPIX_SIGMAS, PAR_CCDRED, 0, "badpix_sigmas",
		       "Bad pixel sigmas", 12.0);
	set_par_description(CCDRED_BADPIX_SIGMAS,
			    "Number of frame sigmas a pixel must deviate from the "
			    "median of its neighbours to be considered defect.");

	/* INDI setup */
	add_par_string(INDI_HOST_NAME, PAR_INDI, 0, "indi_host_name",
		       "INDI server host", "localhost");
	add_par_int(INDI_PORT_NUMBER, PAR_INDI, 0, "indi_host_port",
		       "INDI server port", 7624);
	add_par_string(INDI_MAIN_CAMERA_NAME, PAR_INDI, 0, "camera_main_indi_name",
		       "INDI name of main camera", "CCD Simulator");
	add_par_string(INDI_MAIN_CAMERA_PORT, PAR_INDI, 0, "camera_main_indi_port",
		       "INDI port for main camera control", "/dev/video0");
	add_par_string(INDI_GUIDE_CAMERA_NAME, PAR_INDI, 0, "camera_guide_indi_name",
		       "INDI name of guide camera", "CCD Simulator");
	add_par_string(INDI_GUIDE_CAMERA_PORT, PAR_INDI, 0, "camera_guide_indi_port",
		       "INDI port for guide camera control", "/dev/video0");
	add_par_string(INDI_SCOPE_NAME, PAR_INDI, 0, "scope_indi_name",
		       "INDI name of Telescope", "Telescope Simulator");
	add_par_string(INDI_SCOPE_PORT, PAR_INDI, 0, "scope_indi_port",
		       "INDI port for telescope control", "/dev/ttyS0");
	add_par_string(INDI_FWHEEL_NAME, PAR_INDI, 0, "fwheel_indi_name",
		       "INDI name of filter wheel", "Filter Wheel Simulator");
	add_par_string(INDI_FWHEEL_PORT, PAR_INDI, 0, "fwheel_indi_port",
		       "INDI port for filter wheel control", "/dev/ttyS0");
	add_par_string(INDI_FOCUSER_NAME, PAR_INDI, 0, "focuser_indi_name",
		       "INDI name of main focuser", "Focuser Simulator");
	add_par_string(INDI_FOCUSER_PORT, PAR_INDI, 0, "focuser_indi_port",
		       "INDI port for main focuser", "/dev/ttyS0");

	/* telescope control */
	add_par_double(TELE_E_LIMIT, PAR_TELE, PREC_1, "elimit",
		       "Eastern limit for goto ops (hour angle)", -1.0);
	add_par_int(TELE_E_LIMIT_EN, PAR_TELE, FMT_BOOL, "elimit_en",
		    "Enable eastern limit", 0);
	add_par_double(TELE_W_LIMIT, PAR_TELE, PREC_1, "wlimit",
		       "Western limit for goto ops (hour angle)", 1.0);
	add_par_int(TELE_W_LIMIT_EN, PAR_TELE, FMT_BOOL, "wlimit_en",
		    "Enable western limit", 0);
	add_par_double(TELE_S_LIMIT, PAR_TELE, PREC_1, "slimit",
		       "Southern limit for goto ops (degrees)", -45.0);
	add_par_int(TELE_S_LIMIT_EN, PAR_TELE, FMT_BOOL, "slimit_en",
		       "Enable southern limit", 1);
	add_par_double(TELE_N_LIMIT, PAR_TELE, PREC_1, "nlimit",
		       "Northern limit for goto ops (degrees)", 80.0);
	add_par_int(TELE_N_LIMIT_EN, PAR_TELE, FMT_BOOL, "nlimit_en",
		       "Enable northern limit", 1);
	add_par_int(TELE_ABORT_FLIP, PAR_TELE, FMT_BOOL, "abort_flip",
		       "Abort obscript gotos that cause a meridian flip", 1);
	add_par_double(TELE_CENTERING_SPEED, PAR_TELE, PREC_1, "centering_speed",
		       "Mount speed when centering (times sidereal rate)", 32);
	add_par_double(TELE_GUIDE_SPEED, PAR_TELE, PREC_1, "centering_speed",
		       "Mount speed when guiding (times sidereal rate)", 0.5);
	add_par_int(TELE_USE_CENTERING, PAR_TELE, FMT_BOOL, "use_centering",
		       "Use timed centering moves for small slews", 1);
	add_par_int(TELE_PRECESS_TO_EOD, PAR_TELE, FMT_BOOL, "precess_eod",
		       "Precess coordinated sent to the scope to the epoch of the day", 1);
	add_par_double(TELE_GEAR_PLAY, PAR_TELE, PREC_2, "gear_play",
		       "Amount of gear play we take out at the end of slews (degrees)", 0.1);
	add_par_int(TELE_STABILISATION_DELAY, PAR_TELE, 0, "stabilisation_delay",
		       "Mount stabilisation delay (ms)", 1000);
	add_par_double(TELE_DITHER_AMOUNT, PAR_TELE, PREC_1, "dither",
		       "Amount of pointing dither between frames (arcmin)", 0.5);
	add_par_int(TELE_DITHER_ENABLE, PAR_TELE, FMT_BOOL, "dither_en",
		       "Enable pointing dithering", 0);
	add_par_double(MAX_POINTING_ERR, PAR_TELE, PREC_2, "max_point_err",
		       "Max error we accept in telescope pointing (degrees)", 0.05);
//	add_par_int(TELE_TYPE, PAR_TELE, FMT_BOOL, "type",
//		       "Telescope mount protocol", 0);
//	set_par_choices(TELE_TYPE, tele_types);
	add_par_int(GUIDE_ALGORITHM, PAR_GUIDE, FMT_OPTION, "algorithm",
		    "Guiding algorithm", PAR_GUIDE_CENTROID);
	set_par_choices(GUIDE_ALGORITHM, guide_algorithms);
	set_par_description(GUIDE_ALGORITHM,
			    "Algorithm used to determine the guide star position error. "
			    "The reticle algorithm determines the error as the ratio between "
			    "the total flux in box-shaped areas located horisontally and "
			    "vertically around the target position; The ratioed reticle "
			    "tries to maintain the initial flux ratio, rather than a unit ratio. "
		);
	add_par_int(GUIDE_RETICLE_SIZE, PAR_GUIDE, 0, "reticle_size",
		    "Reticle size", 2);
	set_par_description(GUIDE_RETICLE_SIZE,
			    "Size of the reticle in pixels. Must be an even value." );
	add_par_int(GUIDE_CENTROID_AREA, PAR_GUIDE, 0, "centroid_box",
		    "Centroid area radius", 2);
	set_par_description(GUIDE_CENTROID_AREA,
			    "Radius of area inside which the target centroid is calculated." );
	add_par_int(GUIDE_BOX_SIZE, PAR_GUIDE, 0, "box_size",
		    "Guide box size", 16);
	set_par_description(GUIDE_BOX_SIZE,
			    "Size of box around the target guide star position used "
			    "by the guiding algorithm." );
	add_par_int(GUIDE_BOX_ZOOM, PAR_GUIDE, 0, "zoom",
		    "Guide box zoom", 4);
	set_par_description(GUIDE_BOX_ZOOM,
			    "Zoom level of the small image of the guide box." );

	/* synthetic stars */
	add_par_int(SYNTH_PROFILE, PAR_SYNTH, FMT_OPTION, "profile",
		       "Type of star profile", 0);
	set_par_description(SYNTH_PROFILE,
			    "Profile function used to generate synthetic stars.");
	set_par_choices(SYNTH_PROFILE, synth_profiles);

	add_par_double(SYNTH_FWHM, PAR_SYNTH, PREC_2, "fwhm",
		       "FWHM", 2.5);
	set_par_description(SYNTH_FWHM,
			    "The FWHM of synthetically-generated stars in pixels.");

	add_par_double(SYNTH_MOFFAT_BETA, PAR_SYNTH, PREC_2, "beta",
		       "Moffat beta", 4);
	set_par_description(SYNTH_MOFFAT_BETA,
			    "The beta parameter of the Moffat function.");

	add_par_double(SYNTH_ZP, PAR_SYNTH, PREC_2, "zp",
		       "Zero Point", 23);
	set_par_description(SYNTH_ZP,
			    "Zero point (magnitude of a star with a total flux of 1) "
			    "used to scale synthetic stars.");

	add_par_int(SYNTH_OVSAMPLE, PAR_SYNTH, 0, "oversample",
		       "Oversampling", 11);
	set_par_description(SYNTH_OVSAMPLE,
			    "Oversampling factor when generating the "
			    "reference psf.");

}
