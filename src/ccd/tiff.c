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

/*  tiff.c -- handle TIFF files */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <tiffio.h>

#include "ccd.h"


int tiff_filename(char *filename)
{
	char *p;

	p = strchr(filename, '.');
	if (!p)
		return -1;

	p++;
	if (!strcasecmp(p, "tiff") || !strcasecmp(p, "tif"))
		return 0;

	return -1;
}

static int scanline_separate_to_frame(struct ccd_frame *fr,
				      tdata_t buf, uint32_t row,
				      uint16_t color, uint16_t ncolors,
				      uint16_t fmt, uint16_t bpp)
{
	float *dp = fr->dat;
	int i;

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
			// ignore alpha
			return 0;
		}
	}

	dp += row * fr->w;

	for (i = 0; i < fr->w; i++) {
		switch (fmt) {
		case SAMPLEFORMAT_INT:
			switch (bpp) {
			case 8:
				*dp++ = (float) *((int8_t *) buf + i);
				break;
			case 16:
				*dp++ = (float) *((int16_t *) buf + i);
				break;
			case 32:
				*dp++ = (float) *((int32_t *) buf + i);
				break;
			default:
				return -1;
			}
			break;

		case SAMPLEFORMAT_IEEEFP:
			if (bpp != 32)
				return -1;

			*dp++ = *((float *) buf + i);
			break;

		case SAMPLEFORMAT_UINT:
		default:
			switch (bpp) {
			case 8:
				*dp++ = (float) *((uint8_t *) buf + i);
				break;
			case 16:
				*dp++ = (float) *((uint16_t *) buf + i);
				break;
			case 32:
				*dp++ = (float) *((uint32_t *) buf + i);
				break;
			default:
				return -1;
			}
		}
	}

	return 0;
}

static int scanline_contig_to_frame(struct ccd_frame *fr,
				     tdata_t buf, uint32_t row, uint16_t ncolors,
				     uint16_t fmt, uint16_t bpp)
{
	float *dp = fr->dat;
	int i, color;

	for (color = 0; color < ncolors; color++) {
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
				// ignore alpha
				return 0;
			}
		}

		dp += row * fr->w;

		for (i = 0; i < fr->w; i++) {
			switch (fmt) {
			case SAMPLEFORMAT_INT:
				switch (bpp) {
				case 8:
					*dp++ = (float) *((int8_t *) buf + i * ncolors + color);
					break;
				case 16:
					*dp++ = (float) *((int16_t *) buf + i * ncolors + color);
					break;
				case 32:
					*dp++ = (float) *((int32_t *) buf + i * ncolors + color);
					break;
				default:
					return -1;
				}
				break;

			case SAMPLEFORMAT_IEEEFP:
				if (bpp != 32)
					return -1;

				*dp++ = *((float *) buf + i * ncolors + color);
				break;

			case SAMPLEFORMAT_UINT:
			default:
				switch (bpp) {
				case 8:
					*dp++ = (float) *((uint8_t *) buf + i * ncolors + color);
					break;
				case 16:
					*dp++ = (float) *((uint16_t *) buf + i * ncolors + color);
					break;
				case 32:
					*dp++ = (float) *((uint32_t *) buf + i * ncolors + color);
					break;
				default:
					return -1;
				}
			}
		}
	}

	return 0;
}



struct ccd_frame *read_tiff_file(char *filename)
{
	struct ccd_frame *fr = NULL;
	TIFF *tif;
	uint32_t w, h, row;
	uint16_t nsamples, config, bpp, fmt, color;
	tdata_t buf;
	int ret = 0;
	int i;

	if ((tif = TIFFOpen(filename, "r")) == NULL)
		return NULL;


	ret += TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	ret += TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	ret += TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
	ret += TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &fmt);
	ret += TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
	ret += TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bpp);

	/* we understand only the simplest of TIFF files */
	if (ret != 6)
		goto out;

	if (nsamples != 1 && nsamples != 3 && nsamples != 4)
		goto out;

	fr = new_frame(w, h);
	if (nsamples != 1) {
		free(fr->dat);
		alloc_frame_rgb_data(fr);
		fr->magic |= FRAME_VALID_RGB;
	}

	buf = _TIFFmalloc(TIFFScanlineSize(tif));
	if (config == PLANARCONFIG_CONTIG) {
		/* packed RGB(A) */
		for (row = 0; row < h; row++) {
			if (TIFFReadScanline(tif, buf, row, 0) != 1)
				goto out_err;

			if (scanline_contig_to_frame(fr, buf, row, nsamples, fmt, bpp))
				goto out_err;
		}
	} else if (config == PLANARCONFIG_SEPARATE) {
		/* separate planes */
		for (color = 0; color < nsamples; color++) {
			for (row = 0; row < h; row++) {
				if (TIFFReadScanline(tif, buf, row, color) != 1)
					goto out_err;

				if (scanline_separate_to_frame(fr, buf, row, color, nsamples, fmt, bpp))
					goto out_err;
			}
		}

	} else
		goto out_err;

	_TIFFfree(buf);

out:
	TIFFClose(tif);

	return fr;

out_err:
	_TIFFfree(buf);
	TIFFClose(tif);
	release_frame(fr);

	return NULL;
}
