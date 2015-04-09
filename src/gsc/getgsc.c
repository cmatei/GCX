#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern int getgsc(float ra, float dec, float radius, float maxmag,
		  int *regs, int *ids, float *ras, float *decs, float *mags,
		  int n, char *catpath);

extern int xypix(double xpos, double ypos, double xref, double yref, double xrefpix,
                 double yrefpix, double xinc, double yinc, double rot,
                 char *type, double *xpix, double *ypix);

extern int worldpos(double xpos, double ypos, double xref, double yref, double xrefpix,
                 double yrefpix, double xinc, double yinc, double rot,
                 char *type, double *xpix, double *ypix);

/* aips-style (cdelt + rotation) */
static void XE_to_xy_aips(double xrefpix, double yrefpix,
                          double xinc, double yinc, double rot,
                          double X, double E, double *xpix, double *ypix)
{
        double dx = X;
        double dy = E;
        double dz, cosr, sinr;
        double r, cond2r = 1.745329252e-2;

        r = rot * cond2r;
        cosr = cos (r);
        sinr = sin (r);
/*  Correct for rotation */
        dz = dx*cosr + dy*sinr;
        dy = dy*cosr - dx*sinr;
        dx = dz;
/*     convert to pixels  */
        *xpix = dx / xinc + xrefpix;
        *ypix = dy / yinc + yrefpix;
}


static double width = 3948.0, height = 2622.0;

int main(int argc, char **argv)
{
	  float ras[1000], decs[1000], mags[1000];
	  int regs[1000], ids[1000];
	  int i, nn;
	  double X, E, x, y;

	  // 295.692209116, 23.1605880976
	  nn = getgsc(295.69, 23.16, 30.0, 12.0,
		      regs, ids, ras, decs, mags,
		      1000, NULL);

	  printf("# X Y MAG\n");
	  for (i = 0; i < nn; i++) {


		  xypix((float) ras[i], (float) decs[i], width / 2.0, height / 2.0, 0.0, 0.0, 1.0, 1.0, 0.0,
			"-TAN", &X, &E);

		  XE_to_xy_aips(width / 2.0, height / 2.0, 1.0, 1.0, 0.0, X, E, &x, &y);

#if 0
		  XE_to_xy_aips(wcs->xrefpix, wcs->yrefpix,
				wcs->xinc, wcs->yinc, wcs->rot, X, E, x, y);
#endif

		  // convert ra/dec to xy coords
		  printf("%.2f %.2f %.2f\n", x, y, mags[i]);
	  }

	  return 0;
}
