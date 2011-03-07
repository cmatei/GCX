
/* columns for the processing file list GtkListStore */
enum {
	IMFL_COL_FILENAME = 0,
	IMFL_COL_STATUS,
	IMFL_COL_IMF,

	IMFL_COL_SIZE
};



GtkWidget* create_pstar (void);
GtkWidget* create_wcs_edit (void);
GtkWidget* create_show_text (void);
GtkWidget* create_create_recipe (void);
GtkWidget* create_yes_no (void);
GtkWidget* create_par_edit (void);

/* FIXME: menu in glade file */
GtkWidget* create_image_processing (void);

/* FIXME: menu */
GtkWidget* create_mband_dialog (void);


GtkWidget* create_entry_prompt (void);
GtkWidget* create_query_log_window (void);
GtkWidget* create_camera_control (void);
GtkWidget* create_guide_window (void);
