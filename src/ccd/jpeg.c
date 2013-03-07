/*******************************************************************************
  Copyright(c) 2013 Matei Conovici. All rights reserved.

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

  Contact Information: mconovici@gmail.com
*******************************************************************************/

/*  jpeg.c -- handle JPEG files */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#include "config.h"
#include "ccd.h"

#ifdef HAVE_LIBJPEG

#include <jpeglib.h>

int jpeg_filename(char *filename)
{
	char *p;

	p = strchr(filename, '.');
	if (!p)
		return -1;

	p++;
	if (!strcasecmp(p, "jpeg") || !strcasecmp(p, "jpg"))
		return 0;

	return -1;
}

static void err_exit(j_common_ptr cinfo)
{
	longjmp(*(jmp_buf *)cinfo->client_data, 1);
}

static int scanline_to_frame(struct ccd_frame *fr, JSAMPLE *buf, int row, int ncolors)
{
	float *dp;
	int color, i;

	for (color = 0; color < ncolors; color++) {
		dp = fr->dat;
		if (ncolors != 1) {
			switch (color) {
			case 0:
				dp = fr->rdat;
				break;
			case 1:
				dp = fr->gdat;
				break;
			case 2:
				dp = fr->bdat;
				break;
			default:
				return 0;
			}
		}

		dp += row * fr->w;

		for (i = 0; i < fr->w; i++) {
			*dp++ = (float) *((JSAMPLE *) buf + i * ncolors + color);
		}
	}

	return 0;
}


struct ccd_frame *read_jpeg_file(char *filename)
{
	FILE *fin;
	struct ccd_frame *fr = NULL;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	int i;
	JSAMPLE *buf = NULL;
	jmp_buf errbuf;

	if ((fin = fopen(filename, "rb")) == NULL)
		return NULL;

	cinfo.err = jpeg_std_error(&jerr);

	/* override exit fn */
	cinfo.err->error_exit = err_exit;
	cinfo.client_data = &errbuf;

	/* have you seen my axe ? I have some error recovery to do */
	if (setjmp(errbuf)) {
		release_frame(fr);
		fr = NULL;
		goto out;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fin);

	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
		goto out;

	jpeg_start_decompress(&cinfo);

	if (cinfo.output_components != 1 && cinfo.output_components != 3)
		goto out;

	fr = new_frame(cinfo.output_width, cinfo.output_height);
	if (cinfo.output_components == 3) {
		free(fr->dat);
		alloc_frame_rgb_data(fr);
		fr->magic |= FRAME_VALID_RGB;
	}

	if ((buf = malloc(cinfo.output_width * cinfo.output_components * sizeof(JSAMPLE))) == NULL) {
		release_frame(fr);
		fr = NULL;
		goto out;
	}

	for (i = 0; i < cinfo.output_height; i++) {
		jpeg_read_scanlines(&cinfo, &buf, 1);
		scanline_to_frame(fr, buf, i, cinfo.output_components);
	}

out:
	free(buf);
	fclose(fin);
	jpeg_destroy_decompress(&cinfo);

	return fr;

}

#endif
