#ifndef _WCS_H_
#define _WCS_H_


/* a pair to be fitted */
struct fit_pair {
	double x;		/* extracted pixel coordinates of star */
	double y;
	double X;		/* projection plane coordinates (from x,y) */
	double E;
	double poserr;		/* estimated error of extracted coords (degrees) */
	double X0;		/* catalog projection plane coordinates (xi, eta) */
	double E0;
	double caterr;		/* error of catalog coordinates (degrees) */
	struct cat_star *cats;	/* the star itself */
};

/* function prototypes */
void wcs_from_frame(struct ccd_frame *fr, struct wcs *wcs);
struct wcs *wcs_new(void);
void wcs_ref(struct wcs *wcs);
void wcs_release(struct wcs *wcs);
int window_fit_wcs(GtkWidget *window);
int w_worldpos(struct wcs *wcs, double xpix, double ypix, double *xpos, double *ypos);
int w_xypix(struct wcs *wcs, double xpos, double ypos, double *xpix, double *ypix);
void cat_change_wcs(GSList *sl, struct wcs *wcs);
int auto_pairs(struct gui_star_list *gsl);
int pairs_cs_diff(GSList *pairs, double *dxo, double *dyo, 
		  double *dso, double *dthetao, int scale_en, int rot_en);
int fastmatch(GSList *field, GSList *cat);
void pairs_fit_errxy(GSList *pairs, struct wcs *wcs, double *ra_err, double *de_err);
void cats_xypix (struct wcs *wcs, struct cat_star *cats, double *xpix, double *ypix);

/* from plate.c */

void XE_to_xy(struct wcs *wcs, double X, double E, double *x, double *y);
void xy_to_XE(struct wcs *wcs, double x, double y, double *X, double *E);
int plate_solve(struct wcs *wcs, struct fit_pair fpairs[], int n, int model);



#endif
