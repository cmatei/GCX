#ifndef _GUI_H_
#define _GUI_H_

#include <gtk/gtk.h>

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

extern void act_user_quit (GtkAction *action, gpointer window);
extern void act_file_new (GtkAction *action, gpointer window);
extern void act_about_cx (GtkAction *action, gpointer window);

extern void act_stars_auto_pairs(GtkAction *action, gpointer window);
extern void act_stars_rm_selected(GtkAction *action, gpointer window);
extern void act_stars_rm_detected(GtkAction *action, gpointer window);
extern void act_stars_rm_user(GtkAction *action, gpointer window);
extern void act_stars_rm_field(GtkAction *action, gpointer window);
extern void act_stars_rm_catalog(GtkAction *action, gpointer window);
extern void act_stars_rm_off_frame(GtkAction *action, gpointer window);
extern void act_stars_rm_all(GtkAction *action, gpointer window);
extern void act_stars_rm_pairs_all(GtkAction *action, gpointer window);
extern void act_stars_rm_pairs_selected(GtkAction *action, gpointer window);

/* from sourcesdraw.c */
extern void act_stars_show_target (GtkAction *action, gpointer window);
extern void act_stars_add_detected (GtkAction *action, gpointer window);
extern void act_stars_add_catalog (GtkAction *action, gpointer window);
extern void act_stars_add_gsc (GtkAction *action, gpointer window);
extern void act_stars_add_tycho2 (GtkAction *action, gpointer window);
extern void act_stars_edit(GtkAction *action, gpointer window);
extern void act_stars_brighter(GtkAction *action, gpointer window);
extern void act_stars_fainter(GtkAction *action, gpointer window);
extern void act_stars_redraw(GtkAction *action, gpointer window);
extern void act_stars_plot_profiles(GtkAction *action, gpointer window);

extern void act_stars_popup_edit (GtkAction *action, gpointer window);
extern void act_stars_popup_unmark (GtkAction *action, gpointer window);
extern void act_stars_popup_add_pair (GtkAction *action, gpointer window);
extern void act_stars_popup_rm_pair (GtkAction *action, gpointer window);
extern void act_stars_popup_move (GtkAction *action, gpointer window);
extern void act_stars_popup_plot_profiles (GtkAction *action, gpointer window);
extern void act_stars_popup_measure (GtkAction *action, gpointer window);
extern void act_stars_popup_plot_skyhist (GtkAction *action, gpointer window);
extern void act_stars_popup_fit_psf (GtkAction *action, gpointer window);

/* from filegui.c */
extern void act_file_open (GtkAction *action, gpointer data);
extern void act_file_save (GtkAction *action, gpointer data);
extern void act_file_export_pnm8 (GtkAction *action, gpointer data);
extern void act_file_export_pnm16 (GtkAction *action, gpointer data);

extern void act_mband_add_file(GtkAction *action, gpointer data);

extern void act_recipe_open (GtkAction *action, gpointer window);
extern void act_stars_add_gsc2_file (GtkAction *action, gpointer window);

/* from imadjust.c */
extern void set_default_channel_cuts(struct image_channel* channel);
extern void set_darea_size(GtkWidget *window, struct map_geometry *geom, double xc, double yc);
extern void drag_adjust_cuts(GtkWidget *window, int dx, int dy);
extern void pan_cursor(GtkWidget *window);
extern void show_region_stats(GtkWidget *window, double x, double y);
extern void show_zoom_cuts(GtkWidget * window);
extern void stats_cb(gpointer data, guint action);

extern void act_view_cuts_auto (GtkAction *action, gpointer window);
extern void act_view_cuts_minmax (GtkAction *action, gpointer window);
extern void act_view_cuts_flatter (GtkAction *action, gpointer window);
extern void act_view_cuts_sharper (GtkAction *action, gpointer window);
extern void act_view_cuts_brighter (GtkAction *action, gpointer window);
extern void act_view_cuts_darker (GtkAction *action, gpointer window);
extern void act_view_cuts_invert (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_1 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_2 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_3 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_4 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_5 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_6 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_7 (GtkAction *action, gpointer window);
extern void act_view_cuts_contrast_8 (GtkAction *action, gpointer window);

extern void act_view_zoom_in (GtkAction *action, gpointer window);
extern void act_view_zoom_out (GtkAction *action, gpointer window);
extern void act_view_pixels (GtkAction *action, gpointer window);
extern void act_view_pan_center (GtkAction *action, gpointer window);
extern void act_view_pan_cursor (GtkAction *action, gpointer window);

extern void act_control_histogram (GtkAction *action, gpointer window);

/* paramsgui.c */
extern void act_control_options(GtkAction *action, gpointer window);

/* cameragui.c */
extern void act_control_camera (GtkAction *action, gpointer window);

/* staredit.c */
extern void star_edit_dialog(GtkWidget *window, GSList *found);
extern void star_edit_star(GtkWidget *window, struct cat_star *cats);
extern void do_edit_star(GtkWidget *window, GSList *found, int make_std);
extern void add_star_from_catalog(gpointer window);

/* textgui.c */
extern void act_show_fits_headers (GtkAction *action, gpointer window);
extern void act_help_bindings (GtkAction *action, gpointer window);
extern void act_help_usage (GtkAction *action, gpointer window);
extern void act_help_obscript (GtkAction *action, gpointer window);
extern void act_help_repconv (GtkAction *action, gpointer window);

/* photometry.c */
extern char * phot_to_fd(gpointer window, FILE *fd, int format);

extern void act_phot_center_stars (GtkAction *action, gpointer window);
extern void act_phot_quick (GtkAction *action, gpointer window);
extern void act_phot_multi_frame (GtkAction *action, gpointer window);
extern void act_phot_to_file (GtkAction *action, gpointer window);
extern void act_phot_to_aavso (GtkAction *action, gpointer window);
extern void act_phot_to_stdout (GtkAction *action, gpointer window);


/* wcsedit.c */
extern void wcsedit_refresh(gpointer window);
extern int match_field_in_window_quiet(gpointer window);
extern int match_field_in_window(gpointer window);

extern void act_wcs_auto (GtkAction *action, gpointer window);
extern void act_wcs_quiet_auto (GtkAction *action, gpointer window);
extern void act_wcs_existing (GtkAction *action, gpointer window);
extern void act_wcs_fit_pairs (GtkAction *action, gpointer window);
extern void act_wcs_validate (GtkAction *action, gpointer window);
extern void act_wcs_invalidate (GtkAction *action, gpointer window);
extern void act_wcs_reload (GtkAction *action, gpointer window);

extern void act_control_wcs (GtkAction *action, gpointer window);

/* recipygui.c */
extern void act_recipe_create (GtkAction *action, gpointer window);

/* reducegui.c */
extern void act_control_processing (GtkAction *action, gpointer window);

extern void act_process_next (GtkAction *action, gpointer window);
extern void act_process_prev (GtkAction *action, gpointer window);
extern void act_process_skip (GtkAction *action, gpointer window);
extern void act_process_qphot (GtkAction *action, gpointer window);
extern void act_process_reduce (GtkAction *action, gpointer window);

/* guidegui.c */
extern void act_control_guider (GtkAction *action, gpointer window);

/* mbandgui.c */
extern void add_to_mband(gpointer dialog, char *fn);

extern void act_control_mband(GtkAction *action, gpointer window);

extern void act_mband_save_dataset (GtkAction *action, gpointer data);
extern void act_mband_close_dataset (GtkAction *action, gpointer data);
extern void act_mband_report (GtkAction *action, gpointer data);
extern void act_mband_close (GtkAction *action, gpointer data);
extern void act_mband_select_all  (GtkAction *action, gpointer data);
extern void act_mband_unselect_all  (GtkAction *action, gpointer data);
extern void act_mband_hide  (GtkAction *action, gpointer data);
extern void act_mband_unhide  (GtkAction *action, gpointer data);
extern void act_mband_fit_zp_null (GtkAction *action, gpointer data);
extern void act_mband_fit_zp_current (GtkAction *action, gpointer data);
extern void act_mband_fit_trans (GtkAction *action, gpointer data);
extern void act_mband_fit_allsky (GtkAction *action, gpointer data);
extern void act_mband_plot_resmag (GtkAction *action, gpointer data);
extern void act_mband_plot_rescol (GtkAction *action, gpointer data);
extern void act_mband_plot_errmag (GtkAction *action, gpointer data);
extern void act_mband_plot_errcol (GtkAction *action, gpointer data);
extern void act_mband_plot_zpairmass (GtkAction *action, gpointer data);
extern void act_mband_plot_zptime (GtkAction *action, gpointer data);
extern void act_mband_plot_magtime (GtkAction *action, gpointer data);

/* synth.c */
extern void act_stars_add_synthetic(GtkAction *action, gpointer window);

/* query.c */
extern void act_stars_add_cds_gsc_act (GtkAction *action, gpointer window);
extern void act_stars_add_cds_ucac2 (GtkAction *action, gpointer window);
extern void act_stars_add_cds_ucac3 (GtkAction *action, gpointer window);
extern void act_stars_add_cds_gsc2 (GtkAction *action, gpointer window);
extern void act_stars_add_cds_usnob (GtkAction *action, gpointer window);

#endif
