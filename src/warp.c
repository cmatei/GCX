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

// warp.c: non-linear image coordinates transform functions
// and other geometric transforms and filters

// $Revision: 1.23 $
// $Date: 2009/08/19 18:40:19 $

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "ccd.h"

float smooth3[9] = {0.25, 0.5, 0.25, 0.5, 1.0, 0.5, 0.25, 0.5, 0.25};

// fast 7x7 convolution; dp points to the upper-left of the convolution box
// in the image, ker is a linear vector that contains the kerner
// scanned l->r and t>b; w is the width of the image
static inline float conv7(float *dp, float *ker, int w)
{
	register int nl;
	register float d;

	nl = w - 6;
	d = *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	return d;
}

// fast 5x5 convolution; dp points to the upper-left of the convolution box
// in the image, ker is a linear vector that contains the kerner
// scanned l->r and t>b; w is the width of the image
static inline float conv5(float *dp, float *ker, int w)
{
	int nl;
	float d;

	nl = w - 4;
	d = *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	dp += nl;
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp++ * (*ker++);
	d += *dp * (*ker++);
	return d;
}

// fast 3x3 convolution; dp points to the upper-left of the convolution box
// in the image, ker is a linear vector that contains the kerner
// scanned l->r and t>b; w is the width of the image
static inline float conv3(float *dp, float *ker, int w)
{
	register int nl;
	register float d;

	nl = w - 2;
	d = *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	dp += nl;
	d += *dp++ * *ker++;
	d += *dp++ * *ker++;
	d += *dp * *ker++;
	return d;
}


// filter a frame using the supplied kernel
// size must be an odd number >=3
// returns 0 for ok, -1 for error
int filter_frame(struct ccd_frame *fr, struct ccd_frame *fro, float *kern, int size)
{
	int w;
	float *dpi;
	float *dpo;
	int i, all;
	int plane_iter = 0;

	if (size % 2 != 1 || size < 3 || size > 7) {
		err_printf("filter_frame: bad size %d\n");
		return -1;
	}

	w = fr->w;

	all = w * (fr->h - (size - 1)) - (size) ;

	while ((plane_iter = color_plane_iter(fr, plane_iter))) {
		dpi = get_color_plane(fr, plane_iter);
		dpo = get_color_plane(fro, plane_iter);


//		for(i=0; i<all; i++)
//			*dpo++ = *dpi++;
//		return;

//		d3_printf("dpi %x dpo %x\n", dpi, dpo);

		for (i = 0; i < (size / 2) * w + (size / 2); i++) {
			*dpo++ = fr->stats.cavg;
		}

//		d3_printf("all: %d (%d) w: %d allo: %d\n", all, w * fr->h, w,
//			  fro->w*fro->h);

//		d3_printf("dpi %x dpo %x\n", dpi, dpo);

		switch(size) {
		case 3:
			for (i=0; i<all; i++) {
				*dpo = conv3(dpi, kern, w);
				dpi ++;
				dpo ++;
			}
			break;
		case 5:
			for (i=0; i<all; i++) {
				*dpo = conv5(dpi, kern, w);
				dpi ++;
  				dpo ++;
			}
			break;
		case 7:
			for (i=0; i<all; i++) {
				*dpo = conv7(dpi, kern, w);
				dpi ++;
				dpo ++;
			}
			break;
		}

//		d3_printf("dpi %x dpo %x\n", dpi, dpo);

		for (i = 0; i < (size / 2) * w + (size / 2); i++) {
			*dpo++ = fr->stats.cavg;
		}
	}

	fr->stats.statsok = 0;

	return 0;

}

// same as filter_frame, but operates in-place
// allocs a new frame for temporary data, then frees it.
int filter_frame_inplace(struct ccd_frame *fr, float *kern, int size)
{
	struct ccd_frame *new;
	float *dpi, *dpo;
	int ret, all;
	int plane_iter = 0;

	new = clone_frame(fr);
	if (new == NULL)
		return -1;

	ret = filter_frame(fr, new, kern, size);
	if (ret < 0) {
		release_frame(new);
		return ret;
	}
	all = fr->w * fr->h;
	while ((plane_iter = color_plane_iter(fr, plane_iter))) {
		dpi = get_color_plane(fr, plane_iter);
		dpo = get_color_plane(new, plane_iter);
		memcpy(dpo, dpi, all * PIXEL_SIZE);
	}
	fr->stats.statsok = 0;
	release_frame(new);
	return ret;
}


// create a shift-only ctrans
// a positive dx will cause an image to shift to the right after the transform
int make_shift_ctrans(struct ctrans *ct, double dx, double dy)
{
	int i,j;

	ct->order = 1;

	for (i = 0; i < MAX_ORDER; i++)
		for (j = 0; j < MAX_ORDER; j++) {
			ct->a[i][j] = 0.0;
			ct->b[i][j] = 0.0;
		}
	ct->u0 = dx;
	ct->v0 = dy;
	ct->a[1][0] = 1.0;
	ct->b[0][1] = 1.0;
	return 0;
}

// create a shift-scale-rotate ctrans
// a positive dx will cause an image to shift to the right after the transform
// scales > 1 will cause the transformed image to appear larger
// rot is in radians!

int make_roto_translate(struct ctrans *ct, double dx, double dy, double xs, double ys, double rot)
{
	int i,j;
	double sa, ca;

	ct->order = 1;

	for (i = 0; i < MAX_ORDER; i++)
		for (j = 0; j < MAX_ORDER; j++) {
			ct->a[i][j] = 0.0;
			ct->b[i][j] = 0.0;
		}
	ct->u0 = dx;
	ct->v0 = dy;
	sa = sin(-rot);
	ca = cos(-rot);
	ct->a[1][0] = 1.0 / xs * ca;
	ct->a[0][1] = - 1.0 / xs * sa;
	ct->b[0][1] = 1.0 / ys * ca;
	ct->b[1][0] = 1.0 / ys * sa;
	return 0;
}

// compute the coordinate transform
static void do_ctrans(struct ctrans *ct, double u, double v, double *x, double *y)
{
	double up[MAX_ORDER+1];
	double vp[MAX_ORDER+1];
	int k,i;

	u -= ct->u0;
	v -= ct->v0;
	*x = ct->a[0][0] + u * ct->a[1][0] + v * ct->a[0][1];
	*y = ct->b[0][0] + u * ct->b[1][0] + v * ct->b[0][1];
	if (ct->order <= 1)
		return;

// tabulate the powers
	up[0] = 1;
	vp[0] = 1;
	for (i = 1; i <= ct->order; i++) {
		up[i] = u * up[i-1];
		vp[i] = v * vp[i-1];
	}
// now do the calculations of the higher-order terms
	for (k = 2; k <= ct->order; k++) {
		for (i=0; i<=k; i++) {
			*x += up[i]*vp[k-i]*ct->a[i][k-i];
			*y += up[i]*vp[k-i]*ct->b[i][k-i];
		}
	}
}

// do linear shear transform in the x direction
// x = c ( u - a * v )
// y = v

int linear_x_shear(struct ccd_frame *in, struct ccd_frame *out, double a, double c)
{
	int wi, hi, wo, ho; // frame geometries
	float *dpi, *dpo; // frame pointers
	int plane_iter = 0;

	double dwi; // double input frame w/h to avoid repeated transforms

	double xl; // output pixel size in input units
	double xl0; // pixel slice factors

	int ui, vi; //coordinates in the output frame
	double x; //coordinates in the input frame
	double ox;
	double v;

	float filler; // filler value for out-of-frame spots

//	d3_printf("x_shear a=%.1f c=%.1f\n", a, c);

// handy frame geometries
	wi = in->w;
	hi = in->h;
	wo = out->w;
	ho = out->h;
	dwi = 1.0 * wi;

	if (!in->stats.statsok)
		frame_stats(in);
	filler = in->stats.cavg;

//	d3_printf("avg = %.1f\n", in->stats.avg);

	if (c < 0) {
		err_printf("linear_x_shear: frame needs 'x' flipping\n");
		return -1;
	}

//	d3_printf("wi:%d, wo:%d, ho:%d, hi:%d, filler:%.1f\n", wi, wo, ho, hi, filler);

	while ((plane_iter = color_plane_iter(in, plane_iter))) {
		for (vi = 0; vi < ho; vi++) { //output lines
			dpo = get_color_plane(out, plane_iter);
			dpi = get_color_plane(in, plane_iter);
			dpo += wo * vi; // line origins
			dpi += wi * vi;
			ox = - a * c * 1.0 * vi; // x of pixel with u = 0;
			x = ox - c;
			xl = c;
//			read = 0;
//			d3_printf("vi=%d ", vi);
			for (ui = 0; ui < wo; ui++) { //output pixels
				xl = c;
				x += xl;
				if ((x <= 0) || (x > dwi - xl) || (vi >= hi)) { // we are outside the input frame
					*dpo++ = filler;
					continue;
 				}
				xl0 = ceil(x) - x;
				if (xl0 > xl) { // it's all within one input pixel
					*dpo++ = *dpi * xl;
					continue;
				}
				v = (*dpi++) * xl0; // get the first slice
//				read ++;
				xl -= xl0;
				while (xl > 1.0) { // add the whole pixels
					v += (*dpi++);
//					read ++;
					xl -= 1.0;
				}
				v += (*dpi) * xl; // add the last bit
				*dpo ++ = v;
			}
//			d3_printf("read %d vi=%d\n", read, vi);
		}
	}
//	d3_printf("x_shear: exiting\n");
	return 0;
}

// do linear shear transform in the y direction
// x = u - u0
// y = c (v - v0 + b * (u - u0))
int linear_y_shear(struct ccd_frame *in, struct ccd_frame *out, double b, double c,
		   double u0, double v0)
{
	int wi, wo, ho; // frame geometries
	float *dpi, *dpo; // frame pointers
	int plane_iter = 0;

	double dhi; // double input frame w/h to avoid repeated transforms

	double yl; // output pixel size in input units
	double yl0;

	int ui, uip, vi; //coordinates in the output frame
	double y; //coordinates in the input frame
	double oy;
	double v;

	float filler; // filler value for out-of-frame spots

// handy frame geometries
	wi = in->w;
	wo = out->w;
	ho = out->h;
	dhi = 1.0 * in->h;

	if (!in->stats.statsok)
		frame_stats(in);
	filler = in->stats.cavg;

	if (c < 0) {
		err_printf("linear_y_shear: frame needs 'y' flipping\n");
		return -1;
	}

	while ((plane_iter = color_plane_iter(in, plane_iter))) {
		for (ui = 0; ui < wo; ui++) { //output columns
			uip = (int)floor(ui - u0);
			dpo = get_color_plane(out, plane_iter);
			dpi = get_color_plane(in, plane_iter);
			dpo += uip; // line origins
			dpi += uip;
			oy = c * (b * (1.0 * ui - u0) - v0); // y of pixel with v = 0;
			y = oy - c;
			for (vi = 0; vi < ho; vi++) { //output pixels
				yl = c;
				y += yl;
				if (y <= 0 || y > dhi - yl || ui >= wi) { // we are outside the input frame
					*dpo = filler;
					dpo += wo;
					continue;
	 			}
				yl0 = ceil(y) - y;
				if (yl0 > yl) { // it's all within one input pixel
					*dpo = *dpi * yl;
					dpo += wo;
					continue;
				}
				v = (*dpi) * yl0; // get the first slice
				dpi += wi;
				yl -= yl0;
				while (yl > 1.0) { // add the whole pixels
					v += (*dpi);
					dpi += wi;
					yl -= 1.0;
				}
				v += (*dpi) * yl; // add the last bit
				*dpo = v;
				dpo += wo;
			}
		}
	}
	return 0;
}


// change frame coordinates
int warp_frame(struct ccd_frame *in, struct ccd_frame *out, struct ctrans *ct)
{
	return -1;
}


// fast shift-only functions

// shift frame left
static void shift_right(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *sp, *dp;
	for (y = 0; y < h; y++) {
		dp = dat + y * w + w - 1;
		sp = dat + y * w - dn + w - 1;
		for (x = 0; x < w - dn - 1; x++) {
			*dp = *sp * b + *(sp-1) * a;
			dp--;
			sp--;
		}
		*dp-- = *sp-- * b;
		for (x = 0; x < dn; x++)
			*dp-- = 0.0;
	}
}

// shift frame left
static void shift_left(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *sp, *dp;
	for (y = 0; y < h; y++) {
		dp = dat + w * y;
		sp = dat + w * y + dn;
		for (x = 0; x < w - dn - 1; x++) {
			*dp = *sp * b + *(sp+1) * a;
			dp++;
			sp++;
		}
		*dp++ = *sp * b;
		for (x = 0; x < dn; x++)
			*dp++ = 0.0;
	}
}

// shift frame up
static void shift_up(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *dp, *sp1, *sp2;
	dp = dat;
	sp1 = dat + w * (dn);
	sp2 = dat + w * (dn + 1);
	for (y = 0; y < h - dn - 1; y++) {
		for (x = 0; x < w; x++) {
			*dp = *sp1 * b + * sp2 * a;
			dp++; sp1++; sp2++;
		}
	}
	for (x = 0; x < w; x++) {
		*dp = *sp1 * b;
		dp++; sp1++;
	}
	for (y = 0; y < dn; y++)
		for (x = 0; x < w; x++)
			*dp-- = 0.0;
}

// shift frame down
static void shift_down(float *dat, int w, int h, int dn, double a, double b)
{
	int x, y;
	float *dp, *sp1, *sp2;

//	d3_printf("down\n");

	dp = dat + w * h - 1;
	sp1 = dp - w * dn;
	sp2 = dp - w * (dn + 1);
	for (y = 0; y < h - dn - 1; y++) {
		for (x = 0; x < w; x++) {
			*dp = *sp1 * b + * sp2 * a;
			dp--; sp1--; sp2--;
		}
	}
	for (x = 0; x < w; x++) {
		*dp = *sp1 * b;
		dp--; sp1--;
	}
	for (y = 0; y < dn; y++)
		for (x = 0; x < w; x++)
			*dp-- = 0.0;

}
// shift_frame rebins a frame in-place maintaining it's orientation and scale
// frame size is maintained; new pixels are 0-filled

int shift_frame(struct ccd_frame *fr, double dx, double dy)
{
	double a, b;
	int dn;
	int h, w;
	float *dat;
	int plane_iter = 0;

	w = fr->w;
	h = fr->h;

	while ((plane_iter = color_plane_iter(fr, plane_iter))) {
		dat = get_color_plane(fr, plane_iter);
		if (dx > 0) { // shift right
			a = dx - floor(dx);
			b = 1 - a;
			dn = floor(dx);
			shift_right(dat, w, h, dn, a, b);
		} else if (dx < 0) { // shift left
			a = -dx - floor(-dx);
			b = 1 - a;
			dn = floor(-dx);
			shift_left(dat, w, h, dn, a, b);
		}

		if (dy < 0.0) { // shift up
			a = -dy - floor(-dy);
			b = 1 - a;
			dn = floor(-dy);
			shift_up(dat, w, h, dn, a, b);

		} else if (dy > 0.0) {
			a = dy - floor(dy);
			b = 1 - a;
			dn = floor(dy);
			shift_down(dat, w, h, dn, a, b);
		}
	}

	return 0;
}


/*
static void coord_transform(double dx, double dy, double ds, double dt, double x, double y, double *xx, double *yy)
{
	*xx = dx + (x * ds - dx) * cos(dt) + (y * ds - dy) * sin(dt);
	*yy = dy - (x * ds - dx) * sin(dt) + (y * ds - dy) * cos(dt);
}
*/

#if 0
	ct->u0 = dx;
	ct->v0 = dy;
	sa = sin(-rot);
	ca = cos(-rot);
	ct->a[1][0] = 1.0 / xs * ca;
	ct->a[0][1] = - 1.0 / xs * sa;
	ct->b[0][1] = 1.0 / ys * ca;
	ct->b[1][0] = 1.0 / ys * sa;
	return 0;
#endif

struct cmatrix {
	double d[3][3];
	double i[3][3];
};

void make_cmatrix(struct cmatrix *mat, double dx, double dy, double ds, double dt)
{
	int i, j;
	double st, ct;
	double det;

	st = sin(dt);
	ct = cos(dt);

	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			mat->d[i][j] = (i == j) ? 1.0 : 0.0;

	mat->d[0][2] = dx;
	mat->d[1][2] = dy;

	mat->d[0][0] = ct * ds;
	mat->d[0][1] = - st * ds;
	mat->d[1][0] = st * ds;
	mat->d[1][1] = ct * ds;

	det = mat->d[0][0] * mat->d[1][1] - mat->d[0][1] * mat->d[1][0];
	if (fabs(det) < 10e-30) {
		err_printf("singular matrix\n");
		return;
	}

	mat->i[0][0] = 1.0 / det * mat->d[1][1];
	mat->i[0][1] = - 1.0 / det * mat->d[0][1];
	mat->i[0][2] = 1.0 / det * (mat->d[0][1] * mat->d[1][2] - mat->d[0][2] * mat->d[1][1]);

	mat->i[1][0] = - 1.0 / det * mat->d[1][0];
	mat->i[1][1] = 1.0 / det * mat->d[0][0];
	mat->i[1][2] = 1.0 / det * (mat->d[0][2] * mat->d[1][0] - mat->d[0][0] * mat->d[1][2]);

	mat->i[2][0] = 0.0;
	mat->i[2][1] = 0.0;
	mat->i[2][2] = 1.0 / det * (mat->d[0][0] * mat->d[1][1] - mat->d[0][1] * mat->d[1][0]);

}

void do_cmatrix_d(struct cmatrix *mat, double u, double v, double *x, double *y)
{
	*x = mat->d[0][0] * u + mat->d[0][1] * v + mat->d[0][2];
	*y = mat->d[1][0] * u + mat->d[1][1] * v + mat->d[1][2];
}

static inline void do_cmatrix_i(struct cmatrix *mat, double u, double v, double *x, double *y)
{
	*x = mat->i[0][0] * u + mat->i[0][1] * v + mat->i[0][2];
	*y = mat->i[1][0] * u + mat->i[1][1] * v + mat->i[1][2];
}


static inline double cubic_weight(double x, double a, double b)
{
	double v = fabs(x);
	double q = 0.0;

	if (v < 1) {
		q = (-6*a - 9*b + 12) * v * v *v +
		    (6*a + 12*b - 18) * v * v -
		    2*b + 6;
	} else if (v < 2) {
		q = (-6*a - b) * v * v * v +
		    (30*a + 6*b) * v * v +
		    (-48*a - 12*b) * v +
		    24*a + 8*b;
	}

	return 1.0 / 6.0 * q;
}

static inline double interpolate_pixel_cubic(float *data, int w, int h, double x, double y, double a, double b)
{
	int x0, y0;
	double p, q;
	int i, j, u, v;

	if (x < 3 || x >= w - 3 || y < 3 || y >= h - 3)
		return 0.0;

	x0 = (int) floor(x);
	y0 = (int) floor(y);

	// n = extent of kernel
	// q = 0
	// for j = 0 ... 2n-1 do // iterate over 2n line
	//     let v <- vj = |y0| + j - n + 1
	//     let p <- 0
	//     for i = 0 ... 2n-1 do // iterate over 2n cols
	//         let u <- ui = |x0| + i - n +1
	//         let p <- p + I(u,v) * w(x0-u)
	//     done
	//     let q = q + p * w(y0-v)
	// done

	q = 0.0;
	for (j = 0; j < 4; j++) {
		p = 0.0;
		v = y0 + j - 1;
		for (i = 0; i < 4; i++) {
			u = x0 + i - 1;
			p += data[v * w + u] * cubic_weight(x - (double) u, a, b);
		}

		q += p * cubic_weight(y - (double) v, a, b);
	}


	return q;
}

int shift_scale_rotate_frame(struct ccd_frame *fr, double dx, double dy, double ds, double dt, int resampling)
{
	struct cmatrix cm;
	int i, j;
	int xx, yy;
	double x, y;
	double a = 0.0, b = 0.0, c, d;
	int cubic = 1;
	int index = 0;
	float *ddat = NULL, *sdat = NULL;
	int plane_iter;

	make_cmatrix(&cm, dx, dy, ds, dt);

	struct ccd_frame *nfr = new_frame_fr(fr, fr->w, fr->h);

	switch (resampling) {

	case PAR_RESAMPLE_NEAREST:
		for (i = 0; i < nfr->h; i++) {
			for (j = 0; j < nfr->w; j++) {
				do_cmatrix_i(&cm, j, i, &x, &y);

				yy = (int) floor(y + 0.5);
				xx = (int) floor(x + 0.5);

				if (yy >= 0 && yy < fr->h && xx >= 0 && xx < fr->w) {
					plane_iter = 0;
					while ((plane_iter = color_plane_iter(fr, plane_iter))) {
						sdat = get_color_plane(fr, plane_iter);
						ddat = get_color_plane(nfr, plane_iter);

						ddat[index] = sdat[yy*fr->w + xx];
					}
					index++;

				} else {
					index++;
				}
			}
		}
		cubic = 0;
		break;

	case PAR_RESAMPLE_BILINEAR:
		for (i = 0; i < nfr->h; i++) {
			for (j = 0; j < nfr->w; j++) {
				do_cmatrix_i(&cm, j, i, &x, &y);

				if (x < 1 || x > fr->w - 1 || y < 1 || y > fr->h - 1)
					index++;
				else {
					a = x - floor(x);
					b = 1 - a;
					c = y - floor(y);
					d = 1 - c;

					int xx = (int) floor(x);
					int yy = (int) floor(y);

					plane_iter = 0;
					while ((plane_iter = color_plane_iter(fr, plane_iter))) {
						sdat = get_color_plane(fr, plane_iter);
						ddat = get_color_plane(nfr, plane_iter);


						ddat[index] = b * d * sdat[yy * fr->w + xx] +
							      a * d * sdat[yy * fr->w + xx + 1] +
							      b * c * sdat[(yy + 1) * fr->w + xx] +
							      a * c * sdat[(yy + 1) * fr->w + xx + 1];
					}
					index++;
				}
			}
		}
		cubic = 0;
		break;

	case PAR_RESAMPLE_BSPLINE:	     /* Cubic B-spline */
		a = 1.0; b = 0.0;
		break;

	case PAR_RESAMPLE_CATMULL:	     /* Catmull-Rom */
		a = 0.0; b = 0.5;
		break;

	case PAR_RESAMPLE_MITCHELL:	     /* Mitchell-Netravali */
		a = 1/3.0; b = 1/3.0;
		break;

	default:
		return -1;
	}

	if (cubic) {
		for (i = 0; i < nfr->h; i++) {
			for (j = 0; j < nfr->w; j++) {
				do_cmatrix_i(&cm, j, i, &x, &y);

				/* a = 1.0, b = 0.0     -> Cubic B-spline
				   a = 0.0, b = 0.5     -> Catmull-Rom
				   a = 1/3.0, b = 1/3.0 -> Mitchell-Netravali
				*/
				plane_iter = 0;
				while ((plane_iter = color_plane_iter(fr, plane_iter))) {
					sdat = get_color_plane(fr, plane_iter);
					ddat = get_color_plane(nfr, plane_iter);

					ddat[index] = interpolate_pixel_cubic(sdat, fr->w, fr->h, x, y, a, b);
				}
				index++;
			}
		}
	}

	fr->stats.statsok = 0;

	free(fr->dat);  fr->dat  = nfr->dat;  nfr->dat = NULL;
	free(fr->rdat); fr->rdat = nfr->rdat; nfr->rdat = NULL;
	free(fr->gdat); fr->gdat = nfr->gdat; nfr->gdat = NULL;
	free(fr->bdat); fr->bdat = nfr->bdat; nfr->bdat = NULL;

	release_frame(nfr);

	return 0;

}


// create a gaussian filter kernel of given sigma
// requires a prealloced table of floats of suitable size (size*size)
// only odd-sized kernels are produced
// returns 0 for success, -1 for error

int make_gaussian(float sigma, int size, float *kern)
{
	int mid, all;
	float *mp;
	float sum, v;
	int x, y;

	if (sigma < 0.01) {
		err_printf("make_gaussian: clipping sigma to 0.01\n");
		sigma = 0.01;
	}

	if (size % 2 != 1 || size < 3 || size > 127) {
		err_printf("make_gaussian: bad size %d\n");
		return -1;
	}
	mid = size / 2;
	mp = kern + mid * size + mid;
	sum = 0;
	for (y = 0; y < mid + 1; y++)
		for(x = 0; x < mid + 1; x++) {
			v = exp(- sqrt(1.0 * (sqr(x) + sqr(y))) / sigma);
//			d3_printf("x%d y%d v%.2f\n", x, y, v);
			if (x == 0 && y == 0)
				sum += v;
			else if (x == 0 || y == 0)
				sum += 2 * v;
			else
				sum += 4 * v;

			*(mp + x + size * y) = v;
			*(mp + x - size * y) = v;
			*(mp - x - size * y) = v;
			*(mp - x + size * y) = v;
		}
	all = size * size;
	for (y = 0; y < all; y++)
		kern[y] /= sum;
	return 0;
}
