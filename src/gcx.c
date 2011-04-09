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

// gcx.c: main file for camera control program
// $Revision: 1.34 $
// $Date: 2009/09/27 15:29:51 $

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <errno.h>

#include "gcx.h"
#include "catalogs.h"
#include "gui.h"
#include "params.h"
#include "filegui.h"
#include "obslist.h"
#include "helpmsg.h"
#include "recipy.h"
#include "reduce.h"
#include "obsdata.h"
#include "multiband.h"
#include "query.h"

static void show_usage(void) {
	info_printf("%s", help_usage_page);
//	info_printf("\nFrame stacking options\n");
//	info_printf("-o <ofile>   Set output file name (for -c, -s, -a, -p) \n");
//	info_printf("-c <method>  combine files in the file list\n");
//	info_printf("             using the specified method. Valid methods\n");
//	info_printf("             are: mm(edian), av(erage), me(dian)\n");
//	info_printf("-s <method>  Stack (align and combine) files in the file\n");
//	info_printf("             list using same methods as -c\n");
//	info_printf("-a           align files in-place\n");
//	info_printf("-w <value>   Max FWHM accepted when auto-stacking frames [5.0]\n");
//	info_printf("\nFrame processing options\n");
//	info_printf("-d <dkfile>  Substract <dkfile> from files\n");
//	info_printf("-f <fffile>  Flatfield files using <fffile> (maintain avg flux)\n");
//	info_printf("-b <bfile>   Fix bad pixels in fits files using <bfile>\n");
//	info_printf("-m <mult>    Multiply files by the scalar <mult>\n");
//	info_printf("Any of -d, -f, -b and -m can be invoked at the same time; in this\n");
//	info_printf("    case, the order of processing is dark, flat, badpix, multiply\n");
//	info_printf("    These options will cause the frame stack options to be ingnored\n");
}

static void help_rep_conv(void) {
	printf("%s\n\n", help_rep_conv_page);
}

static void help_all(void) {
	printf("%s\n\n", help_usage_page);
	printf("%s\n\n", help_bindings_page);
	printf("%s\n\n", help_obscmd_page);
	printf("%s\n\n", help_rep_conv_page);
}

static int load_par_file(char *fn, GcxPar p)
{
	FILE *fp;
	int ret;

	fp = fopen(fn, "r");
	if (fp == NULL) {
		err_printf("load_par_file: cannot open %s\n", fn);
		return -1;
	}

	ret = fscan_params(fp, p);
	fclose(fp);
	return ret;
}

static int save_par_file(char *fn, GcxPar p)
{
	FILE *fp;
	fp = fopen(fn, "w");
	if (fp == NULL) {
		err_printf("save_par_file: cannot open %s\n", fn);
		return -1;
	}
	fprint_params(fp, p);
	fclose(fp);
	return 0;
}

int load_params_rc(void)
{
	uid_t my_uid;
	struct passwd *passwd;
	char *rcname;
	int ret = -1;

	my_uid = getuid();
	passwd = getpwuid(my_uid);
	if (passwd == NULL) {
		err_printf("load_params_rc: cannot determine home directoy\n");
		return -1;
	}
	if (-1 != asprintf(&rcname, "%s/.gcxrc", passwd->pw_dir)) {
		ret = load_par_file(rcname, PAR_NULL);
		free(rcname);
	}
	return ret;
}

int save_params_rc(void)
{
	uid_t my_uid;
	struct passwd *passwd;
	char *rcname;
	int ret = -1;

	my_uid = getuid();
	passwd = getpwuid(my_uid);
	if (passwd == NULL) {
		err_printf("save_params_rc: cannot determine home directoy\n");
		return -1;
	}
	if (-1 != asprintf(&rcname, "%s/.gcxrc", passwd->pw_dir)) {
		ret = save_par_file(rcname, PAR_NULL);
		free(rcname);
	}
	return ret;
}

static void recipe_file_convert(char *rcf, char *outf)
{
	FILE *infp = NULL, *outfp = NULL;
	int nc;
	if (rcf[0] != 0) {
		infp = fopen(rcf, "r");
		if (infp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   rcf, strerror(errno));
			exit(1);
		}
	}
	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
		if (outfp == NULL) {
			err_printf("Cannot open file %s for writing\n%s\n",
				   outf, strerror(errno));
			fclose(infp);
			exit(1);
		}
	}
	if (outfp && infp)
		nc = convert_recipe(infp, outfp);
	else if (outfp)
		nc = convert_recipe(stdin, outfp);
	else if (infp)
		nc = convert_recipe(infp, stdout);
	else
		nc = convert_recipe(stdin, stdout);

	if (nc < 0) {
		err_printf("Error converting recipe\n");
		if (infp)
			fclose(infp);
		if (outfp)
			fclose(outfp);
		exit(1);
	} else {
		if (infp)
			fclose(infp);
		if (outfp) {
			info_printf("%d star(s) written\n", nc);
			fclose(outfp);
		}
		exit (0);
	}
}

static void recipe_aavso_convert(char *rcf, char *outf)
{
	FILE *infp = NULL, *outfp = NULL;
	int nc;
	if (rcf[0] != 0) {
		infp = fopen(rcf, "r");
		if (infp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   rcf, strerror(errno));
			exit(1);
		}
	}
	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
		if (outfp == NULL) {
			err_printf("Cannot open file %s for writing\n%s\n",
				   outf, strerror(errno));
			fclose(infp);
			exit(1);
		}
	}
	if (outfp && infp)
		nc = recipe_to_aavso_db(infp, outfp);
	else if (outfp)
		nc = recipe_to_aavso_db(stdin, outfp);
	else if (infp)
		nc = recipe_to_aavso_db(infp, stdout);
	else
		nc = recipe_to_aavso_db(stdin, stdout);

	if (nc < 0) {
		err_printf("Error converting recipe\n");
		if (infp)
			fclose(infp);
		if (outfp)
			fclose(outfp);
		exit(1);
	} else {
		if (infp)
			fclose(infp);
		if (outfp) {
			info_printf("%d star(s) written\n", nc);
			fclose(outfp);
		}
		exit (0);
	}
}


static void report_convert(char *rcf, char *outf)
{
	FILE *infp = NULL, *outfp = NULL;

	if (rcf[0] != 0) {
		infp = fopen(rcf, "r");
		if (infp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   rcf, strerror(errno));
			exit(1);
		}
	}
	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
		if (outfp == NULL) {
			err_printf("Cannot open file %s for writing\n%s\n",
				   outf, strerror(errno));
			if (infp)
				fclose(infp);
			exit(1);
		}
	}
	if (outfp && infp)
		report_to_table(infp, outfp, NULL);
	else if (infp)
		report_to_table(infp, stdout, NULL);
	else if (outfp)
		report_to_table(stdin, outfp, NULL);
	else
		report_to_table(stdin, stdout, NULL);
	if (infp)
		fclose(infp);
	if (outfp)
		fclose(outfp);
	exit (0);
}

static void recipe_merge(char *rcf, char *mergef, char *outf, double mag_limit)
{
	FILE *infp = NULL, *outfp = NULL;
	FILE *mfp = NULL;

	if (rcf[0] != '-' || rcf[1] != 0) {
		infp = fopen(rcf, "r");
		if (infp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   rcf, strerror(errno));
			exit(1);
		}
	} else {
		infp = stdin;
	}
	if (mergef[0] != '-' || mergef[1] != 0) {
		mfp = fopen(mergef, "r");
		if (mfp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   mergef, strerror(errno));
			exit(1);
		}
	} else {
		mfp = stdin;
	}
	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
		if (outfp == NULL) {
			err_printf("Cannot open file %s for writing\n%s\n",
				   outf, strerror(errno));
			exit(1);
		}
	} else {
		outfp = stdout;
	}
	if (merge_rcp(infp, mfp, outfp, mag_limit) >= 0)
		exit (0);
	else
		exit (1);
}

static void recipe_set_tobj(char *rcf, char *obj, char *outf, double mag_limit)
{
	FILE *infp = NULL, *outfp = NULL;

	if (rcf[0] != '-' || rcf[1] != 0) {
		infp = fopen(rcf, "r");
		if (infp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   rcf, strerror(errno));
			exit(1);
		}
	} else {
		infp = stdin;
	}
	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
		if (outfp == NULL) {
			err_printf("Cannot open file %s for writing\n%s\n",
				   outf, strerror(errno));
			exit(1);
		}
	} else {
		outfp = stdout;
	}
	if (rcp_set_target(infp, obj, outfp, mag_limit) >= 0)
		exit (0);
	else
		exit (1);
}


static void catalog_file_convert(char *rcf, char *outf, double mag_limit)
{
	FILE *outfp = NULL;
	int nc;
	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
	}
	if (outfp)
		nc = convert_catalog(stdin, outfp, rcf, mag_limit);
	else
		nc = convert_catalog(stdin, stdout, rcf, mag_limit);
	if (nc < 0) {
		err_printf("Error converting catalog table\n");
		if (outfp) {
			fclose(outfp);
			unlink(outf);
		}
		exit(1);
	} else {
		if (outfp) {
			info_printf("%d star(s) written\n", nc);
			fclose(outfp);
		}
		exit (0);
	}
}

static void mb_reduce(char *mband, char *outf)
{
	FILE *outfp = stdout;
	FILE *infp;

	if (outf[0] != 0) {
		outfp = fopen(outf, "w");
		if (outfp == NULL) {
			err_printf("Cannot open file %s for writing\n%s\n",
				   outf, strerror(errno));
			exit(1);
		}
	}
	if (mband[0] != '-' || mband[1] != 0) {
		infp = fopen(mband, "r");
		if (infp == NULL) {
			err_printf("Cannot open file %s for reading\n%s\n",
				   mband, strerror(errno));
			exit(1);
		}
	} else {
		infp = stdin;
	}
	mband_reduce(infp, outfp);
	exit (0);
}


static void add_image_file_to_list(struct image_file_list *imfl, char *filename, int flags)
{
	struct image_file *imf;

	imf = image_file_new();

	imf->filename = strdup(filename);
	imf->flags = flags;

	imfl->imlist = g_list_append(imfl->imlist, imf);
}

static int set_wcs_from_object (struct ccd_frame *fr, char *name, double spp)
{
	struct cat_star *cats;

	cats = get_object_by_name(name);
	if (cats == NULL)
		return -1;
	fr->fim.wcsset = WCS_INITIAL;
	fr->fim.xref = cats->ra;
	fr->fim.yref = cats->dec;
	fr->fim.xrefpix = fr->w / 2;
	fr->fim.yrefpix = fr->h / 2;
	fr->fim.equinox = cats->equinox;
	fr->fim.rot = 0.0;
	fr->fim.xinc = spp / 3600.0;
	fr->fim.yinc = spp / 3600.0;
	if (P_INT(OBS_FLIPPED))
		fr->fim.yinc = -fr->fim.yinc;
	cat_star_release(cats);
	return 0;

}

static void make_tycho_rcp(char *obj, double tycrcp_box, char *outf, double mag_limit)
{
	FILE *of = NULL;
	if (obj[0] == 0) {
		err_printf("Please specify an object (with -j)\n");
		exit(1);
	}
	if (outf[0] != 0)
		of = fopen(outf, "w");
	if (of == NULL)
		of = stdout;
	if (!make_tyc_rcp(obj, tycrcp_box, of, mag_limit))
		exit (0);
	exit(1);
}

static void cat_rcp(char *obj, double cat_box, char *outf, double mag_limit)
{
	FILE *of = NULL;
	if (obj[0] == 0) {
		err_printf("Please specify an object (with -j)\n");
		exit(1);
	}
	if (outf[0] != 0)
		of = fopen(outf, "w");
	if (of == NULL)
		of = stdout;
	if (!make_cat_rcp(obj, "cat", cat_box, of, mag_limit))
		exit (0);
	exit(1);
}



int extract_bad_pixels(char *badpix, char *outf)
{
	struct image_file *imf;
	struct bad_pix_map *map;

	imf = image_file_new();
	imf->filename = strdup(badpix);

	if (load_image_file(imf) < 0)
		goto out_err;

	map = calloc(1, sizeof(struct bad_pix_map));
	map->filename = strdup(outf);

	if (find_bad_pixels(map, imf->fr, P_DBL(CCDRED_BADPIX_SIGMAS)) < 0)
		goto out_err;

	save_bad_pix(map);

out_err:
	exit(0);
}

int extract_sources(char *starf, char *outf)
{
	FILE *of = NULL;
	struct image_file *imf;
	struct sources *src;
	int i;

	imf = image_file_new();
	imf->filename = strdup(starf);

	if (load_image_file(imf) < 0)
		exit(1);

	src = new_sources(P_INT(SD_MAX_STARS));
	extract_stars(imf->fr, NULL, 0, P_DBL(SD_SNR), src);

	if (outf[0] != 0)
		of = fopen(outf, "w");

	if (of == NULL)
		of = stdout;

	for (i = 0; i < src->ns; i++) {
		if (src->s[i].peak > P_DBL(AP_SATURATION))
			continue;

		fprintf(of, "%.2f %.2f %.2f\n",
			src->s[i].x, src->s[i].y, src->s[i].flux);
	}

	fclose(of);

	exit(0);
}



static struct ccd_frame *make_blank_obj_fr(char *obj)
{
	struct ccd_frame *fr;
	fr = new_frame(P_INT(FILE_NEW_WIDTH), P_INT(FILE_NEW_HEIGHT));
	set_wcs_from_object(fr, obj, P_DBL(FILE_NEW_SECPERPIX));
	return fr;
}

#define NUM_AIRMASSES 8
#define NUM_EXPTIMES 16
static double amtbl[]={1.0, 1.2, 1.4, 1.7, 2.0, 2.4, 2.8, 3.5};
static double extbl[]={0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};
static void print_scint_table(char *apert)
{
	double d;
	int i, j;

	d = strtod(apert, NULL);
	if (d == 0) {
		err_printf("invalid aperture %s\n", apert);
		return;
	}

	printf("\n    Scintillation table for D=%.0fcm, sea level\n\n", d);
	printf("exp/am");
	for (i=0; i<NUM_AIRMASSES; i++)
		printf("\t%.1f", amtbl[i]);
	printf("\n\n");

	for (j = 0; j < NUM_EXPTIMES; j++) {
		printf("%.4g", extbl[j]);
		for (i = 0; i < NUM_AIRMASSES; i++) {
			printf("\t%.2g", scintillation(extbl[j], d, amtbl[i]));
		}
		printf("\n");
	}

}


int main(int ac, char **av)
{
	int ret, i;
	char *file;
	struct ccd_frame *fr = NULL;
	char repf[1024]=""; /* report file to be converted */
	char rf[1024]=""; /* recipe file to load */
	char ldf[1024]=""; /* landolt table file to be converted */
	char rcf[1024]=""; /* recipe file to be converted */
	char outf[1024]=""; /* outfile */
	char of[1024]=""; /* obsfile */
	char obj[1024] = ""; /* object */
	char mband[1024] = ""; /* argument to mband */
	char badpixf[1024] = ""; /* dark for badpix extraction */
	char starf[1024] = ""; /* star list */
	int to_pnm = 0;
	int run_phot = 0;
	int convert_recipe = 0;
	int rcp_to_aavso = 0;
	int rep_to_table = 0;
	int no_reduce = 0;
	int save_internal_cat = 0;
	int update_files = 0;
	int extract_badpix = 0;
	int extract_stars = 0;
	struct ccd_reduce *ccdr = NULL;
	struct image_file_list *imfl = NULL; /* list of frames to load / process */
	float mag_limit = 99.0; /* mag limit when generating rcp and cat files */
	char extrf[1024]=""; 	/* frame we extract targets from */
	char mergef[1024] = ""; /* rcp we merge stars from */
	char tobj[1024] = ""; 	/* object we set as target in the rcp */
	float tycrcp_box = 0.0;
	float cat_box = 0.0;
	int make_gpsf = 1;

	char *rc_fn = NULL;
	char oc;
	int interactive = 0; /* user option to run otherwise batch jobs in
			      * interactive mode */
	int batch = 0; /* batch operations have been selected */

	char *endp;
	double v;

	GtkWidget *window;

	char *shortopts = "D:p:hP:V:vo:id:b:f:B:M:A:O:usa:T:S:nG:Nj:FcX:e:";
	struct option longopts[] = {
		{"debug", required_argument, NULL, 'D'},
		{"dark", required_argument, NULL, 'd'},
		{"bias", required_argument, NULL, 'b'},
		{"flat", required_argument, NULL, 'f'},

		{"badpix", required_argument, NULL, 'B'},
		{"extract-badpix", required_argument, NULL, 'X'},

		{ "sextract", required_argument, NULL, 'e' },

		{"add-bias", required_argument, NULL, 'A'},
		{"multiply", required_argument, NULL, 'M'},
		{"gaussian-blur", required_argument, NULL, 'G'},

		{"output", required_argument, NULL, 'o'},
		{"update-file", no_argument, NULL, 'u'},

		{"stack", no_argument, NULL, 's'},
		{"superflat", no_argument, NULL, 'F'},
		{"align", required_argument, NULL, 'a'},
		{"no-reduce", no_argument, NULL, 'N'},
		{"set", required_argument, NULL, 'S'},

		{"rcfile", required_argument, NULL, 'r'},
		{"obsfile", required_argument, NULL, 'O'},
		{"recipe", required_argument, NULL, 'p'},
//		{"convert-rcp", required_argument, NULL, '2'},
		{"to-pnm", no_argument, NULL, 'n'},
		{"import", required_argument, NULL, '4'},
		{"save-internal-cat", no_argument, NULL, '7'},
		{"extract-targets", required_argument, NULL, '8'},
		{"mag-limit", required_argument, NULL, '9'},
		{"merge", required_argument, NULL, '0'},
		{"set-target", required_argument, NULL, '_'},
		{"make-tycho-rcp", required_argument, NULL, ']'},
		{"make-cat-rcp", required_argument, NULL, '>'},

		{"rep-to-table", required_argument, NULL, 'T'},
		{"phot-run", required_argument, NULL, 'P'},
		{"phot-run-aavso", required_argument, NULL, 'V'},
		{"interactive", no_argument, NULL, 'i'},
		{"help", no_argument, NULL, 'h'},
		{"help-all", no_argument, NULL, '3'},
		{"help-rep-conv", no_argument, NULL, '('},
		{"test", no_argument, NULL, ')'},
		{"version", no_argument, NULL, '1'},
		{"object", required_argument, NULL, 'j'},
		{"rcp-to-aavso", required_argument, NULL, '6'},
		{"multi-band-reduce", required_argument, NULL, '^'},
		{"options-doc", no_argument, NULL, '`'},
		{"scintillation", required_argument, NULL, ';'},

		{"demosaic", optional_argument, NULL, 'c' },

		{NULL, 0, NULL, 0}
	};

	setenv("LANG", "C", 1);

	init_ptable();
	load_params_rc();
	gtk_init (&ac, &av);

	debug_level = 0;

	while ((oc = getopt_long(ac, av, shortopts, longopts, NULL)) > 0) {
		switch(oc) {
		case ';':
			print_scint_table(optarg);
			exit(0);
		case '9':
			sscanf(optarg, "%f", &mag_limit);
			break;
		case '<':
			make_gpsf = 1;
			break;
		case ')':
//			test_query();
			return 0;
			break;
		case '`':
			doc_printf_par(stdout, PAR_FIRST, 0);
			exit(0);
			break;
		case '^':
			strncpy(mband, optarg, 1023);
			break;
		case '8':
			strncpy(extrf, optarg, 1023);
			batch = 1;
			break;
		case '0':
			strncpy(mergef, optarg, 1023);
			batch = 1;
			break;
		case '_':
			strncpy(tobj, optarg, 1023);
			batch = 1;
			break;
		case ']':
			sscanf(optarg, "%f", &tycrcp_box);
			batch = 1;
			break;
		case '>':
			sscanf(optarg, "%f", &cat_box);
			batch = 1;
			break;
		case 'D':
			sscanf(optarg, "%d", &debug_level);
			break;
		case 'N':
			no_reduce = 1;
			break;
		case 'h':
			show_usage();
			exit(0);
		case '3':
			help_all();
			exit(0);
		case '(':
			help_rep_conv();
			exit(0);
		case '1':
			info_printf("%s\n", VERSION);
			exit(0);
		case ':':
			err_printf("option -%c requires an argument\n", optopt);
			exit(1);
		case '?':
			err_printf("Try `gcx -h' for more information\n");
			exit(1);
		case 'i':
			interactive = 1;
			break;
		case 'n':
			to_pnm = 1;
			batch = 1;
			break;
		case 'O':
			strncpy(of, optarg, 1023);
			break;
		case 'o':
			strncpy(outf, optarg, 1023);
			break;
		case 'p':
			sscanf(optarg, "%800s", rf);
			break;
		case 'P':
			sscanf(optarg, "%800s", rf);
			run_phot = REP_ALL|REP_DATASET;
			batch = 1;
			break;
		case 'V':
			sscanf(optarg, "%800s", rf);
			run_phot = REP_TGT|REP_AAVSO;
			batch = 1;
			break;
		case '2':
			if (optarg) {
				if (optarg[0] == '-' && optarg[1] == 0)
					;
				else
					strncpy(rcf, optarg, 1023);
			}
			convert_recipe = 1;
			batch = 1;
			break;
		case '6':
			if (optarg) {
				if (optarg[0] == '-' && optarg[1] == 0)
					;
				else
					strncpy(rcf, optarg, 1023);
			}
			rcp_to_aavso = 1;
			batch = 1;
			break;
		case '7':
			save_internal_cat = 1;
			break;
		case 'S':
			if (optarg) {
				if (set_par_by_name(optarg))
					err_printf("error setting %s\n", optarg);
			}
			break;
		case 'T':
			if (optarg) {
				if (optarg[0] == '-' && optarg[1] == 0)
					;
				else
					strncpy(repf, optarg, 1023);
			}
			rep_to_table = 1;
			batch = 1;
			break;
		case '4':
			if (optarg)
				strncpy(ldf, optarg, 1023);
			break;
		case 'j':
			if (optarg)
				strncpy(obj, optarg, 1023);
//			interactive = 1;
			break;
		case 'u':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_INPLACE;
			update_files = 1;
			break;
		case 's':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_STACK;
			batch = 1;
			break;
		case 'F':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_STACK;
			ccdr->ops |= IMG_OP_BG_ALIGN_MUL;
			batch = 1;
			break;
		case 'a':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_ALIGN;
			ccdr->alignref = image_file_new();
			ccdr->alignref->filename = strdup(optarg);
			batch = 1;
			break;
		case 'b':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->bias = image_file_new();
			ccdr->bias->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_BIAS;
			batch = 1;
			break;
		case 'f':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->flat = image_file_new();
			ccdr->flat->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_FLAT;
			batch = 1;
			break;
		case 'd':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->dark = image_file_new();
			ccdr->dark->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_DARK;
			batch = 1;
			break;
		case 'G':
			v = strtod(optarg, &endp);
			if (endp != optarg) {
				if (ccdr == NULL)
					ccdr = ccd_reduce_new();
				ccdr->blurv = v;
				ccdr->ops |= IMG_OP_BLUR;
			}
			batch = 1;
			break;
		case 'M':
			v = strtod(optarg, &endp);
			if (endp != optarg) {
				if (ccdr == NULL)
					ccdr = ccd_reduce_new();
				ccdr->mulv = v;
				ccdr->ops |= IMG_OP_MUL;
			}
			batch = 1;
			break;
		case 'A':
			v = strtod(optarg, &endp);
			if (endp != optarg) {
				if (ccdr == NULL)
					ccdr = ccd_reduce_new();
				ccdr->addv = v;
				ccdr->ops |= IMG_OP_ADD;
			}
			batch = 1;
			break;

		case 'B':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->bad_pix_map = calloc(1, sizeof(struct bad_pix_map));
			ccdr->bad_pix_map->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_BADPIX;
			batch = 1;
			break;

		case 'X':
			extract_badpix = 1;
			strncpy(badpixf, optarg, 1023);
			break;

		case 'e':
			extract_stars = 1;
			strncpy(starf, optarg, 1023);
			break;

		case 'c':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_DEMOSAIC;
			batch = 1;
			break;
		}
	}


/* now run the various requested operations */

	if (rc_fn != NULL) {/* load extra rc file */
		load_par_file(rc_fn, PAR_NULL);
	}

	if (extract_badpix) {
		extract_bad_pixels(badpixf, outf); /* does not return */
	}

	if (extract_stars) {
		extract_sources(starf, outf); /* does not return */
	}

	if (mband[0] != 0) {
		mb_reduce(mband, outf); /* does not return */
	}
	if (tycrcp_box > 0.0) {
		make_tycho_rcp(obj, tycrcp_box, outf, mag_limit); /* does not return */
	}
	if (cat_box > 0.0) {
		cat_rcp(obj, cat_box, outf, mag_limit); /* does not return */
	}
	if (mergef[0] != 0) {
		if (rf[0] == 0) {
			err_printf("Please specify a recipe file to merge to (with -p)\n");
			exit(1);
		}
		recipe_merge(rf, mergef, outf, mag_limit); /* does not return */
	}

	if (tobj[0] != 0) {
		if (rf[0] == 0) {
			err_printf("Please specify a recipe file to set target in (with -p)\n");
			exit(1);
		}
		recipe_set_tobj(rf, tobj, outf, mag_limit); /* does not return */
	}

	if (convert_recipe) { /* convert recipe */
		recipe_file_convert(rcf, outf); /* does not return */
	}

	if (save_internal_cat) { /* save int catalog */
		FILE *outfp;
		if (outf[0] != 0) {
			outfp = fopen(outf, "w");
			if (outfp == NULL) {
				err_printf("Cannot open file %s for writing\n%s\n",
					   outf, strerror(errno));
				exit(1);
			}
			output_internal_catalog(outfp, mag_limit);
			fclose(outfp);
			exit(0);
		}
		output_internal_catalog(stdout, mag_limit);
		exit(0);
	}

	if (rcp_to_aavso) { /* convert recipe */
		recipe_aavso_convert(rcf, outf); /* does not return */
	}

	if (rep_to_table) { /* convert a report into tabular format */
		report_convert(repf, outf); /* does not return */
	}

	if (ldf[0] != 0) { /* import landolt or or other table */
		d3_printf("%s import\n", ldf); /* does not return */
		catalog_file_convert(ldf, outf, mag_limit);
	}

	if (of[0] != 0) { /* search path for obs file */
		if ((strchr(of, '/') == NULL)) {
			file = find_file_in_path(of, P_STR(FILE_OBS_PATH));
			if (file != NULL) {
				strncpy(of, file, 1023);
				of[1023] = 0;
				free(file);
			}
		}
		d3_printf("of is %s\n", of);
	}

	if (rf[0] != 0) { /* search path for recipe file */
		if ((strchr(of, '/') == NULL)) {
			file = find_file_in_path(rf, P_STR(FILE_RCP_PATH));
			if (file != NULL) {
				strncpy(rf, file, 1023);
				rf[1023] = 0;
				free(file);
			}
		}
		d3_printf("rf is '%s'\n", rf);
	}

	if (ac > optind) { /* we build the image list */
		imfl = image_file_list_new();
		for (i = optind; i < ac; i++)
			add_image_file_to_list(imfl, av[i], 0);
	}

	if (ccdr != NULL) {
		if (batch && !interactive) {
			if ((outf != NULL && outf[0] != 0) || update_files) {
				if (imfl == NULL) {
					err_printf("No frames to process, exiting\n");
					ccd_reduce_release(ccdr);
					exit(1);
				}
				d3_printf("outf is |%s|\n", outf);
				ret = batch_reduce_frames(imfl, ccdr, outf);
				ccd_reduce_release(ccdr);
				image_file_list_release(imfl);
				exit(ret);
			} else {
				info_printf("no output file name: going interactive\n");
				interactive = 1;
			}
		}
	}

	if ((imfl != NULL) && (ccdr != NULL) && !no_reduce) {
		fr = reduce_frames_load(imfl, ccdr);
		if (fr == NULL)
			exit (1);
	}

	window = create_image_window();
	if (!batch || interactive) {
		gtk_widget_show_all(window);
	}
	set_imfl_ccdr(window, ccdr, imfl);

	if (imfl == NULL && obj[0] && fr == NULL) {
		fr = make_blank_obj_fr(obj);
		if (fr != NULL) {
			frame_to_channel(fr, window, "i_channel");
			release_frame(fr);
		}
	}

	if ((ac > optind)) {
		if (fr == NULL) {
			fr = read_image_file(av[optind], P_STR(FILE_UNCOMPRESS),
					     P_INT(FILE_UNSIGNED_FITS),
					     default_cfa[P_INT(FILE_DEFAULT_CFA)]);
			if (fr) {
				rescan_fits_exp(fr, &fr->exp);
				rescan_fits_wcs(fr, &fr->fim);
			}
		}
		if (fr == NULL) {
			err_printf("cannot read file %s\n", av[optind]);
			exit(1);
		}
		if (obj[0] != 0)
			set_wcs_from_object(fr, obj, -P_DBL(WCS_SEC_PER_PIX));

		frame_to_channel(fr, window, "i_channel");
		release_frame(fr);
		if (to_pnm) {
			struct image_channel *channel;
			channel = g_object_get_data(G_OBJECT(window), "i_channel");
			if (channel == NULL) {
				err_printf("oops - no channel\n");
				if (batch && !interactive)
					exit(1);
			}
			if (outf[0] != 0) {
				ret = channel_to_pnm_file(channel, NULL, outf, 0);
			} else {
				ret = channel_to_pnm_file(channel, NULL, NULL, 0);
			}
			if (ret) {
				err_printf("error writing pnm file\n");
				if (batch && !interactive)
					exit(1);
			}
			if (batch && !interactive)
				exit(0);
		}
		if (rf[0] != 0) {
			FILE *output_file = NULL;
			load_rcp_to_window(window, rf, obj);
			match_field_in_window_quiet(window);
			if (run_phot) {
				if (outf[0] != 0) {
					output_file = fopen(outf, "a");
					if (output_file == NULL)
						err_printf("Cannot open file %s "
							   "for appending\n%s\n",
							   outf, strerror(errno));
				}
				if (output_file == NULL)
					phot_to_fd(window, stdout, run_phot);
				else {
					phot_to_fd(window, output_file, run_phot);
					fclose(output_file);
				}
			}
			if (batch && !interactive) {
				exit(0);

			}
		}
		d3_printf("calling gtk_main\n");
		gtk_main ();
		exit(0);
	}
	d3_printf("calling gtk_main, no frame\n");
	if (of[0] != 0) { /* run obs */
		if (run_obs_file(window, of))
			exit (1);
	}
	gtk_main ();
	exit(0);
}
