#ifndef _GUI_H_
#define _GUI_H_

#include <gtk/gtk.h>
#include "ccd/ccd.h"


#define MAX_ZOOM 16


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
extern void ref_image_channel(struct image_channel *channel);
extern void release_image_channel(struct image_channel *channel);
extern int channel_to_pnm_file(struct image_channel *channel, GtkWidget *window, char *fn, int is_16bit);

extern struct map_cache *new_map_cache(int size, int type);
extern void release_map_cache(struct map_cache *cache);
extern void paint_from_gray_cache(GtkWidget *widget, struct map_cache *cache, GdkRectangle *area);
extern void image_box_to_cache(struct map_cache *cache, struct image_channel *channel,
			       double zoom, int x, int y, int w, int h);


/* from gui.c */
extern void error_beep(void);
extern void warning_beep(void);

extern int modal_yes_no(char *text, char *title);
extern int modal_entry_prompt(char *text, char *title, char *initial, char **value);

extern int err_printf_sb2(gpointer window, const char *fmt, ...);
extern int info_printf_sb2(gpointer window, const char *fmt, ...);

extern GtkWidget * create_image_window(void);
extern int window_auto_pairs(gpointer window);

extern void star_rm_cb(gpointer data, guint action);

extern void star_pairs_action(GtkAction *action, gpointer window);
extern void stars_rm_selected_action(GtkAction *action, gpointer window);
extern void stars_rm_detected_action(GtkAction *action, gpointer window);
extern void stars_rm_user_action(GtkAction *action, gpointer window);
extern void stars_rm_field_action(GtkAction *action, gpointer window);
extern void stars_rm_cat_action(GtkAction *action, gpointer window);
extern void stars_rm_off_action(GtkAction *action, gpointer window);
extern void stars_rm_all_action(GtkAction *action, gpointer window);
extern void stars_rm_pairs_all_action(GtkAction *action, gpointer window);
extern void stars_rm_pairs_sel_action(GtkAction *action, gpointer window);

/* from imadjust.c */
extern void set_default_channel_cuts(struct image_channel* channel);
extern void set_darea_size(GtkWidget *window, struct map_geometry *geom, double xc, double yc);
extern void drag_adjust_cuts(GtkWidget *window, int dx, int dy);
extern void pan_cursor(GtkWidget *window);
extern void show_region_stats(GtkWidget *window, double x, double y);
extern void show_zoom_cuts(GtkWidget * window);
extern void stats_cb(gpointer data, guint action);

extern void cuts_auto_action (GtkAction *action, gpointer window);
extern void cuts_minmax_action (GtkAction *action, gpointer window);
extern void cuts_flatter_action (GtkAction *action, gpointer window);
extern void cuts_sharper_action (GtkAction *action, gpointer window);
extern void cuts_brighter_action (GtkAction *action, gpointer window);
extern void cuts_darker_action (GtkAction *action, gpointer window);
extern void cuts_invert_action (GtkAction *action, gpointer window);
extern void cuts_contrast_1_action (GtkAction *action, gpointer window);
extern void cuts_contrast_2_action (GtkAction *action, gpointer window);
extern void cuts_contrast_3_action (GtkAction *action, gpointer window);
extern void cuts_contrast_4_action (GtkAction *action, gpointer window);
extern void cuts_contrast_5_action (GtkAction *action, gpointer window);
extern void cuts_contrast_6_action (GtkAction *action, gpointer window);
extern void cuts_contrast_7_action (GtkAction *action, gpointer window);
extern void cuts_contrast_8_action (GtkAction *action, gpointer window);

extern void view_zoom_in_action (GtkAction *action, gpointer window);
extern void view_zoom_out_action (GtkAction *action, gpointer window);
extern void view_pixels_action (GtkAction *action, gpointer window);
extern void view_pan_center_action (GtkAction *action, gpointer window);
extern void view_pan_cursor_action (GtkAction *action, gpointer window);

extern void histogram_action (GtkAction *action, gpointer window);

/* paramsgui.c */
extern void edit_options_action(GtkAction *action, gpointer window);

/* staredit.c */
extern void star_edit_dialog(GtkWidget *window, GSList *found);
extern void star_edit_star(GtkWidget *window, struct cat_star *cats);
extern void do_edit_star(GtkWidget *window, GSList *found, int make_std);
extern void add_star_from_catalog(gpointer window);

/* textgui.c */
extern void fits_header_action(GtkAction *action, gpointer window);
extern void help_bindings_action(GtkAction *action, gpointer window);
extern void help_usage_action(GtkAction *action, gpointer window);
extern void help_obscript_action(GtkAction *action, gpointer window);
extern void help_repconv_action(GtkAction *action, gpointer window);

/* photometry.c */
extern char * phot_to_fd(gpointer window, FILE *fd, int format);

extern void phot_center_stars_action(GtkAction *action, gpointer window);
extern void phot_quick_action(GtkAction *action, gpointer window);
extern void phot_multi_frame_action(GtkAction *action, gpointer window);
extern void phot_to_file_action(GtkAction *action, gpointer window);
extern void phot_to_aavso_action(GtkAction *action, gpointer window);
extern void phot_to_stdout_action(GtkAction *action, gpointer window);


/* wcsedit.c */
extern void wcsedit_refresh(gpointer window);
extern int match_field_in_window_quiet(gpointer window);
extern int match_field_in_window(gpointer window);

extern void wcs_auto_action(GtkAction *action, gpointer window);
extern void wcs_quiet_auto_action(GtkAction *action, gpointer window);
extern void wcs_existing_action(GtkAction *action, gpointer window);
extern void wcs_fit_pairs_action(GtkAction *action, gpointer window);
extern void wcs_validate_action(GtkAction *action, gpointer window);
extern void wcs_invalidate_action(GtkAction *action, gpointer window);
extern void wcs_reload_action(GtkAction *action, gpointer window);
extern void wcs_edit_action(GtkAction *action, gpointer window);

/* recipy.c */
extern void create_recipe_action(GtkAction *action, gpointer window);

/* reducegui.c */
extern void processing_action(GtkAction *action, gpointer window);
extern void switch_frame_next_action(GtkAction *action, gpointer window);
extern void switch_frame_prev_action(GtkAction *action, gpointer window);
extern void switch_frame_skip_action(GtkAction *action, gpointer window);
extern void switch_frame_qphot_action(GtkAction *action, gpointer window);
extern void switch_frame_reduce_action(GtkAction *action, gpointer window);

/* guidegui.c */
extern void open_guide_action(GtkAction *action, gpointer window);

/* mbandgui.c */
extern void mband_open_action(GtkAction *action, gpointer window);
extern void add_to_mband(gpointer dialog, char *fn);

extern void mband_save_dataset_action(GtkAction *action, gpointer data);
extern void mband_close_dataset_action(GtkAction *action, gpointer data);
extern void mband_report_action(GtkAction *action, gpointer data);
extern void mband_close_action(GtkAction *action, gpointer data);
extern void mband_select_all_action (GtkAction *action, gpointer data);
extern void mband_unselect_all_action (GtkAction *action, gpointer data);
extern void mband_hide_action (GtkAction *action, gpointer data);
extern void mband_unhide_action (GtkAction *action, gpointer data);
extern void mband_fit_zpoints_action(GtkAction *action, gpointer data);
extern void mband_fit_zp_wtrans_action(GtkAction *action, gpointer data);
extern void mband_fit_trans_action(GtkAction *action, gpointer data);
extern void mband_fit_allsky_action(GtkAction *action, gpointer data);
extern void mband_plot_resmag_action(GtkAction *action, gpointer data);
extern void mband_plot_rescol_action(GtkAction *action, gpointer data);
extern void mband_plot_errmag_action(GtkAction *action, gpointer data);
extern void mband_plot_errcol_action(GtkAction *action, gpointer data);
extern void mband_plot_zpairmass_action(GtkAction *action, gpointer data);
extern void mband_plot_zptime_action(GtkAction *action, gpointer data);
extern void mband_plot_magtime_action(GtkAction *action, gpointer data);

/* synth.c */
extern void stars_add_synth_action(GtkAction *action, gpointer window);

/* query.c */
extern void cds_query_gsc_act_action(GtkAction *action, gpointer window);
extern void cds_query_ucac2_action(GtkAction *action, gpointer window);
extern void cds_query_gsc2_action(GtkAction *action, gpointer window);
extern void cds_query_usnob_action(GtkAction *action, gpointer window);

#endif
