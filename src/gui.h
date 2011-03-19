#ifndef _GUI_H_
#define _GUI_H_

#include <gtk/gtk.h>
#include "ccd/ccd.h"

/* action values for cuts callback */
#define CUTS_AUTO 0x100
#define CUTS_MINMAX 0x200
#define CUTS_FLATTER 0x300
#define CUTS_SHARPER 0x400
#define CUTS_BRIGHTER 0x500
#define CUTS_DARKER 0x600
#define CUTS_CONTRAST 0x700
#define CUTS_INVERT 0x800
#define CUTS_VAL_MASK 0x000000ff
#define CUTS_ACT_MASK 0x0000ff00

/* action values for view (zoom/pan) callback */
#define VIEW_ZOOM_IN 0x100
#define VIEW_ZOOM_OUT 0x200
#define VIEW_ZOOM_FIT 0x300
#define VIEW_PIXELS 0x400
#define VIEW_PAN_CENTER 0x500
#define VIEW_PAN_CURSOR 0x600

#define MAX_ZOOM 16

/* selection modes */
#define SELECTION_NORMAL 0
#define SELECTION_MARK_STARS 1
#define SELECTION_PAIR 2

/* action values for selection mode callback */
#define SEL_ACTION_NORMAL 1
#define SEL_ACTION_MARK_STARS 2
#define SEL_ACTION_PAIR 3

/* star popup action codes */
#define STARP_UNMARK_STAR 1
#define STARP_REMOVE_SEL 2
#define STARP_INFO 3
#define STARP_PAIR 4
#define STARP_PAIR_RM 5
#define STARP_MAKE_CAT 6
#define STARP_MAKE_STD 7
#define STARP_EDIT_AP 8
#define STARP_GROWTH 9
#define STARP_PROFILE 10
#define STARP_MEASURE 11
#define STARP_SKYHIST 12
#define STARP_MOVE 13
#define STARP_FIT_PSF 14

/* add star action codes */
#define ADD_STARS_DETECT 1
#define ADD_STARS_GSC 2
#define ADD_STARS_OBJECT 3
#define ADD_STARS_TYCHO2 4
#define ADD_FROM_CATALOG 5

/* remove stars actions */
#define STAR_RM_ALL 1
#define STAR_RM_FR 2
#define STAR_RM_USER 3
#define STAR_RM_CAT 4
#define STAR_RM_SEL 5
#define STAR_RM_PAIRS_ALL 6
#define STAR_RM_PAIRS_SEL 7
#define STAR_RM_FIELD 8
#define STAR_RM_OFF 10

/* Pairs actions */
#define PAIRS_AUTO 1

/* wcs actions */
#define WCS_FIT 1
#define WCS_AUTO 2
#define WCS_RELOAD 3
#define WCS_FORCE_VALID 4
#define WCS_QUIET_AUTO 5
#define WCS_RESET 6
#define WCS_EXISTING 7

/* file actions */
#define FILE_OPEN 1
#define FILE_CLOSE 2
#define FILE_SAVE_AS 3
#define FILE_EXPORT_PNM8 4
#define FILE_EXPORT_PNM16 5
#define FILE_FITS_HEADER 6
#define FILE_OPEN_RCP 7
#define FILE_LOAD_GSC2 8

#define FILE_ADD_TO_MBAND 100


/* photometry actions */
#define PHOT_CENTER_STARS 1
#define PHOT_RUN 2
#define PHOT_ACTION_MASK 0xff
#define PHOT_CENTER_PLOT 3

#define PHOT_TO_STDOUT 0x400
#define PHOT_TO_STDOUT_AA 0x500
#define PHOT_TO_FILE 0x100
#define PHOT_TO_FILE_AA 0x600
#define PHOT_TO_MBDS 0x200
#define PHOT_OUTPUT_MASK 0xff00

/* help actions */
#define HELP_BINDINGS 1
#define HELP_USAGE 2
#define HELP_OBSCRIPT 3
#define HELP_REPCONV 4

/* file switch actions */

#define SWF_NEXT 1
#define SWF_SKIP 2
#define SWF_PREV 3
#define SWF_QPHOT 4
#define SWF_RED 5

/* star display */
#define STAR_BRIGHTER 1
#define STAR_FAINTER 2
#define STAR_REDRAW 3

/* star edit */
#define STAR_EDIT 1

/* this is how we identify a status message
 * (we need this essentially to be able to pass
 * it to a timer funtion for deleting */
struct status_ref
{
	gpointer *statusbar;
	guint context_id;
	guint msg_id;
};

/* definitions of channels and other stuff associated with image displaying */

#define LUT_SIZE 4096
#define LUT_IDX_MASK 0x0fff

/* LUT modes */
#define LUT_MODE_DIRECT 1 /* for 8-bit frames, and
			     16-bit frames we know have values < LUT_SIZE
			     output = lut[input], cuts ignored */
#define LUT_MODE_FULL 0   /* output = lut[(input - lcut)/(hcut - lcut) * LUT_SIZE] */

/* this structure describes a channel (a frame and it's associated intesity mapping) */
struct image_channel {
	int ref_count; /* reference count for map */
	double lcut; /* the low cut */
	double hcut; /* the high cut */
	int invert; /* if 1, the image is displayed in reverse video */
	double avg_at; /* position of average between cuts */
	double gamma; /* gamma setting for image */
	double toe; /* toe setting for image */
	double offset; /* toe setting for image */
	unsigned short lut[LUT_SIZE];
	int lut_mode; /* the way the lut is set up */
	double dsigma; /* sigma used for cut calculation */
	double davg; /* image average used for display calculations */
	int flip_h; /* flag for horisontal flip */
	int flip_v; /* flag for horisontal flip */
	int zoom_mode; /* zooming algorithm */
	int channel_changed; /* when anyhting is changed in the map, setting this */
			 /* flag will ask for the map cache to be redrawn */
	struct ccd_frame *fr; /* the actual image of the channel */
	int color;		/* display a color image */
};

/* we keep a cache of the already trasformed image for quick expose
 * redraws.
 */
#define MAP_CACHE_GRAY 0
#define MAP_CACHE_RGB 1
struct map_cache {
	int ref_count; /* reference count for cache */
	int cache_valid; /* the cache is valid */
	int type; /* type of cache: gray or rgb */
	double zoom; /* zoom level of the cache */
	int x; /* coordinate of top-left corner of cache (in display space) */
	int y;
	int w; /* width of cache (in display pixels) */
	int h; /* height of cache (in display pixels) */
	unsigned size; /* size of cache (in bytes) */
	unsigned char *dat; /* pointer to cache data area */
};

/* per-window image display parameters */
struct map_geometry {
	int ref_count;
	double zoom;	/* zoom level for frame mapping */
	int width;   	/* size of drawing area at zoom=1 */
	int height;
};


/* function prototypes */
/* from showimage.c */
extern gboolean image_expose_cb(GtkWidget *widget, GdkEventExpose *event, gpointer data);
extern int frame_to_channel(struct ccd_frame *fr, GtkWidget *window, char *chname);
void ref_image_channel(struct image_channel *channel);
void release_image_channel(struct image_channel *channel);
int channel_to_pnm_file(struct image_channel *channel, GtkWidget *window, char *fn, int is_16bit);
void cam_to_img(GtkWidget *dialog);
struct map_cache *new_map_cache(int size, int type);
void release_map_cache(struct map_cache *cache);
void paint_from_gray_cache(GtkWidget *widget, struct map_cache *cache, GdkRectangle *area);
void image_box_to_cache(struct map_cache *cache, struct image_channel *channel,
			double zoom, int x, int y, int w, int h);




/* from gui.c */
extern GtkWidget * create_image_window(void);
void error_beep(void);
void warning_beep(void);
int err_printf_sb2(gpointer window, const char *fmt, ...);
int info_printf_sb2(gpointer window, const char *fmt, ...);
int modal_yes_no(char *text, char *title);
int modal_entry_prompt(char *text, char *title, char *initial, char **value);
int window_auto_pairs(gpointer window);

void star_pairs_action(GtkAction *action, gpointer window);

void star_rm_cb(gpointer data, guint action, GtkWidget *menu_item);
void stars_rm_selected_action(GtkAction *action, gpointer data);
void stars_rm_detected_action(GtkAction *action, gpointer data);
void stars_rm_user_action(GtkAction *action, gpointer data);
void stars_rm_field_action(GtkAction *action, gpointer data);
void stars_rm_cat_action(GtkAction *action, gpointer data);
void stars_rm_off_action(GtkAction *action, gpointer data);
void stars_rm_all_action(GtkAction *action, gpointer data);
void stars_rm_pairs_all_action(GtkAction *action, gpointer data);
void stars_rm_pairs_sel_action(GtkAction *action, gpointer data);

/* from imadjust.c */
extern void set_default_channel_cuts(struct image_channel* channel);
extern void set_darea_size(GtkWidget *window, struct map_geometry *geom, double xc, double yc);
extern void drag_adjust_cuts(GtkWidget *window, int dx, int dy);
extern void pan_cursor(GtkWidget *window);
extern void stats_cb(gpointer data, guint action, GtkWidget *menu_item);
extern void show_region_stats(GtkWidget *window, double x, double y);
extern void show_zoom_cuts(GtkWidget * window);

extern void cuts_option_cb(gpointer data, guint action, GtkWidget *menu_item);

extern void cuts_auto_action (GtkAction *action, gpointer data);
extern void cuts_minmax_action (GtkAction *action, gpointer data);
extern void cuts_flatter_action (GtkAction *action, gpointer data);
extern void cuts_sharper_action (GtkAction *action, gpointer data);
extern void cuts_brighter_action (GtkAction *action, gpointer data);
extern void cuts_darker_action (GtkAction *action, gpointer data);
extern void cuts_contrast_1_action (GtkAction *action, gpointer data);
extern void cuts_contrast_2_action (GtkAction *action, gpointer data);
extern void cuts_contrast_3_action (GtkAction *action, gpointer data);
extern void cuts_contrast_4_action (GtkAction *action, gpointer data);
extern void cuts_contrast_5_action (GtkAction *action, gpointer data);
extern void cuts_contrast_6_action (GtkAction *action, gpointer data);
extern void cuts_contrast_7_action (GtkAction *action, gpointer data);
extern void cuts_contrast_8_action (GtkAction *action, gpointer data);

extern void view_option_cb(gpointer data, guint action, GtkWidget *menu_item);

extern void view_zoom_in_action (GtkAction *action, gpointer data);
extern void view_zoom_out_action (GtkAction *action, gpointer data);
extern void view_pixels_action (GtkAction *action, gpointer data);
extern void view_pan_center_action (GtkAction *action, gpointer data);
extern void view_pan_cursor_action (GtkAction *action, gpointer data);


void histogram_cb(gpointer data, guint action, GtkWidget *menu_item);
extern void histogram_action (GtkAction *action, gpointer data);

/* paramsgui.c */
void edit_options_action(GtkAction *action, gpointer window);

/* staredit.c */

void star_edit_dialog(GtkWidget *window, GSList *found);
void star_edit_star(GtkWidget *window, struct cat_star *cats);
void update_dynamic_string(char **dest, char *src);
void do_edit_star(GtkWidget *window, GSList *found, int make_std);
void add_star_from_catalog(gpointer window);

/* textgui.c */
void fits_header_action(GtkAction *action, gpointer window);
void help_bindings_action(GtkAction *action, gpointer window);
void help_usage_action(GtkAction *action, gpointer window);
void help_obscript_action(GtkAction *action, gpointer window);
void help_repconv_action(GtkAction *action, gpointer window);

/* photometry.c */
char * phot_to_fd(gpointer window, FILE *fd, int format);

void phot_center_stars_action(GtkAction *action, gpointer window);
void phot_quick_action(GtkAction *action, gpointer window);
void phot_multi_frame_action(GtkAction *action, gpointer window);
void phot_to_file_action(GtkAction *action, gpointer window);
void phot_to_aavso_action(GtkAction *action, gpointer window);
void phot_to_stdout_action(GtkAction *action, gpointer window);


/* wcsedit.c */
void wcsedit_refresh(gpointer window);
int match_field_in_window_quiet(void * image_window);
int match_field_in_window(void * image_window);

void wcs_auto_action(GtkAction *action, gpointer window);
void wcs_quiet_auto_action(GtkAction *action, gpointer window);
void wcs_existing_action(GtkAction *action, gpointer window);
void wcs_fit_pairs_action(GtkAction *action, gpointer window);
void wcs_validate_action(GtkAction *action, gpointer window);
void wcs_invalidate_action(GtkAction *action, gpointer window);
void wcs_reload_action(GtkAction *action, gpointer window);
void wcs_edit_action(GtkAction *action, gpointer window);

/* recipy.c */
void create_recipe_action(GtkAction *action, gpointer window);

/* reducegui.c */
void processing_action(GtkAction *action, gpointer window);
void switch_frame_next_action(GtkAction *action, gpointer window);
void switch_frame_prev_action(GtkAction *action, gpointer window);
void switch_frame_skip_action(GtkAction *action, gpointer window);
void switch_frame_qphot_action(GtkAction *action, gpointer window);
void switch_frame_reduce_action(GtkAction *action, gpointer window);

/* guidegui.c */
void open_guide_action(GtkAction *action, gpointer window);

/* mbandgui.c */
void mband_open_action(GtkAction *action, gpointer window);
void add_to_mband(gpointer dialog, char *fn);

void mband_save_dataset_action(GtkAction *action, gpointer data);
void mband_close_dataset_action(GtkAction *action, gpointer data);
void mband_report_action(GtkAction *action, gpointer data);
void mband_close_action(GtkAction *action, gpointer data);
void mband_select_all_action (GtkAction *action, gpointer data);
void mband_unselect_all_action (GtkAction *action, gpointer data);
void mband_hide_action (GtkAction *action, gpointer data);
void mband_unhide_action (GtkAction *action, gpointer data);
void mband_fit_zpoints_action(GtkAction *action, gpointer data);
void mband_fit_zp_wtrans_action(GtkAction *action, gpointer data);
void mband_fit_trans_action(GtkAction *action, gpointer data);
void mband_fit_allsky_action(GtkAction *action, gpointer data);
void mband_plot_resmag_action(GtkAction *action, gpointer data);
void mband_plot_rescol_action(GtkAction *action, gpointer data);
void mband_plot_errmag_action(GtkAction *action, gpointer data);
void mband_plot_errcol_action(GtkAction *action, gpointer data);
void mband_plot_zpairmass_action(GtkAction *action, gpointer data);
void mband_plot_zptime_action(GtkAction *action, gpointer data);
void mband_plot_magtime_action(GtkAction *action, gpointer data);

/* synth.c */
void stars_add_synth_action(GtkAction *action, gpointer window);

/* query.c */
void cds_query_gsc_act_action(GtkAction *action, gpointer window);
void cds_query_ucac2_action(GtkAction *action, gpointer window);
void cds_query_gsc2_action(GtkAction *action, gpointer window);
void cds_query_usnob_action(GtkAction *action, gpointer window);

#endif
