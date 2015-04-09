#ifndef __GCX_H
#define __GCX_H

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _Gcx      Gcx;
typedef struct _GcxClass GcxClass;

#define GCX_TYPE_GCX            (gcx_get_type())
#define GCX(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCX_TYPE_GCX, Gcx))
#define GCX_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass),  GCX_TYPE_GCX, GcxClass))
#define GCX_IS_GCX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCX_TYPE_GCX))
#define GCX_IS_GCX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GCX_TYPE_GCX))


GType gcx_get_type (void);

Gcx * gcx_new();

GtkWidget * gcx_get_image_window (Gcx *gcx);

int gcx_save_rcfile (Gcx *gcx);
int gcx_load_rcfile (Gcx *gcx, char *filename);

/* from gcx.c */

int save_params_rc(void);
int load_params_rc(void);


G_END_DECLS


/* FIXME: remove when the dust settles */

#include "ccd.h"

#ifndef PI
#define PI 3.141592653589793
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#else
#  define VERSION "0.3.1"
#endif

/* malloc debugger - to enable, uncoment the AC_CKECK_LIB line in
 * configure.in */
#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
//#define USE_DMALLOC
#endif


#define JD2000 2451545.0
#define	hrdeg(x)	((x)*15.)
#define	deghr(x)	((x)/15.)
#define	hrrad(x)	degrad(hrdeg(x))
#define	radhr(x)	deghr(raddeg(x))
#define degrad(x) ((x) * PI / 180)
#define raddeg(x) ((x) * 180 / PI)


// current circumstances of observation
#define OBSCOLS 68

struct obsdata {
	double flen;		// focal length (mm)
	double aperture;	// telescope aperture (cm)
	char telescop[OBSCOLS+1];	// telescope name
	char focus[OBSCOLS+1];	// focus designation
	char filter[OBSCOLS+1];	// filter used
	char object[OBSCOLS+1];	// object targeted
	double ra;		// ra of center of field (degrees)
	double dec;		// dec of center of field
	double rot;		// field rotation (N through E)
	double altitude;	// altitude of target
	int equinox;		// equinox of ra/dec
	double mag;		// magnitude from catalog
};

extern struct obsdata obs;

// max number of frames that we combine in one operation
#define COMB_MAX 256

// stacking methods
#define STACK_MMEDIAN 0
#define STACK_MEDIAN 1
#define STACK_AVERAGE 3
#define STACK_MINMAXREJ 4
#define STACK_AVGSIGCLIP 5


/* clamp functions */
static inline int clamp_double(double *val, double min, double max)
{
	if (*val < min) {
		*val = min;
		return 1;
	}
	if (*val > max) {
		*val = max;
		return 1;
	}
	return 0;
}

static inline int clamp_float(float *val, float min, float max)
{
	if (*val < min) {
		*val = min;
		return 1;
	}
	if (*val > max) {
		*val = max;
		return 1;
	}

	return 0;
}

static inline int clamp_int(int *val, int min, int max)
{
	if (*val < min) {
		*val = min;
		return 1;
	}

	if (*val > max) {
		*val = max;
		return 1;
	}
	return 0;
}



#endif
