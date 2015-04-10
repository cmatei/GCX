#ifndef _GUI_H_
#define _GUI_H_

#include <gtk/gtk.h>


/* from gui.c */
extern void error_beep(void);
extern void warning_beep(void);

extern int modal_yes_no(char *text, char *title);
extern int modal_entry_prompt(char *text, char *title, char *initial, char **value);

extern int err_printf_sb2(gpointer window, const char *fmt, ...);
extern int info_printf_sb2(gpointer window, const char *fmt, ...);

extern GtkWidget * create_image_window(void);
extern int window_auto_pairs(gpointer window);

extern struct ccd_frame *frame_from_window (gpointer window);
extern void frame_to_window(struct ccd_frame *fr, gpointer window);

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
extern void act_stars_add_ucac4 (GtkAction *action, gpointer window);
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
extern void act_stars_add_cds_ucac4 (GtkAction *action, gpointer window);
extern void act_stars_add_cds_gsc2 (GtkAction *action, gpointer window);
extern void act_stars_add_cds_usnob (GtkAction *action, gpointer window);

/* skyview.c */
extern void act_download_skyview (GtkAction *action, gpointer window);

#endif
