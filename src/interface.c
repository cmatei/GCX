
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "params.h"

GtkWidget*
create_pstar (void)
{
  GtkWidget *pstar;
  GtkWidget *vbox1;
  GtkWidget *table1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label5;
  GtkWidget *magnitude_entry;
  GtkWidget *declination_entry;
  GtkWidget *ra_entry;
  GtkWidget *designation_entry;
  GSList *startype_group = NULL;
  GtkWidget *field_star_radiob;
  GtkWidget *std_star_radiob;
  GtkWidget *ap_target_radiob;
  GtkWidget *hbox4;
  GtkWidget *astrometric_check_button;
  GtkWidget *variable_check_button;
  GtkWidget *equinox_entry;
  GtkWidget *label107;
  GtkWidget *cat_object_radiob;
  GtkWidget *vbox2;
  GtkWidget *label6;
  GtkWidget *comment_entry;
  GtkWidget *label7;
  GtkWidget *smag_entry;
  GtkWidget *label8;
  GtkWidget *imag_entry;
  GtkWidget *star_details;
  GtkWidget *hbuttonbox1;
  GtkWidget *ok_button;
  GtkWidget *make_ref_button;
  GtkWidget *cancel_button;
  GtkAccelGroup *accel_group;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  accel_group = gtk_accel_group_new ();

  pstar = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (pstar), "pstar", pstar);
  gtk_window_set_title (GTK_WINDOW (pstar), "Edit Star");
  gtk_window_set_position (GTK_WINDOW (pstar), GTK_WIN_POS_MOUSE);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (pstar), vbox1);

  table1 = gtk_table_new (5, 3, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox1), table1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 2);

  label1 = gtk_label_new ("Name:");
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label1), 3, 0);

  label2 = gtk_label_new ("R.A:");
  gtk_widget_ref (label2);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label2", label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label2), 3, 0);

  label3 = gtk_label_new ("Declination:");
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label3), 3, 0);

  label5 = gtk_label_new ("Magnitude:");
  gtk_widget_ref (label5);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label5", label5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label5), 3, 0);

  magnitude_entry = gtk_entry_new ();
  gtk_widget_ref (magnitude_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "magnitude_entry", magnitude_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (magnitude_entry);
  gtk_table_attach (GTK_TABLE (table1), magnitude_entry, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 1);
  gtk_tooltips_set_tip (tooltips, magnitude_entry, "Generic magnitude of object, used for classification only", NULL);

  declination_entry = gtk_entry_new ();
  gtk_widget_ref (declination_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "declination_entry", declination_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (declination_entry);
  gtk_table_attach (GTK_TABLE (table1), declination_entry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 1);

  ra_entry = gtk_entry_new ();
  gtk_widget_ref (ra_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "ra_entry", ra_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ra_entry);
  gtk_table_attach (GTK_TABLE (table1), ra_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 1);

  designation_entry = gtk_entry_new ();
  gtk_widget_ref (designation_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "designation_entry", designation_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (designation_entry);
  gtk_table_attach (GTK_TABLE (table1), designation_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 1);

  field_star_radiob = gtk_radio_button_new_with_label (startype_group, "Field star");
  startype_group = gtk_radio_button_group (GTK_RADIO_BUTTON (field_star_radiob));
  gtk_widget_ref (field_star_radiob);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "field_star_radiob", field_star_radiob,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (field_star_radiob);
  gtk_table_attach (GTK_TABLE (table1), field_star_radiob, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, field_star_radiob, "Object is a generic field star", NULL);

  std_star_radiob = gtk_radio_button_new_with_label (startype_group, "Standard star");
  startype_group = gtk_radio_button_group (GTK_RADIO_BUTTON (std_star_radiob));
  gtk_widget_ref (std_star_radiob);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "std_star_radiob", std_star_radiob,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (std_star_radiob);
  gtk_table_attach (GTK_TABLE (table1), std_star_radiob, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, std_star_radiob, "Objct is a photometric standard star", NULL);

  ap_target_radiob = gtk_radio_button_new_with_label (startype_group, "AP Target");
  startype_group = gtk_radio_button_group (GTK_RADIO_BUTTON (ap_target_radiob));
  gtk_widget_ref (ap_target_radiob);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "ap_target_radiob", ap_target_radiob,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ap_target_radiob);
  gtk_table_attach (GTK_TABLE (table1), ap_target_radiob, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, ap_target_radiob, "Object is a photometric target", NULL);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox4);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "hbox4", hbox4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox4);
  gtk_table_attach (GTK_TABLE (table1), hbox4, 2, 3, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  astrometric_check_button = gtk_check_button_new_with_label ("Astrometric");
  gtk_widget_ref (astrometric_check_button);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "astrometric_check_button", astrometric_check_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (astrometric_check_button);
  gtk_box_pack_start (GTK_BOX (hbox4), astrometric_check_button, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, astrometric_check_button, "Star has precise position data", NULL);

  variable_check_button = gtk_check_button_new_with_label ("Var");
  gtk_widget_ref (variable_check_button);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "variable_check_button", variable_check_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (variable_check_button);
  gtk_box_pack_start (GTK_BOX (hbox4), variable_check_button, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, variable_check_button, "Star is known to be variable", NULL);

  equinox_entry = gtk_entry_new ();
  gtk_widget_ref (equinox_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "equinox_entry", equinox_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (equinox_entry);
  gtk_table_attach (GTK_TABLE (table1), equinox_entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label107 = gtk_label_new ("Equinox:");
  gtk_widget_ref (label107);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label107", label107,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label107);
  gtk_table_attach (GTK_TABLE (table1), label107, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label107), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label107), 3, 0);

  cat_object_radiob = gtk_radio_button_new_with_label (startype_group, "Object");
  startype_group = gtk_radio_button_group (GTK_RADIO_BUTTON (cat_object_radiob));
  gtk_widget_ref (cat_object_radiob);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "cat_object_radiob", cat_object_radiob,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cat_object_radiob);
  gtk_table_attach (GTK_TABLE (table1), cat_object_radiob, 2, 3, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, cat_object_radiob, "Generic catalog object", NULL);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);

  label6 = gtk_label_new ("Comments:");
  gtk_widget_ref (label6);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label6", label6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label6);
  gtk_box_pack_start (GTK_BOX (vbox2), label6, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label6), 3, 0);

  comment_entry = gtk_entry_new ();
  gtk_widget_ref (comment_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "comment_entry", comment_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (comment_entry);
  gtk_box_pack_start (GTK_BOX (vbox2), comment_entry, FALSE, FALSE, 0);

  label7 = gtk_label_new ("Standard magnitudes");
  gtk_widget_ref (label7);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label7", label7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label7);
  gtk_box_pack_start (GTK_BOX (vbox2), label7, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label7), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label7), 7.45058e-09, 0.5);
  gtk_misc_set_padding (GTK_MISC (label7), 3, 0);

  smag_entry = gtk_entry_new ();
  gtk_widget_ref (smag_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "smag_entry", smag_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (smag_entry);
  gtk_box_pack_start (GTK_BOX (vbox2), smag_entry, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, smag_entry, "Standard magnitudes list, formated as <band>=<mag>/<error> ...", NULL);

  label8 = gtk_label_new ("Instrumental magnitudes");
  gtk_widget_ref (label8);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "label8", label8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label8);
  gtk_box_pack_start (GTK_BOX (vbox2), label8, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 7.45058e-09, 0.5);
  gtk_misc_set_padding (GTK_MISC (label8), 3, 0);

  imag_entry = gtk_entry_new ();
  gtk_widget_ref (imag_entry);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "imag_entry", imag_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (imag_entry);
  gtk_box_pack_start (GTK_BOX (vbox2), imag_entry, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, imag_entry, "Instrumental magnitudes list, formated as <band>=<mag>/<error> ...", NULL);

  star_details = gtk_label_new ("");
  gtk_widget_ref (star_details);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "star_details", star_details,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (star_details);
  gtk_box_pack_start (GTK_BOX (vbox2), star_details, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (star_details), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (star_details), 7.45058e-09, 1);
  gtk_misc_set_padding (GTK_MISC (star_details), 3, 3);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "hbuttonbox1", hbuttonbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, TRUE, 0);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 0);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 6, 0);

  ok_button = gtk_button_new_with_label ("OK");
  gtk_widget_ref (ok_button);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "ok_button", ok_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, ok_button, "Accept changes and exit", NULL);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group,
                              GDK_Return, 0,
                              GTK_ACCEL_VISIBLE);

  make_ref_button = gtk_button_new_with_label ("Make Std");
  gtk_widget_ref (make_ref_button);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "make_ref_button", make_ref_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), make_ref_button);
  gtk_container_set_border_width (GTK_CONTAINER (make_ref_button), 5);
  gtk_widget_set_sensitive (make_ref_button, FALSE);
  gtk_tooltips_set_tip (tooltips, make_ref_button, "Mark star as photometric standard", NULL);
  gtk_widget_add_accelerator (make_ref_button, "clicked", accel_group,
                              GDK_r, 0,
                              GTK_ACCEL_VISIBLE);

  cancel_button = gtk_button_new_with_label ("Undo Edits");
  gtk_widget_ref (cancel_button);
  gtk_object_set_data_full (GTK_OBJECT (pstar), "cancel_button", cancel_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), cancel_button);
  gtk_container_set_border_width (GTK_CONTAINER (cancel_button), 5);
  gtk_tooltips_set_tip (tooltips, cancel_button, "Revert to initial values", NULL);

  gtk_widget_grab_focus (smag_entry);
  gtk_widget_grab_default (ok_button);
  gtk_object_set_data (GTK_OBJECT (pstar), "tooltips", tooltips);

  gtk_window_add_accel_group (GTK_WINDOW (pstar), accel_group);

  return pstar;
}

GtkWidget*
create_camera_control (void)
{
  GtkWidget *camera_control;
  GtkWidget *vbox3;
  GtkWidget *notebook1;
  GtkWidget *vbox4;
  GtkWidget *table2;
  GtkWidget *img_bin_combo;
  GtkObject *img_exp_spin_adj;
  GtkWidget *img_exp_spin;
  GtkObject *img_width_spin_adj;
  GtkWidget *img_width_spin;
  GtkObject *img_height_spin_adj;
  GtkWidget *img_height_spin;
  GtkObject *img_x_skip_spin_adj;
  GtkWidget *img_x_skip_spin;
  GtkObject *img_y_skip_spin_adj;
  GtkWidget *img_y_skip_spin;
  GtkWidget *label15;
  GtkWidget *label16;
  GtkWidget *label17;
  GtkWidget *label18;
  GtkWidget *label19;
  GtkWidget *img_dark_checkb;
  GtkWidget *img_fullsz;
  GtkWidget *img_quartersz;
  GtkWidget *img_eighthsz;
  GtkWidget *img_halfsz;
  GtkWidget *label14;
  GtkWidget *hbox2;
  GtkWidget *img_get_img_button;
  GtkWidget *img_get_multiple_button;
  GtkWidget *img_focus_button;
  GtkWidget *label9;
  GtkWidget *vbox5;
  GtkWidget *table3;
  GtkWidget *obs_object_entry;
  GtkWidget *obs_ra_entry;
  GtkWidget *obs_dec_entry;
  GtkWidget *obs_epoch_entry;
  GtkWidget *obs_filter_combo;
  GtkWidget *obs_info_button;
  GtkWidget *label20;
  GtkWidget *label21;
  GtkWidget *label22;
  GtkWidget *label23;
  GtkWidget *label24;
  GtkWidget *obj_comment_label;
  GtkWidget *hbuttonbox3;
  GtkWidget *scope_goto_button;
  GtkWidget *scope_abort_button;
  GtkWidget *scope_auto_button;
  GtkWidget *label10;
  GtkWidget *vbox6;
  GtkWidget *obs_list_scrolledwin;
  GtkWidget *viewport1;
  GtkWidget *list1;
  GtkWidget *table4;
  GtkWidget *obs_list_fname_combo;
  GtkWidget *obs_list_fname;
  GtkWidget *obs_list_file_button;
  GtkWidget *hbox13;
  GtkWidget *label78;
  GtkWidget *hbox1;
  GtkWidget *obs_list_run_button;
  GtkWidget *obs_list_step_button;
  GtkWidget *obs_list_abort_button;
  GtkWidget *obs_list_err_stop_checkb;
  GtkWidget *label11;
  GtkWidget *vbox7;
  GtkWidget *table5;
  GtkWidget *label27;
  GtkWidget *file_combo;
  GtkWidget *file_entry;
  GtkWidget *file_auto_name_checkb;
  GtkWidget *file_match_wcs_checkb;
  GtkWidget *file_compress_checkb;
  GtkWidget *table6;
  GtkWidget *label28;
  GtkWidget *label29;
  GtkWidget *label30;
  GtkObject *file_seqn_spin_adj;
  GtkWidget *file_seqn_spin;
  GtkObject *img_number_spin_adj;
  GtkWidget *img_number_spin;
  GtkWidget *current_frame_entry;
  GtkWidget *label12;
  GtkWidget *vbox8;
  GtkWidget *table7;
  GtkWidget *table8;
  GtkWidget *label32;
  GtkWidget *label33;
  GtkObject *cooler_tempset_spin_adj;
  GtkWidget *cooler_tempset_spin;
  GtkWidget *cooler_temp_entry;
  GtkWidget *table24;
  GtkWidget *label13;
  GtkWidget *vbox24;
  GtkWidget *table22;
  GtkObject *e_limit_spin_adj;
  GtkWidget *e_limit_spin;
  GtkWidget *w_limit_checkb;
  GtkObject *w_limit_spin_adj;
  GtkWidget *w_limit_spin;
  GtkWidget *e_limit_checkb;
  GtkWidget *s_limit_checkb;
  GtkWidget *n_limit_checkb;
  GtkObject *s_limit_spin_adj;
  GtkWidget *s_limit_spin;
  GtkObject *n_limit_spin_adj;
  GtkWidget *n_limit_spin;
  GtkWidget *hseparator4;
  GtkWidget *hseparator5;
  GtkWidget *label88;
  GtkWidget *tele_port_entry;
  GtkWidget *hseparator6;
  GtkWidget *hbuttonbox13;
  GtkWidget *scope_park_button;
  GtkWidget *scope_sync_button;
  GtkWidget *scope_dither_button;
  GtkWidget *label86;
  GtkWidget *statuslabel;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  camera_control = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (camera_control), "camera_control", camera_control);
  gtk_window_set_title (GTK_WINDOW (camera_control), "Camera and Telescope Control");

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (camera_control), vbox3);

  notebook1 = gtk_notebook_new ();
  gtk_widget_ref (notebook1);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "notebook1", notebook1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (vbox3), notebook1, TRUE, TRUE, 0);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox4);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox4", vbox4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox4);

  table2 = gtk_table_new (6, 3, FALSE);
  gtk_widget_ref (table2);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table2", table2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table2);
  gtk_box_pack_start (GTK_BOX (vbox4), table2, TRUE, TRUE, 0);

  img_bin_combo = gtk_combo_box_entry_new_text ();
  gtk_widget_ref (img_bin_combo);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_bin_combo", img_bin_combo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_bin_combo);
  gtk_table_attach (GTK_TABLE (table2), img_bin_combo, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_combo_box_append_text(GTK_COMBO_BOX (img_bin_combo), "1x1");
  gtk_combo_box_append_text(GTK_COMBO_BOX (img_bin_combo), "2x2");
  gtk_combo_box_append_text(GTK_COMBO_BOX (img_bin_combo), "3x3");
  gtk_combo_box_append_text(GTK_COMBO_BOX (img_bin_combo), "4x4");
  gtk_combo_box_set_active(GTK_COMBO_BOX (img_bin_combo), 0);

  img_exp_spin_adj = gtk_adjustment_new (1, 0, 3600, 1, 10, 0);
  img_exp_spin = gtk_spin_button_new (GTK_ADJUSTMENT (img_exp_spin_adj), 1, 2);
  gtk_widget_ref (img_exp_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_exp_spin", img_exp_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_exp_spin);
  gtk_table_attach (GTK_TABLE (table2), img_exp_spin, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (img_exp_spin), TRUE);
  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (img_exp_spin), TRUE);

  img_width_spin_adj = gtk_adjustment_new (1, 0, 100, 10, 10, 0);
  img_width_spin = gtk_spin_button_new (GTK_ADJUSTMENT (img_width_spin_adj), 1, 0);
  gtk_widget_ref (img_width_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_width_spin", img_width_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_width_spin);
  gtk_table_attach (GTK_TABLE (table2), img_width_spin, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (img_width_spin), TRUE);

  img_height_spin_adj = gtk_adjustment_new (1, 0, 100, 10, 10, 0);
  img_height_spin = gtk_spin_button_new (GTK_ADJUSTMENT (img_height_spin_adj), 1, 0);
  gtk_widget_ref (img_height_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_height_spin", img_height_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_height_spin);
  gtk_table_attach (GTK_TABLE (table2), img_height_spin, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  img_x_skip_spin_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 0);
  img_x_skip_spin = gtk_spin_button_new (GTK_ADJUSTMENT (img_x_skip_spin_adj), 1, 0);
  gtk_widget_ref (img_x_skip_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_x_skip_spin", img_x_skip_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_x_skip_spin);
  gtk_table_attach (GTK_TABLE (table2), img_x_skip_spin, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  img_y_skip_spin_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 0);
  img_y_skip_spin = gtk_spin_button_new (GTK_ADJUSTMENT (img_y_skip_spin_adj), 1, 0);
  gtk_widget_ref (img_y_skip_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_y_skip_spin", img_y_skip_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_y_skip_spin);
  gtk_table_attach (GTK_TABLE (table2), img_y_skip_spin, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label15 = gtk_label_new ("Exptime");
  gtk_widget_ref (label15);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label15", label15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label15);
  gtk_table_attach (GTK_TABLE (table2), label15, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label15), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label15), 3, 0);

  label16 = gtk_label_new ("Width");
  gtk_widget_ref (label16);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label16", label16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label16);
  gtk_table_attach (GTK_TABLE (table2), label16, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label16), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label16), 3, 0);

  label17 = gtk_label_new ("Height");
  gtk_widget_ref (label17);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label17", label17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label17);
  gtk_table_attach (GTK_TABLE (table2), label17, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label17), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label17), 3, 0);

  label18 = gtk_label_new ("X-Skip");
  gtk_widget_ref (label18);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label18", label18,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label18);
  gtk_table_attach (GTK_TABLE (table2), label18, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label18), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label18), 3, 0);

  label19 = gtk_label_new ("Y-Skip");
  gtk_widget_ref (label19);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label19", label19,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label19);
  gtk_table_attach (GTK_TABLE (table2), label19, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label19), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label19), 3, 0);

  img_dark_checkb = gtk_check_button_new_with_label ("Dark");
  gtk_widget_ref (img_dark_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_dark_checkb", img_dark_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_dark_checkb);
  gtk_table_attach (GTK_TABLE (table2), img_dark_checkb, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, img_dark_checkb, "Keep the shutter closed in order to obtain dark or bias frames", NULL);

  img_fullsz = gtk_button_new_with_label ("Full Size");
  gtk_widget_ref (img_fullsz);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_fullsz", img_fullsz,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_fullsz);
  gtk_table_attach (GTK_TABLE (table2), img_fullsz, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, img_fullsz, "Set the window to the full sensor area", NULL);

  img_quartersz = gtk_button_new_with_label ("1/4 Size");
  gtk_widget_ref (img_quartersz);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_quartersz", img_quartersz,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_quartersz);
  gtk_table_attach (GTK_TABLE (table2), img_quartersz, 2, 3, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, img_quartersz, "Set the window to a quarter of the full sensor area", NULL);

  img_eighthsz = gtk_button_new_with_label ("1/8 Size");
  gtk_widget_ref (img_eighthsz);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_eighthsz", img_eighthsz,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_eighthsz);
  gtk_table_attach (GTK_TABLE (table2), img_eighthsz, 2, 3, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, img_eighthsz, "Set the window to one-eighth of the sensor area", NULL);

  img_halfsz = gtk_button_new_with_label ("Half Size");
  gtk_widget_ref (img_halfsz);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_halfsz", img_halfsz,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_halfsz);
  gtk_table_attach (GTK_TABLE (table2), img_halfsz, 2, 3, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, img_halfsz, "Set the window to half the full sensor area", NULL);

  label14 = gtk_label_new ("Binning");
  gtk_widget_ref (label14);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label14", label14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label14);
  gtk_table_attach (GTK_TABLE (table2), label14, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label14), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label14), 3, 0);

  hbox2 = gtk_hbox_new (TRUE, 0);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hbox2", hbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox2, FALSE, TRUE, 3);

  img_get_img_button = gtk_button_new_with_label ("Get");
  gtk_widget_ref (img_get_img_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_get_img_button", img_get_img_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_get_img_button);
  gtk_box_pack_start (GTK_BOX (hbox2), img_get_img_button, TRUE, TRUE, 2);
  gtk_tooltips_set_tip (tooltips, img_get_img_button, "Get and display a single frame, no automatic save", NULL);

  img_get_multiple_button = gtk_toggle_button_new_with_label ("Multiple");
  gtk_widget_ref (img_get_multiple_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_get_multiple_button", img_get_multiple_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_get_multiple_button);
  gtk_box_pack_start (GTK_BOX (hbox2), img_get_multiple_button, TRUE, TRUE, 2);
  gtk_tooltips_set_tip (tooltips, img_get_multiple_button, "Get and save multiple frames", NULL);

  img_focus_button = gtk_toggle_button_new_with_label ("Focus");
  gtk_widget_ref (img_focus_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_focus_button", img_focus_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_focus_button);
  gtk_box_pack_start (GTK_BOX (hbox2), img_focus_button, TRUE, TRUE, 2);
  gtk_tooltips_set_tip (tooltips, img_focus_button, "Get and display frames continuously, no save", NULL);

  label9 = gtk_label_new ("Frame");
  gtk_widget_ref (label9);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label9", label9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label9);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), label9);

  vbox5 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox5);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox5", vbox5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox5);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox5);

  table3 = gtk_table_new (6, 3, FALSE);
  gtk_widget_ref (table3);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table3", table3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table3);
  gtk_box_pack_start (GTK_BOX (vbox5), table3, TRUE, TRUE, 0);

  obs_object_entry = gtk_entry_new ();
  gtk_widget_ref (obs_object_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_object_entry", obs_object_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_object_entry);
  gtk_table_attach (GTK_TABLE (table3), obs_object_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  obs_ra_entry = gtk_entry_new ();
  gtk_widget_ref (obs_ra_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_ra_entry", obs_ra_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_ra_entry);
  gtk_table_attach (GTK_TABLE (table3), obs_ra_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  obs_dec_entry = gtk_entry_new ();
  gtk_widget_ref (obs_dec_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_dec_entry", obs_dec_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_dec_entry);
  gtk_table_attach (GTK_TABLE (table3), obs_dec_entry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  obs_epoch_entry = gtk_entry_new ();
  gtk_widget_ref (obs_epoch_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_epoch_entry", obs_epoch_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_epoch_entry);
  gtk_table_attach (GTK_TABLE (table3), obs_epoch_entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  obs_filter_combo = gtk_combo_box_new_text();
  gtk_widget_ref (obs_filter_combo);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_filter_combo", obs_filter_combo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_filter_combo);
  gtk_table_attach (GTK_TABLE (table3), obs_filter_combo, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  obs_info_button = gtk_button_new_with_label (" i ");
  gtk_widget_ref (obs_info_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_info_button", obs_info_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_info_button);
  gtk_table_attach (GTK_TABLE (table3), obs_info_button, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_sensitive (obs_info_button, FALSE);
  gtk_tooltips_set_tip (tooltips, obs_info_button, "Show object information", NULL);

  label20 = gtk_label_new ("Object:");
  gtk_widget_ref (label20);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label20", label20,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label20);
  gtk_table_attach (GTK_TABLE (table3), label20, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label20), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label20), 3, 0);

  label21 = gtk_label_new ("R.A:");
  gtk_widget_ref (label21);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label21", label21,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label21);
  gtk_table_attach (GTK_TABLE (table3), label21, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label21), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label21), 3, 0);

  label22 = gtk_label_new ("Dec:");
  gtk_widget_ref (label22);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label22", label22,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label22);
  gtk_table_attach (GTK_TABLE (table3), label22, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label22), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label22), 3, 0);

  label23 = gtk_label_new ("Epoch:");
  gtk_widget_ref (label23);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label23", label23,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label23);
  gtk_table_attach (GTK_TABLE (table3), label23, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label23), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label23), 3, 0);

  label24 = gtk_label_new ("Filter:");
  gtk_widget_ref (label24);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label24", label24,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label24);
  gtk_table_attach (GTK_TABLE (table3), label24, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label24), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (label24), 3, 0);

  obj_comment_label = gtk_label_new ("");
  gtk_widget_ref (obj_comment_label);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obj_comment_label", obj_comment_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obj_comment_label);
  gtk_table_attach (GTK_TABLE (table3), obj_comment_label, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (obj_comment_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (obj_comment_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (obj_comment_label), 0, 3);

  hbuttonbox3 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox3);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hbuttonbox3", hbuttonbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox3);
  gtk_box_pack_start (GTK_BOX (vbox5), hbuttonbox3, FALSE, TRUE, 0);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox3), 10);

  scope_goto_button = gtk_button_new_with_label ("Goto");
  gtk_widget_ref (scope_goto_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "scope_goto_button", scope_goto_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scope_goto_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), scope_goto_button);
  GTK_WIDGET_SET_FLAGS (scope_goto_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, scope_goto_button, "Goto selected object", NULL);

  scope_abort_button = gtk_button_new_with_label ("Abort");
  gtk_widget_ref (scope_abort_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "scope_abort_button", scope_abort_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scope_abort_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), scope_abort_button);
  GTK_WIDGET_SET_FLAGS (scope_abort_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, scope_abort_button, "Abort goto operation", NULL);

  scope_auto_button = gtk_button_new_with_label ("Center");
  gtk_widget_ref (scope_auto_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "scope_auto_button", scope_auto_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scope_auto_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), scope_auto_button);
  GTK_WIDGET_SET_FLAGS (scope_auto_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, scope_auto_button, "Match field and center target object", NULL);

  label10 = gtk_label_new ("Obs");
  gtk_widget_ref (label10);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label10", label10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label10);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), label10);

  vbox6 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox6);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox6", vbox6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox6);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox6);

  obs_list_scrolledwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (obs_list_scrolledwin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_scrolledwin", obs_list_scrolledwin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_scrolledwin);
  gtk_box_pack_start (GTK_BOX (vbox6), obs_list_scrolledwin, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (obs_list_scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport1 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport1);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "viewport1", viewport1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport1);
  gtk_container_add (GTK_CONTAINER (obs_list_scrolledwin), viewport1);

  list1 = gtk_list_new ();
  gtk_widget_ref (list1);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "list1", list1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (list1);
  gtk_container_add (GTK_CONTAINER (viewport1), list1);
  gtk_list_set_selection_mode (GTK_LIST (list1), GTK_SELECTION_BROWSE);

  table4 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table4);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table4", table4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table4);
  gtk_box_pack_start (GTK_BOX (vbox6), table4, FALSE, TRUE, 2);

  obs_list_fname_combo = gtk_combo_new ();
  gtk_widget_ref (obs_list_fname_combo);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_fname_combo", obs_list_fname_combo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_fname_combo);
  gtk_table_attach (GTK_TABLE (table4), obs_list_fname_combo, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  obs_list_fname = GTK_COMBO (obs_list_fname_combo)->entry;
  gtk_widget_ref (obs_list_fname);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_fname", obs_list_fname,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_fname);

  obs_list_file_button = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (obs_list_file_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_file_button", obs_list_file_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_file_button);
  gtk_table_attach (GTK_TABLE (table4), obs_list_file_button, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, obs_list_file_button, "Browse for observation list filename", NULL);

  hbox13 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox13);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hbox13", hbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox13);
  gtk_table_attach (GTK_TABLE (table4), hbox13, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  label78 = gtk_label_new ("Obslist file");
  gtk_widget_ref (label78);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label78", label78,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label78);
  gtk_box_pack_start (GTK_BOX (hbox13), label78, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label78), 7.45058e-09, 1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox6), hbox1, FALSE, TRUE, 0);

  obs_list_run_button = gtk_toggle_button_new_with_label ("Run");
  gtk_widget_ref (obs_list_run_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_run_button", obs_list_run_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_run_button);
  gtk_box_pack_start (GTK_BOX (hbox1), obs_list_run_button, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, obs_list_run_button, "Run the observation list", NULL);

  obs_list_step_button = gtk_toggle_button_new_with_label ("Step");
  gtk_widget_ref (obs_list_step_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_step_button", obs_list_step_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_step_button);
  gtk_box_pack_start (GTK_BOX (hbox1), obs_list_step_button, TRUE, TRUE, 8);
  gtk_tooltips_set_tip (tooltips, obs_list_step_button, "Run one step of the observation list", NULL);

  obs_list_abort_button = gtk_button_new_with_label ("Abort");
  gtk_widget_ref (obs_list_abort_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_abort_button", obs_list_abort_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_abort_button);
  gtk_box_pack_start (GTK_BOX (hbox1), obs_list_abort_button, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, obs_list_abort_button, "Abort current operation", NULL);

  obs_list_err_stop_checkb = gtk_check_button_new_with_label ("Stop on errors");
  gtk_widget_ref (obs_list_err_stop_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "obs_list_err_stop_checkb", obs_list_err_stop_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (obs_list_err_stop_checkb);
  gtk_box_pack_start (GTK_BOX (hbox1), obs_list_err_stop_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, obs_list_err_stop_checkb, "On errors, stop from running through the list", NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (obs_list_err_stop_checkb), TRUE);

  label11 = gtk_label_new ("Obslist");
  gtk_widget_ref (label11);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label11", label11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label11);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), label11);

  vbox7 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox7);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox7", vbox7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox7);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox7);

  table5 = gtk_table_new (2, 1, FALSE);
  gtk_widget_ref (table5);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table5", table5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table5);
  gtk_box_pack_start (GTK_BOX (vbox7), table5, FALSE, TRUE, 0);

  label27 = gtk_label_new ("Filename for image saves");
  gtk_widget_ref (label27);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label27", label27,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label27);
  gtk_table_attach (GTK_TABLE (table5), label27, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label27), 0, 0.5);

  file_combo = gtk_combo_new ();
  gtk_widget_ref (file_combo);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "file_combo", file_combo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file_combo);
  gtk_table_attach (GTK_TABLE (table5), file_combo, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  file_entry = GTK_COMBO (file_combo)->entry;
  gtk_widget_ref (file_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "file_entry", file_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file_entry);

  file_auto_name_checkb = gtk_check_button_new_with_label ("Generate filenames automatically");
  gtk_widget_ref (file_auto_name_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "file_auto_name_checkb", file_auto_name_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file_auto_name_checkb);
  gtk_box_pack_start (GTK_BOX (vbox7), file_auto_name_checkb, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (file_auto_name_checkb), TRUE);

  file_match_wcs_checkb = gtk_check_button_new_with_label ("Match frame wcs before saving");
  gtk_widget_ref (file_match_wcs_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "file_match_wcs_checkb", file_match_wcs_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file_match_wcs_checkb);
  gtk_box_pack_start (GTK_BOX (vbox7), file_match_wcs_checkb, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (file_match_wcs_checkb), TRUE);

  file_compress_checkb = gtk_check_button_new_with_label ("Compress fits files");
  gtk_widget_ref (file_compress_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "file_compress_checkb", file_compress_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file_compress_checkb);
  gtk_box_pack_start (GTK_BOX (vbox7), file_compress_checkb, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (file_compress_checkb), TRUE);

  table6 = gtk_table_new (2, 3, TRUE);
  gtk_widget_ref (table6);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table6", table6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table6);
  gtk_box_pack_start (GTK_BOX (vbox7), table6, TRUE, TRUE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table6), 3);

  label28 = gtk_label_new ("Sequence #");
  gtk_widget_ref (label28);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label28", label28,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label28);
  gtk_table_attach (GTK_TABLE (table6), label28, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label28), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label28), 2, 0);

  label29 = gtk_label_new ("Total frames");
  gtk_widget_ref (label29);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label29", label29,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label29);
  gtk_table_attach (GTK_TABLE (table6), label29, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label29), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label29), 2, 0);

  label30 = gtk_label_new ("To read");
  gtk_widget_ref (label30);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label30", label30,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label30);
  gtk_table_attach (GTK_TABLE (table6), label30, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label30), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label30), 2, 0);

  file_seqn_spin_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 0);
  file_seqn_spin = gtk_spin_button_new (GTK_ADJUSTMENT (file_seqn_spin_adj), 1, 0);
  gtk_widget_ref (file_seqn_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "file_seqn_spin", file_seqn_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file_seqn_spin);
  gtk_table_attach (GTK_TABLE (table6), file_seqn_spin, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  img_number_spin_adj = gtk_adjustment_new (2, 1, 100, 1, 10, 0);
  img_number_spin = gtk_spin_button_new (GTK_ADJUSTMENT (img_number_spin_adj), 1, 0);
  gtk_widget_ref (img_number_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "img_number_spin", img_number_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (img_number_spin);
  gtk_table_attach (GTK_TABLE (table6), img_number_spin, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  current_frame_entry = gtk_entry_new ();
  gtk_widget_ref (current_frame_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "current_frame_entry", current_frame_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (current_frame_entry);
  gtk_table_attach (GTK_TABLE (table6), current_frame_entry, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (current_frame_entry, 50, -2);
  gtk_entry_set_editable (GTK_ENTRY (current_frame_entry), FALSE);

  label12 = gtk_label_new ("Files");
  gtk_widget_ref (label12);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label12", label12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label12);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 3), label12);

  vbox8 = gtk_vbox_new (FALSE, 3);
  gtk_widget_ref (vbox8);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox8", vbox8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox8);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox8);

  table7 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table7);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table7", table7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table7);
  gtk_box_pack_start (GTK_BOX (vbox8), table7, FALSE, TRUE, 0);

  table8 = gtk_table_new (3, 2, FALSE);
  gtk_widget_ref (table8);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table8", table8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table8);
  gtk_box_pack_start (GTK_BOX (vbox8), table8, FALSE, TRUE, 0);

  label32 = gtk_label_new ("Current temp");
  gtk_widget_ref (label32);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label32", label32,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label32);
  gtk_table_attach (GTK_TABLE (table8), label32, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label32), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label32), 3, 0);

  label33 = gtk_label_new ("Temp setpoint");
  gtk_widget_ref (label33);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label33", label33,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label33);
  gtk_table_attach (GTK_TABLE (table8), label33, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label33), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label33), 3, 0);

  cooler_tempset_spin_adj = gtk_adjustment_new (-35, -50, 30, 5, 10, 0);
  cooler_tempset_spin = gtk_spin_button_new (GTK_ADJUSTMENT (cooler_tempset_spin_adj), 1, 0);
  gtk_widget_ref (cooler_tempset_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "cooler_tempset_spin", cooler_tempset_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cooler_tempset_spin);
  gtk_table_attach (GTK_TABLE (table8), cooler_tempset_spin, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (cooler_tempset_spin), TRUE);

  cooler_temp_entry = gtk_entry_new ();
  gtk_widget_ref (cooler_temp_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "cooler_temp_entry", cooler_temp_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cooler_temp_entry);
  gtk_table_attach (GTK_TABLE (table8), cooler_temp_entry, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_editable (GTK_ENTRY (cooler_temp_entry), FALSE);

  table24 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table24);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table24", table24,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table24);
  gtk_box_pack_start (GTK_BOX (vbox8), table24, TRUE, TRUE, 0);

  label13 = gtk_label_new ("Camera");
  gtk_widget_ref (label13);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label13", label13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label13);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 4), label13);

  vbox24 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox24);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "vbox24", vbox24,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox24);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox24);

  table22 = gtk_table_new (10, 2, FALSE);
  gtk_widget_ref (table22);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "table22", table22,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table22);
  gtk_box_pack_start (GTK_BOX (vbox24), table22, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table22), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table22), 6);

  e_limit_spin_adj = gtk_adjustment_new (1, -6, 2, 0.5, 10, 0);
  e_limit_spin = gtk_spin_button_new (GTK_ADJUSTMENT (e_limit_spin_adj), 1, 1);
  gtk_widget_ref (e_limit_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "e_limit_spin", e_limit_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (e_limit_spin);
  gtk_table_attach (GTK_TABLE (table22), e_limit_spin, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, e_limit_spin, "East limit for goto operations (in hours of hour angle)", NULL);

  w_limit_checkb = gtk_check_button_new_with_label ("W Limit (max HA)");
  gtk_widget_ref (w_limit_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "w_limit_checkb", w_limit_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (w_limit_checkb);
  gtk_table_attach (GTK_TABLE (table22), w_limit_checkb, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, w_limit_checkb, "Prohibit goto operations to positions west of the hour angle below", NULL);

  w_limit_spin_adj = gtk_adjustment_new (1, -2, 6, 0.5, 10, 0);
  w_limit_spin = gtk_spin_button_new (GTK_ADJUSTMENT (w_limit_spin_adj), 1, 1);
  gtk_widget_ref (w_limit_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "w_limit_spin", w_limit_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (w_limit_spin);
  gtk_table_attach (GTK_TABLE (table22), w_limit_spin, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, w_limit_spin, "West limit for goto operations (in hours of hour angle)", NULL);

  e_limit_checkb = gtk_check_button_new_with_label ("E Limit (min HA)");
  gtk_widget_ref (e_limit_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "e_limit_checkb", e_limit_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (e_limit_checkb);
  gtk_table_attach (GTK_TABLE (table22), e_limit_checkb, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, e_limit_checkb, "Prohibit goto operations to positions east of the hour angle below", NULL);

  s_limit_checkb = gtk_check_button_new_with_label ("S Limit (min Dec)");
  gtk_widget_ref (s_limit_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "s_limit_checkb", s_limit_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (s_limit_checkb);
  gtk_table_attach (GTK_TABLE (table22), s_limit_checkb, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, s_limit_checkb, "Prohibit goto operations to positions south of the declination below", NULL);

  n_limit_checkb = gtk_check_button_new_with_label ("N Limit (max Dec)");
  gtk_widget_ref (n_limit_checkb);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "n_limit_checkb", n_limit_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (n_limit_checkb);
  gtk_table_attach (GTK_TABLE (table22), n_limit_checkb, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, n_limit_checkb, "Prohibit goto operations to positions north of the declination below", NULL);

  s_limit_spin_adj = gtk_adjustment_new (-45, -90, 90, 1, 10, 0);
  s_limit_spin = gtk_spin_button_new (GTK_ADJUSTMENT (s_limit_spin_adj), 1, 1);
  gtk_widget_ref (s_limit_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "s_limit_spin", s_limit_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (s_limit_spin);
  gtk_table_attach (GTK_TABLE (table22), s_limit_spin, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, s_limit_spin, "South limit for goto operations (in degrees of declination)", NULL);

  n_limit_spin_adj = gtk_adjustment_new (80, -90, 90, 1, 10, 0);
  n_limit_spin = gtk_spin_button_new (GTK_ADJUSTMENT (n_limit_spin_adj), 1, 1);
  gtk_widget_ref (n_limit_spin);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "n_limit_spin", n_limit_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (n_limit_spin);
  gtk_table_attach (GTK_TABLE (table22), n_limit_spin, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, n_limit_spin, "North limit for goto operations (in degrees of declination)", NULL);

  hseparator4 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator4);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hseparator4", hseparator4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator4);
  gtk_table_attach (GTK_TABLE (table22), hseparator4, 0, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 3);

  hseparator5 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator5);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hseparator5", hseparator5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator5);
  gtk_table_attach (GTK_TABLE (table22), hseparator5, 0, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 3);

  label88 = gtk_label_new ("Telescope Port");
  gtk_widget_ref (label88);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label88", label88,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label88);
  gtk_table_attach (GTK_TABLE (table22), label88, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label88), 0, 0.5);

  tele_port_entry = gtk_entry_new ();
  gtk_widget_ref (tele_port_entry);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "tele_port_entry", tele_port_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (tele_port_entry);
  gtk_table_attach (GTK_TABLE (table22), tele_port_entry, 0, 2, 7, 8,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_sensitive (tele_port_entry, FALSE);

  hseparator6 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator6);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hseparator6", hseparator6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator6);
  gtk_table_attach (GTK_TABLE (table22), hseparator6, 0, 2, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 3);

  hbuttonbox13 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox13);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "hbuttonbox13", hbuttonbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox13);
  gtk_table_attach (GTK_TABLE (table22), hbuttonbox13, 0, 2, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox13), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox13), 68, 27);

  scope_park_button = gtk_button_new_with_label ("Park");
  gtk_widget_ref (scope_park_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "scope_park_button", scope_park_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scope_park_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox13), scope_park_button);
  gtk_widget_set_sensitive (scope_park_button, FALSE);
  GTK_WIDGET_SET_FLAGS (scope_park_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, scope_park_button, "Slew telescope to parking position", NULL);

  scope_sync_button = gtk_button_new_with_label ("Sync");
  gtk_widget_ref (scope_sync_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "scope_sync_button", scope_sync_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scope_sync_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox13), scope_sync_button);
  GTK_WIDGET_SET_FLAGS (scope_sync_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, scope_sync_button, "Synchronise telescope pointing to current obs coordinates", NULL);

  scope_dither_button = gtk_button_new_with_label ("Dither");
  gtk_widget_ref (scope_dither_button);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "scope_dither_button", scope_dither_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scope_dither_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox13), scope_dither_button);
  GTK_WIDGET_SET_FLAGS (scope_dither_button, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, scope_dither_button, "Dither (small random) telescope movement", NULL);

  label86 = gtk_label_new ("Telescope");
  gtk_widget_ref (label86);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "label86", label86,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label86);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 5), label86);

  statuslabel = gtk_label_new ("");
  gtk_widget_ref (statuslabel);
  gtk_object_set_data_full (GTK_OBJECT (camera_control), "statuslabel", statuslabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (statuslabel);
  gtk_box_pack_start (GTK_BOX (vbox3), statuslabel, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (statuslabel), 7.45058e-09, 0.5);
  gtk_misc_set_padding (GTK_MISC (statuslabel), 3, 3);

  gtk_object_set_data (GTK_OBJECT (camera_control), "tooltips", tooltips);

  return camera_control;
}

GtkWidget*
create_wcs_edit (void)
{
  GtkWidget *wcs_edit;
  GtkWidget *vbox9;
  GtkWidget *hbox3;
  GtkWidget *vbox10;
  GtkWidget *label38;
  GtkWidget *wcs_ra_entry;
  GtkWidget *label39;
  GtkWidget *wcs_dec_entry;
  GtkWidget *label40;
  GtkWidget *wcs_equinox_entry;
  GtkWidget *label41;
  GtkWidget *wcs_v_scale_entry;
  GtkWidget *label42;
  GtkWidget *wcs_h_scale_entry;
  GtkWidget *label43;
  GtkWidget *wcs_rot_entry;
  GtkWidget *vbox12;
  GtkWidget *frame1;
  GtkWidget *vbox11;
  GSList *_1_group = NULL;
  guint wcs_unset_rb_key;
  GtkWidget *wcs_unset_rb;
  guint wcs_initial_rb_key;
  GtkWidget *wcs_initial_rb;
  guint wcs_fitted_rb_key;
  GtkWidget *wcs_fitted_rb;
  guint wcs_valid_rb_key;
  GtkWidget *wcs_valid_rb;
  GtkWidget *frame16;
  GtkWidget *fixed1;
  GtkWidget *wcs_step_button;
  GtkWidget *wcs_rot_inc_button;
  GtkWidget *wcs_rot_dec_button;
  GtkWidget *wcs_north_button;
  GtkWidget *wcs_south_button;
  GtkWidget *wcs_west_button;
  GtkWidget *wcs_east_button;
  GtkWidget *hseparator1;
  GtkWidget *hbuttonbox4;
  GtkWidget *wcs_ok_button;
  GtkWidget *wcs_close_button;
  GtkAccelGroup *accel_group;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  accel_group = gtk_accel_group_new ();

  wcs_edit = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (wcs_edit), "wcs_edit", wcs_edit);
  gtk_window_set_title (GTK_WINDOW (wcs_edit), "Edit WCS");
  gtk_window_set_position (GTK_WINDOW (wcs_edit), GTK_WIN_POS_MOUSE);

  vbox9 = gtk_vbox_new (FALSE, 4);
  gtk_widget_ref (vbox9);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "vbox9", vbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox9);
  gtk_container_add (GTK_CONTAINER (wcs_edit), vbox9);
  gtk_container_set_border_width (GTK_CONTAINER (vbox9), 5);

  hbox3 = gtk_hbox_new (FALSE, 16);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "hbox3", hbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (vbox9), hbox3, TRUE, TRUE, 0);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox10);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "vbox10", vbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox10);
  gtk_box_pack_start (GTK_BOX (hbox3), vbox10, TRUE, TRUE, 0);

  label38 = gtk_label_new ("R.A. of field center");
  gtk_widget_ref (label38);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "label38", label38,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label38);
  gtk_box_pack_start (GTK_BOX (vbox10), label38, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label38), 0, 0.71);
  gtk_misc_set_padding (GTK_MISC (label38), 3, 0);

  wcs_ra_entry = gtk_entry_new ();
  gtk_widget_ref (wcs_ra_entry);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_ra_entry", wcs_ra_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_ra_entry);
  gtk_box_pack_start (GTK_BOX (vbox10), wcs_ra_entry, FALSE, FALSE, 0);

  label39 = gtk_label_new ("Declination of field center");
  gtk_widget_ref (label39);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "label39", label39,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label39);
  gtk_box_pack_start (GTK_BOX (vbox10), label39, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label39), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label39), 3, 0);

  wcs_dec_entry = gtk_entry_new ();
  gtk_widget_ref (wcs_dec_entry);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_dec_entry", wcs_dec_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_dec_entry);
  gtk_box_pack_start (GTK_BOX (vbox10), wcs_dec_entry, FALSE, FALSE, 0);

  label40 = gtk_label_new ("Equinox of coordinates");
  gtk_widget_ref (label40);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "label40", label40,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label40);
  gtk_box_pack_start (GTK_BOX (vbox10), label40, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label40), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label40), 3, 0);

  wcs_equinox_entry = gtk_entry_new ();
  gtk_widget_ref (wcs_equinox_entry);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_equinox_entry", wcs_equinox_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_equinox_entry);
  gtk_box_pack_start (GTK_BOX (vbox10), wcs_equinox_entry, FALSE, FALSE, 0);

  label41 = gtk_label_new ("Arcsec/pixel down (negative if N is up)");
  gtk_widget_ref (label41);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "label41", label41,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label41);
  gtk_box_pack_start (GTK_BOX (vbox10), label41, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label41), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label41), 3, 0);

  wcs_v_scale_entry = gtk_entry_new ();
  gtk_widget_ref (wcs_v_scale_entry);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_v_scale_entry", wcs_v_scale_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_v_scale_entry);
  gtk_box_pack_start (GTK_BOX (vbox10), wcs_v_scale_entry, FALSE, FALSE, 0);

  label42 = gtk_label_new ("Arcsec/pixel right (negative if W is right)");
  gtk_widget_ref (label42);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "label42", label42,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label42);
  gtk_box_pack_start (GTK_BOX (vbox10), label42, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label42), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label42), 3, 0);

  wcs_h_scale_entry = gtk_entry_new ();
  gtk_widget_ref (wcs_h_scale_entry);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_h_scale_entry", wcs_h_scale_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_h_scale_entry);
  gtk_box_pack_start (GTK_BOX (vbox10), wcs_h_scale_entry, FALSE, FALSE, 0);

  label43 = gtk_label_new ("Field rotation (degrees E of N)");
  gtk_widget_ref (label43);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "label43", label43,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label43);
  gtk_box_pack_start (GTK_BOX (vbox10), label43, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label43), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label43), 3, 0);

  wcs_rot_entry = gtk_entry_new ();
  gtk_widget_ref (wcs_rot_entry);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_rot_entry", wcs_rot_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_rot_entry);
  gtk_box_pack_start (GTK_BOX (vbox10), wcs_rot_entry, FALSE, FALSE, 0);

  vbox12 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox12);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "vbox12", vbox12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox12);
  gtk_box_pack_start (GTK_BOX (hbox3), vbox12, TRUE, TRUE, 0);

  frame1 = gtk_frame_new ("WCS status");
  gtk_widget_ref (frame1);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "frame1", frame1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (vbox12), frame1, FALSE, FALSE, 0);

  vbox11 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox11);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "vbox11", vbox11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox11);
  gtk_container_add (GTK_CONTAINER (frame1), vbox11);

  wcs_unset_rb = gtk_radio_button_new_with_label (_1_group, "");
  wcs_unset_rb_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (wcs_unset_rb)->child),
                                   "_Unset");
  gtk_widget_add_accelerator (wcs_unset_rb, "clicked", accel_group,
                              wcs_unset_rb_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  _1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (wcs_unset_rb));
  gtk_widget_ref (wcs_unset_rb);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_unset_rb", wcs_unset_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_unset_rb);
  gtk_box_pack_start (GTK_BOX (vbox11), wcs_unset_rb, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (wcs_unset_rb, FALSE);

  wcs_initial_rb = gtk_radio_button_new_with_label (_1_group, "");
  wcs_initial_rb_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (wcs_initial_rb)->child),
                                   "_Initial");
  gtk_widget_add_accelerator (wcs_initial_rb, "clicked", accel_group,
                              wcs_initial_rb_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  _1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (wcs_initial_rb));
  gtk_widget_ref (wcs_initial_rb);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_initial_rb", wcs_initial_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_initial_rb);
  gtk_box_pack_start (GTK_BOX (vbox11), wcs_initial_rb, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (wcs_initial_rb, FALSE);

  wcs_fitted_rb = gtk_radio_button_new_with_label (_1_group, "");
  wcs_fitted_rb_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (wcs_fitted_rb)->child),
                                   "_Fitted");
  gtk_widget_add_accelerator (wcs_fitted_rb, "clicked", accel_group,
                              wcs_fitted_rb_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  _1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (wcs_fitted_rb));
  gtk_widget_ref (wcs_fitted_rb);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_fitted_rb", wcs_fitted_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_fitted_rb);
  gtk_box_pack_start (GTK_BOX (vbox11), wcs_fitted_rb, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (wcs_fitted_rb, FALSE);

  wcs_valid_rb = gtk_radio_button_new_with_label (_1_group, "");
  wcs_valid_rb_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (wcs_valid_rb)->child),
                                   "_Validated");
  gtk_widget_add_accelerator (wcs_valid_rb, "clicked", accel_group,
                              wcs_valid_rb_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  _1_group = gtk_radio_button_group (GTK_RADIO_BUTTON (wcs_valid_rb));
  gtk_widget_ref (wcs_valid_rb);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_valid_rb", wcs_valid_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_valid_rb);
  gtk_box_pack_start (GTK_BOX (vbox11), wcs_valid_rb, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (wcs_valid_rb, FALSE);

  frame16 = gtk_frame_new ("Move WCS");
  gtk_widget_ref (frame16);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "frame16", frame16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame16);
  gtk_box_pack_start (GTK_BOX (vbox12), frame16, TRUE, TRUE, 0);

  fixed1 = gtk_fixed_new ();
  gtk_widget_ref (fixed1);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "fixed1", fixed1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (frame16), fixed1);
  gtk_fixed_set_has_window(GTK_FIXED(fixed1), TRUE);
  gtk_widget_show (fixed1);

  wcs_step_button = gtk_toggle_button_new_with_label ("x1");
  gtk_widget_ref (wcs_step_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_step_button", wcs_step_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_step_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_step_button, 32, 32);
  gtk_widget_set_uposition (wcs_step_button, 32, 32);
  gtk_widget_set_usize (wcs_step_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_step_button, "Change step size", NULL);

  wcs_rot_inc_button = gtk_button_new_with_label ("+");
  gtk_widget_ref (wcs_rot_inc_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_rot_inc_button", wcs_rot_inc_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_rot_inc_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_rot_inc_button, 64, 72);
  gtk_widget_set_uposition (wcs_rot_inc_button, 64, 72);
  gtk_widget_set_usize (wcs_rot_inc_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_rot_inc_button, "Increase rotation", NULL);

  wcs_rot_dec_button = gtk_button_new_with_label ("-");
  gtk_widget_ref (wcs_rot_dec_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_rot_dec_button", wcs_rot_dec_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_rot_dec_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_rot_dec_button, 0, 72);
  gtk_widget_set_uposition (wcs_rot_dec_button, 0, 72);
  gtk_widget_set_usize (wcs_rot_dec_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_rot_dec_button, "Decrease rotation", NULL);

  wcs_north_button = gtk_button_new_with_label ("N");
  gtk_widget_ref (wcs_north_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_north_button", wcs_north_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_north_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_north_button, 32, 0);
  gtk_widget_set_uposition (wcs_north_button, 32, 0);
  gtk_widget_set_usize (wcs_north_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_north_button, "Move WCS north by amount of arcsec/pixel down", NULL);

  wcs_south_button = gtk_button_new_with_label ("S");
  gtk_widget_ref (wcs_south_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_south_button", wcs_south_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_south_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_south_button, 32, 64);
  gtk_widget_set_uposition (wcs_south_button, 32, 64);
  gtk_widget_set_usize (wcs_south_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_south_button, "Move WCS south by amount of arcsec/pixel down", NULL);

  wcs_west_button = gtk_button_new_with_label ("W");
  gtk_widget_ref (wcs_west_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_west_button", wcs_west_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_west_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_west_button, 64, 32);
  gtk_widget_set_uposition (wcs_west_button, 64, 32);
  gtk_widget_set_usize (wcs_west_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_west_button, "Move WCS west by amount of arcsec/pixel right", NULL);

  wcs_east_button = gtk_button_new_with_label ("E");
  gtk_widget_ref (wcs_east_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_east_button", wcs_east_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_east_button);
  gtk_fixed_put (GTK_FIXED (fixed1), wcs_east_button, 0, 32);
  gtk_widget_set_uposition (wcs_east_button, 0, 32);
  gtk_widget_set_usize (wcs_east_button, 24, 24);
  gtk_tooltips_set_tip (tooltips, wcs_east_button, "Move WCS east by amount of arcsec/pixel right", NULL);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (vbox9), hseparator1, TRUE, TRUE, 0);

  hbuttonbox4 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox4);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "hbuttonbox4", hbuttonbox4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox4);
  gtk_box_pack_start (GTK_BOX (vbox9), hbuttonbox4, TRUE, TRUE, 0);

  wcs_ok_button = gtk_button_new_with_label ("Update");
  gtk_widget_ref (wcs_ok_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_ok_button", wcs_ok_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox4), wcs_ok_button);
  GTK_WIDGET_SET_FLAGS (wcs_ok_button, GTK_CAN_DEFAULT);

  wcs_close_button = gtk_button_new_with_label ("Close");
  gtk_widget_ref (wcs_close_button);
  gtk_object_set_data_full (GTK_OBJECT (wcs_edit), "wcs_close_button", wcs_close_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (wcs_close_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox4), wcs_close_button);
  GTK_WIDGET_SET_FLAGS (wcs_close_button, GTK_CAN_DEFAULT);

  gtk_object_set_data (GTK_OBJECT (wcs_edit), "tooltips", tooltips);

  gtk_window_add_accel_group (GTK_WINDOW (wcs_edit), accel_group);

  return wcs_edit;
}

GtkWidget*
create_show_text (void)
{
  GtkWidget *show_text;
  GtkWidget *vbox13;
  GtkWidget *scrolledwindow1;
  GtkWidget *text1;
  GtkWidget *hbuttonbox7;
  GtkWidget *close_button;
  PangoFontDescription *font;

  show_text = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (show_text), "show_text", show_text);
  gtk_window_set_title (GTK_WINDOW (show_text), "FITS Header");
  gtk_window_set_position (GTK_WINDOW (show_text), GTK_WIN_POS_CENTER);

  vbox13 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox13);
  gtk_object_set_data_full (GTK_OBJECT (show_text), "vbox13", vbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox13);
  gtk_container_add (GTK_CONTAINER (show_text), vbox13);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (show_text), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox13), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  text1 = gtk_text_view_new();
  gtk_widget_ref (text1);
  gtk_object_set_data_full (GTK_OBJECT (show_text), "text1", text1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text1), GTK_WRAP_NONE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text1), FALSE);

  font = pango_font_description_from_string(P_STR(MONO_FONT));
  gtk_widget_modify_font(text1, font);

  gtk_widget_show (text1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), text1);

  hbuttonbox7 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox7);
  gtk_object_set_data_full (GTK_OBJECT (show_text), "hbuttonbox7", hbuttonbox7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox7);
  gtk_box_pack_start (GTK_BOX (vbox13), hbuttonbox7, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox7), GTK_BUTTONBOX_END);

  close_button = gtk_button_new_with_label ("Close");
  gtk_widget_ref (close_button);
  gtk_object_set_data_full (GTK_OBJECT (show_text), "close_button", close_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (close_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox7), close_button);
  GTK_WIDGET_SET_FLAGS (close_button, GTK_CAN_DEFAULT);

  return show_text;
}

GtkWidget*
create_create_recipe (void)
{
  GtkWidget *create_recipe;
  GtkWidget *vbox16;
  GtkWidget *table13;
  GtkWidget *var_checkb;
  GtkWidget *field_checkb;
  GtkWidget *user_checkb;
  GtkWidget *det_checkb;
  GtkWidget *label49;
  GtkWidget *convuser_checkb;
  GtkWidget *convdet_checkb;
  GtkWidget *convfield_checkb;
  GtkWidget *std_checkb;
  GtkWidget *convcat_checkb;
  GtkWidget *catalog_checkb;
  GtkWidget *label97;
  GtkWidget *label98;
  GtkWidget *label50;
  GtkWidget *tgt_entry;
  GtkWidget *seq_entry;
  GtkWidget *comments_entry;
  GtkWidget *hbox25;
  GtkWidget *recipe_file_entry;
  GtkWidget *browse_file_button;
  GtkWidget *off_frame_checkb;
  GtkWidget *hseparator3;
  GtkWidget *hbuttonbox9;
  GtkWidget *mkrcp_ok_button;
  GtkWidget *mkrcp_close_button;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  create_recipe = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (create_recipe), "create_recipe", create_recipe);
  gtk_window_set_title (GTK_WINDOW (create_recipe), "Create Recipe");
  gtk_window_set_position (GTK_WINDOW (create_recipe), GTK_WIN_POS_CENTER);

  vbox16 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox16);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "vbox16", vbox16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox16);
  gtk_container_add (GTK_CONTAINER (create_recipe), vbox16);
  gtk_container_set_border_width (GTK_CONTAINER (vbox16), 3);

  table13 = gtk_table_new (15, 2, FALSE);
  gtk_widget_ref (table13);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "table13", table13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table13);
  gtk_box_pack_start (GTK_BOX (vbox16), table13, FALSE, TRUE, 0);

  var_checkb = gtk_check_button_new_with_label ("Target stars");
  gtk_widget_ref (var_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "var_checkb", var_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (var_checkb);
  gtk_table_attach (GTK_TABLE (table13), var_checkb, 1, 2, 10, 11,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, var_checkb, "Save target stars to the recipe", NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (var_checkb), TRUE);

  field_checkb = gtk_check_button_new_with_label ("Field stars");
  gtk_widget_ref (field_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "field_checkb", field_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (field_checkb);
  gtk_table_attach (GTK_TABLE (table13), field_checkb, 1, 2, 12, 13,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, field_checkb, "Save field stars to the recipe", NULL);

  user_checkb = gtk_check_button_new_with_label ("User stars");
  gtk_widget_ref (user_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "user_checkb", user_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (user_checkb);
  gtk_table_attach (GTK_TABLE (table13), user_checkb, 1, 2, 13, 14,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, user_checkb, "Save user-selected stars to the recipe", NULL);

  det_checkb = gtk_check_button_new_with_label ("Detected stars");
  gtk_widget_ref (det_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "det_checkb", det_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (det_checkb);
  gtk_table_attach (GTK_TABLE (table13), det_checkb, 1, 2, 14, 15,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, det_checkb, "Save detected stars to the recipe", NULL);

  label49 = gtk_label_new ("File name:");
  gtk_widget_ref (label49);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "label49", label49,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label49);
  gtk_table_attach (GTK_TABLE (table13), label49, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label49), 0, 0.5);

  convuser_checkb = gtk_check_button_new_with_label ("Convert user stars to target");
  gtk_widget_ref (convuser_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "convuser_checkb", convuser_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (convuser_checkb);
  gtk_table_attach (GTK_TABLE (table13), convuser_checkb, 0, 1, 13, 14,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, convuser_checkb, "Create target stars in the recipe from the user-selected stars", NULL);

  convdet_checkb = gtk_check_button_new_with_label ("Convert detected stars to target");
  gtk_widget_ref (convdet_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "convdet_checkb", convdet_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (convdet_checkb);
  gtk_table_attach (GTK_TABLE (table13), convdet_checkb, 0, 1, 14, 15,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, convdet_checkb, "Create standard stars in the recipe from detected stars", NULL);

  convfield_checkb = gtk_check_button_new_with_label ("Convert field stars to target");
  gtk_widget_ref (convfield_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "convfield_checkb", convfield_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (convfield_checkb);
  gtk_table_attach (GTK_TABLE (table13), convfield_checkb, 0, 1, 12, 13,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, convfield_checkb, "Create targets in the recipe from the fields stars", NULL);

  std_checkb = gtk_check_button_new_with_label ("Standard stars");
  gtk_widget_ref (std_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "std_checkb", std_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (std_checkb);
  gtk_table_attach (GTK_TABLE (table13), std_checkb, 1, 2, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, std_checkb, "Save standard stars to the recipe", NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (std_checkb), TRUE);

  convcat_checkb = gtk_check_button_new_with_label ("Convert catalog objects to std");
  gtk_widget_ref (convcat_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "convcat_checkb", convcat_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (convcat_checkb);
  gtk_table_attach (GTK_TABLE (table13), convcat_checkb, 0, 1, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, convcat_checkb, "Set the catalog objects' type to standard when saving", NULL);

  catalog_checkb = gtk_check_button_new_with_label ("Catalog objects");
  gtk_widget_ref (catalog_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "catalog_checkb", catalog_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (catalog_checkb);
  gtk_table_attach (GTK_TABLE (table13), catalog_checkb, 1, 2, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, catalog_checkb, "Save catalog objects to the recipe", NULL);

  label97 = gtk_label_new ("Target object:");
  gtk_widget_ref (label97);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "label97", label97,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label97);
  gtk_table_attach (GTK_TABLE (table13), label97, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label97), 0, 0.5);

  label98 = gtk_label_new ("Sequence source:");
  gtk_widget_ref (label98);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "label98", label98,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label98);
  gtk_table_attach (GTK_TABLE (table13), label98, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label98), 0, 0.5);

  label50 = gtk_label_new ("Comments:");
  gtk_widget_ref (label50);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "label50", label50,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label50);
  gtk_table_attach (GTK_TABLE (table13), label50, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label50), 0, 1);

  tgt_entry = gtk_entry_new ();
  gtk_widget_ref (tgt_entry);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "tgt_entry", tgt_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (tgt_entry);
  gtk_table_attach (GTK_TABLE (table13), tgt_entry, 0, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, tgt_entry, "The name of the object that is the main target of the recipe, to be set in the recipe header", NULL);

  seq_entry = gtk_entry_new ();
  gtk_widget_ref (seq_entry);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "seq_entry", seq_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (seq_entry);
  gtk_table_attach (GTK_TABLE (table13), seq_entry, 0, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, seq_entry, "A description of where the sequnce information (std magnitudes) comes from", NULL);

  comments_entry = gtk_entry_new ();
  gtk_widget_ref (comments_entry);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "comments_entry", comments_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (comments_entry);
  gtk_table_attach (GTK_TABLE (table13), comments_entry, 0, 2, 7, 8,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, comments_entry, "Recipe comments", NULL);

  hbox25 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox25);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "hbox25", hbox25,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox25);
  gtk_table_attach (GTK_TABLE (table13), hbox25, 0, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  recipe_file_entry = gtk_entry_new ();
  gtk_widget_ref (recipe_file_entry);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "recipe_file_entry", recipe_file_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (recipe_file_entry);
  gtk_box_pack_start (GTK_BOX (hbox25), recipe_file_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, recipe_file_entry, "Recipe file name", NULL);

  browse_file_button = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (browse_file_button);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "browse_file_button", browse_file_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (browse_file_button);
  gtk_box_pack_start (GTK_BOX (hbox25), browse_file_button, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, browse_file_button, "Browse for file name", NULL);

  off_frame_checkb = gtk_check_button_new_with_label ("Include off-frame stars");
  gtk_widget_ref (off_frame_checkb);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "off_frame_checkb", off_frame_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (off_frame_checkb);
  gtk_table_attach (GTK_TABLE (table13), off_frame_checkb, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, off_frame_checkb, "If this option is cleared, stars outside the frame area are not included in the recipe", NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (off_frame_checkb), TRUE);

  hseparator3 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator3);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "hseparator3", hseparator3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator3);
  gtk_box_pack_start (GTK_BOX (vbox16), hseparator3, TRUE, FALSE, 0);

  hbuttonbox9 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox9);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "hbuttonbox9", hbuttonbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox9);
  gtk_box_pack_start (GTK_BOX (vbox16), hbuttonbox9, FALSE, FALSE, 0);

  mkrcp_ok_button = gtk_button_new_with_label ("OK");
  gtk_widget_ref (mkrcp_ok_button);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "mkrcp_ok_button", mkrcp_ok_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (mkrcp_ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox9), mkrcp_ok_button);
  GTK_WIDGET_SET_FLAGS (mkrcp_ok_button, GTK_CAN_DEFAULT);

  mkrcp_close_button = gtk_button_new_with_label ("Close");
  gtk_widget_ref (mkrcp_close_button);
  gtk_object_set_data_full (GTK_OBJECT (create_recipe), "mkrcp_close_button", mkrcp_close_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (mkrcp_close_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox9), mkrcp_close_button);
  GTK_WIDGET_SET_FLAGS (mkrcp_close_button, GTK_CAN_DEFAULT);

  gtk_widget_grab_focus (recipe_file_entry);
  gtk_widget_grab_default (mkrcp_ok_button);
  gtk_object_set_data (GTK_OBJECT (create_recipe), "tooltips", tooltips);

  return create_recipe;
}

GtkWidget*
create_yes_no (void)
{
  GtkWidget *yes_no;
  GtkWidget *vbox17;
  GtkWidget *scrolledwindow2;
  GtkWidget *viewport2;
  GtkWidget *vbox;
  GtkWidget *hbuttonbox10;
  GtkWidget *yes_button;
  GtkWidget *no_button;

  yes_no = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (yes_no), "yes_no", yes_no);
  gtk_window_set_title (GTK_WINDOW (yes_no), "gcx question");
  gtk_window_set_position (GTK_WINDOW (yes_no), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (yes_no), TRUE);

  vbox17 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox17);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "vbox17", vbox17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox17);
  gtk_container_add (GTK_CONTAINER (yes_no), vbox17);
  gtk_container_set_border_width (GTK_CONTAINER (vbox17), 5);

  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow2);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "scrolledwindow2", scrolledwindow2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow2);
  gtk_box_pack_start (GTK_BOX (vbox17), scrolledwindow2, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

  viewport2 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport2);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "viewport2", viewport2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport2);
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), viewport2);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "vbox", vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (viewport2), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbuttonbox10 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox10);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "hbuttonbox10", hbuttonbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox10);
  gtk_box_pack_start (GTK_BOX (vbox17), hbuttonbox10, FALSE, FALSE, 0);

  yes_button = gtk_button_new_with_label ("Yes");
  gtk_widget_ref (yes_button);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "yes_button", yes_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (yes_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox10), yes_button);
  GTK_WIDGET_SET_FLAGS (yes_button, GTK_CAN_DEFAULT);

  no_button = gtk_button_new_with_label ("No");
  gtk_widget_ref (no_button);
  gtk_object_set_data_full (GTK_OBJECT (yes_no), "no_button", no_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (no_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox10), no_button);
  GTK_WIDGET_SET_FLAGS (no_button, GTK_CAN_DEFAULT);

  gtk_widget_grab_focus (yes_button);
  gtk_widget_grab_default (yes_button);
  return yes_no;
}

GtkWidget*
create_image_processing (void)
{
  GtkWidget *image_processing;
  GtkWidget *notebook3;
  GtkWidget *vbox19;
  GtkWidget *top_hbox;
  GtkWidget *imf_info_label;
  GtkWidget *vpaned1;
  GtkWidget *frame13;
  GtkWidget *scrolledwindow;
  GtkWidget *viewport6;
  GtkWidget    *image_file_view;
  GtkListStore *image_file_list;
  GtkWidget *frame14;
  GtkWidget *scrolledwindow5;
  GtkWidget *processing_log_text;
  GtkWidget *label79;
  GtkWidget *table25;
  GtkWidget *frame3;
  GtkWidget *hbox15;
  GtkWidget *dark_entry;
  GtkWidget *dark_browse;
  GtkWidget *dark_checkb;
  GtkWidget *frame4;
  GtkWidget *hbox16;
  GtkWidget *flat_entry;
  GtkWidget *flat_browse;
  GtkWidget *flat_checkb;
  GtkWidget *frame5;
  GtkWidget *hbox17;
  GtkWidget *badpix_entry;
  GtkWidget *badpix_browse;
  GtkWidget *badpix_checkb;
  GtkWidget *frame7;
  GtkWidget *hbox19;
  GtkObject *mul_spin_adj;
  GtkWidget *mul_spin;
  GtkWidget *mul_checkb;
  GtkObject *add_spin_adj;
  GtkWidget *add_spin;
  GtkWidget *add_checkb;
  GtkWidget *frame15;
  GtkWidget *hbox28;
  GtkWidget *output_file_entry;
  GtkWidget *output_file_browse;
  GtkWidget *overwrite_checkb;
  GtkWidget *frame6;
  GtkWidget *hbox18;
  GtkWidget *align_entry;
  GtkWidget *align_browse;
  GtkWidget *align_checkb;
  GtkWidget *frame10;
  GtkWidget *hbox22;
  GtkObject *blur_spin_adj;
  GtkWidget *blur_spin;
  GtkWidget *label90;
  GtkWidget *blur_checkb;
  GtkWidget *frame11;
  GtkWidget *hbox23;
  GtkWidget *demosaic_method_optmenu;
  GtkWidget *demosaic_method_optmenu_menu;
  GtkWidget *demosaic_checkb;
  GtkWidget *frame8;
  GtkWidget *hbox20;
  GtkWidget *table26;
  GtkWidget *stack_method_optmenu;
  GtkWidget *stack_method_optmenu_menu;
  GtkObject *stack_sigmas_spin_adj;
  GtkWidget *stack_sigmas_spin;
  GtkObject *stack_iter_spin_adj;
  GtkWidget *stack_iter_spin;
  GSList *bgalign_group = NULL;
  GtkWidget *bg_match_off_rb;
  GtkWidget *bg_match_add_rb;
  GtkWidget *bg_match_mul_rb;
  GtkWidget *stack_checkb;
  GtkWidget *label83;
  GtkWidget *label82;
  GtkWidget *label106;
  GtkWidget *frame2;
  GtkWidget *hbox14;
  GtkWidget *bias_entry;
  GtkWidget *bias_browse;
  GtkWidget *bias_checkb;
  GtkWidget *gggg;
  GtkWidget *hbox27;
  GtkWidget *phot_reuse_wcs_checkb;
  GtkWidget *frame17;
  GtkWidget *hbox29;
  GtkWidget *recipe_entry;
  GtkWidget *recipe_browse;
  GtkWidget *phot_en_checkb;
  GtkWidget *label80;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  image_processing = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (image_processing), "image_processing", image_processing);
  gtk_window_set_title (GTK_WINDOW (image_processing), "CCD Reduction");
  gtk_window_set_position (GTK_WINDOW (image_processing), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (image_processing), 640, 420);

  notebook3 = gtk_notebook_new ();
  gtk_widget_ref (notebook3);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "notebook3", notebook3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook3);
  gtk_container_add (GTK_CONTAINER (image_processing), notebook3);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook3), GTK_POS_BOTTOM);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (notebook3), 16);

  vbox19 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox19);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "vbox19", vbox19,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox19);
  gtk_container_add (GTK_CONTAINER (notebook3), vbox19);

  top_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (top_hbox);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "top_hbox", top_hbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (top_hbox);
  gtk_box_pack_start (GTK_BOX (vbox19), top_hbox, FALSE, FALSE, 0);

  imf_info_label = gtk_label_new ("");
  gtk_widget_ref (imf_info_label);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "imf_info_label", imf_info_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (imf_info_label);
  gtk_box_pack_start (GTK_BOX (top_hbox), imf_info_label, FALSE, TRUE, 0);

  vpaned1 = gtk_vpaned_new ();
  gtk_widget_ref (vpaned1);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "vpaned1", vpaned1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vpaned1);
  gtk_box_pack_start (GTK_BOX (vbox19), vpaned1, TRUE, TRUE, 0);
  gtk_paned_set_position (GTK_PANED (vpaned1), 250);

  frame13 = gtk_frame_new ("Image Files");
  gtk_widget_ref (frame13);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame13", frame13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame13);
  gtk_paned_pack1 (GTK_PANED (vpaned1), frame13, FALSE, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (frame13), 3);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "scrolledwindow", scrolledwindow,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow);
  gtk_container_add (GTK_CONTAINER (frame13), scrolledwindow);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport6 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport6);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "viewport6", viewport6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport6);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport6);


  // filename, status, imf *
  image_file_list = gtk_list_store_new (IMFL_COL_SIZE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
  image_file_view = gtk_tree_view_new_with_model ( GTK_TREE_MODEL(image_file_list));
  gtk_widget_ref (image_file_view);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "image_file_view", image_file_view,
                            (GtkDestroyNotify) gtk_widget_unref);
  g_object_set_data_full (G_OBJECT (image_processing), "image_file_list", image_file_list,
			  g_object_unref);
  gtk_widget_show (image_file_view);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(image_file_view), FALSE);
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(image_file_view), FALSE);

  GtkCellRenderer *render = gtk_cell_renderer_text_new();

  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(image_file_view),
					      -1,
					      "File", render, "text", IMFL_COL_FILENAME,
					      NULL);

  render = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(image_file_view),
					      -1,
					      "Status", render, "text", IMFL_COL_STATUS,
					      NULL);

  gtk_container_add (GTK_CONTAINER (viewport6), image_file_view);

  GtkTreeSelection *treesel = gtk_tree_view_get_selection (GTK_TREE_VIEW(image_file_view));
  gtk_tree_selection_set_mode (treesel, GTK_SELECTION_MULTIPLE);

  frame14 = gtk_frame_new ("Log output");
  gtk_widget_ref (frame14);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame14", frame14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame14);
  gtk_paned_pack2 (GTK_PANED (vpaned1), frame14, TRUE, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (frame14), 3);

  scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow5);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "scrolledwindow5", scrolledwindow5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow5);
  gtk_container_add (GTK_CONTAINER (frame14), scrolledwindow5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  processing_log_text = gtk_text_view_new ();
  gtk_widget_ref (processing_log_text);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "processing_log_text", processing_log_text,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (processing_log_text);
  gtk_container_add (GTK_CONTAINER (scrolledwindow5), processing_log_text);
  gtk_tooltips_set_tip (tooltips, processing_log_text, "reduction log window", NULL);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(processing_log_text), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(processing_log_text), FALSE);

  label79 = gtk_label_new ("Reduce");
  gtk_widget_ref (label79);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "label79", label79,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label79);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook3), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook3), 0), label79);

  table25 = gtk_table_new (8, 2, TRUE);
  gtk_widget_ref (table25);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "table25", table25,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table25);
  gtk_container_add (GTK_CONTAINER (notebook3), table25);
  gtk_table_set_col_spacings (GTK_TABLE (table25), 12);

  frame3 = gtk_frame_new ("Dark frame");
  gtk_widget_ref (frame3);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame3", frame3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame3);
  gtk_table_attach (GTK_TABLE (table25), frame3, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame3), 3);

  hbox15 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox15);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox15", hbox15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox15);
  gtk_container_add (GTK_CONTAINER (frame3), hbox15);

  dark_entry = gtk_entry_new ();
  gtk_widget_ref (dark_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "dark_entry", dark_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dark_entry);
  gtk_box_pack_start (GTK_BOX (hbox15), dark_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, dark_entry, "Dark frame file name", NULL);

  dark_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (dark_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "dark_browse", dark_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dark_browse);
  gtk_box_pack_start (GTK_BOX (hbox15), dark_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dark_browse), 2);
  gtk_tooltips_set_tip (tooltips, dark_browse, "Browse", NULL);

  dark_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (dark_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "dark_checkb", dark_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (dark_checkb);
  gtk_box_pack_start (GTK_BOX (hbox15), dark_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, dark_checkb, "Enable dark subtraction step", NULL);

  frame4 = gtk_frame_new ("Flat frame");
  gtk_widget_ref (frame4);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame4", frame4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame4);
  gtk_table_attach (GTK_TABLE (table25), frame4, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame4), 3);

  hbox16 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox16);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox16", hbox16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox16);
  gtk_container_add (GTK_CONTAINER (frame4), hbox16);

  flat_entry = gtk_entry_new ();
  gtk_widget_ref (flat_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "flat_entry", flat_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (flat_entry);
  gtk_box_pack_start (GTK_BOX (hbox16), flat_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, flat_entry, "Flat field file name", NULL);

  flat_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (flat_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "flat_browse", flat_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (flat_browse);
  gtk_box_pack_start (GTK_BOX (hbox16), flat_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (flat_browse), 2);
  gtk_tooltips_set_tip (tooltips, flat_browse, "Browse", NULL);

  flat_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (flat_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "flat_checkb", flat_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (flat_checkb);
  gtk_box_pack_start (GTK_BOX (hbox16), flat_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, flat_checkb, "Enable flat fielding step", NULL);

  frame5 = gtk_frame_new ("Bad pixel file");
  gtk_widget_ref (frame5);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame5", frame5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame5);
  gtk_table_attach (GTK_TABLE (table25), frame5, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 3);

  hbox17 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox17);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox17", hbox17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox17);
  gtk_container_add (GTK_CONTAINER (frame5), hbox17);

  badpix_entry = gtk_entry_new ();
  gtk_widget_ref (badpix_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "badpix_entry", badpix_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (badpix_entry);
  gtk_box_pack_start (GTK_BOX (hbox17), badpix_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, badpix_entry, "bad pixel map file name", NULL);

  badpix_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (badpix_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "badpix_browse", badpix_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (badpix_browse);
  gtk_box_pack_start (GTK_BOX (hbox17), badpix_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (badpix_browse), 2);
  gtk_tooltips_set_tip (tooltips, badpix_browse, "Browse", NULL);

  badpix_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (badpix_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "badpix_checkb", badpix_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (badpix_checkb);
  gtk_box_pack_start (GTK_BOX (hbox17), badpix_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, badpix_checkb, "Enable bad pixel correction step", NULL);

  frame7 = gtk_frame_new ("Multiply/Add");
  gtk_widget_ref (frame7);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame7", frame7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame7);
  gtk_table_attach (GTK_TABLE (table25), frame7, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame7), 3);

  hbox19 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox19);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox19", hbox19,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox19);
  gtk_container_add (GTK_CONTAINER (frame7), hbox19);

  mul_spin_adj = gtk_adjustment_new (1, 0, 100, 0.1, 10, 0);
  mul_spin = gtk_spin_button_new (GTK_ADJUSTMENT (mul_spin_adj), 1, 3);
  gtk_widget_ref (mul_spin);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "mul_spin", mul_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (mul_spin);
  gtk_box_pack_start (GTK_BOX (hbox19), mul_spin, TRUE, TRUE, 2);
  gtk_tooltips_set_tip (tooltips, mul_spin, "Amount by which the pixel values are multiplied", NULL);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (mul_spin), TRUE);

  mul_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (mul_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "mul_checkb", mul_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (mul_checkb);
  gtk_box_pack_start (GTK_BOX (hbox19), mul_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, mul_checkb, "Enable multiplication by a constant", NULL);

  add_spin_adj = gtk_adjustment_new (0, -10000, 10000, 1, 10, 0);
  add_spin = gtk_spin_button_new (GTK_ADJUSTMENT (add_spin_adj), 1, 1);
  gtk_widget_ref (add_spin);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "add_spin", add_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (add_spin);
  gtk_box_pack_start (GTK_BOX (hbox19), add_spin, TRUE, TRUE, 2);
  gtk_tooltips_set_tip (tooltips, add_spin, "Amount added to pixel values", NULL);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (add_spin), TRUE);

  add_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (add_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "add_checkb", add_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (add_checkb);
  gtk_box_pack_start (GTK_BOX (hbox19), add_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, add_checkb, "Enable constant addition", NULL);

  frame15 = gtk_frame_new ("Output file/directory");
  gtk_widget_ref (frame15);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame15", frame15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame15);
  gtk_table_attach (GTK_TABLE (table25), frame15, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hbox28 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox28);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox28", hbox28,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox28);
  gtk_container_add (GTK_CONTAINER (frame15), hbox28);

  output_file_entry = gtk_entry_new ();
  gtk_widget_ref (output_file_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "output_file_entry", output_file_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (output_file_entry);
  gtk_box_pack_start (GTK_BOX (hbox28), output_file_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, output_file_entry, "Output file name, file name stud or directory name", NULL);

  output_file_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (output_file_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "output_file_browse", output_file_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (output_file_browse);
  gtk_box_pack_start (GTK_BOX (hbox28), output_file_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (output_file_browse), 3);
  gtk_tooltips_set_tip (tooltips, output_file_browse, "Browse", NULL);

  overwrite_checkb = gtk_check_button_new_with_label ("Overwrite");
  gtk_widget_ref (overwrite_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "overwrite_checkb", overwrite_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (hbox28), overwrite_checkb, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (overwrite_checkb), 3);
  gtk_widget_set_sensitive (overwrite_checkb, FALSE);
  gtk_tooltips_set_tip (tooltips, overwrite_checkb, "Save processed frames over the original ones", NULL);

//Demosaic
  frame11 = gtk_frame_new ("Demosaic");
  gtk_widget_ref (frame11);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame11", frame11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame11);
  gtk_table_attach (GTK_TABLE (table25), frame11, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame11), 3);

  hbox23 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox23);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox23", hbox23,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox23);
  gtk_container_add (GTK_CONTAINER (frame11), hbox23);

  demosaic_method_optmenu = gtk_option_menu_new ();
  gtk_widget_ref (demosaic_method_optmenu);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "demosaic_method_optmenu", demosaic_method_optmenu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (demosaic_method_optmenu);
  gtk_box_pack_start (GTK_BOX (hbox23), demosaic_method_optmenu, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, demosaic_method_optmenu, "Demosaic method (when applying Bayer matrix)", NULL);
  demosaic_method_optmenu_menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (demosaic_method_optmenu), demosaic_method_optmenu_menu);

  demosaic_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (demosaic_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "demosaic_checkb", demosaic_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (demosaic_checkb);
  gtk_box_pack_start (GTK_BOX (hbox23), demosaic_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, demosaic_checkb, "Apply Bayer matrix", NULL);
//// End Demosaic

  frame6 = gtk_frame_new ("Alignment reference frame");
  gtk_widget_ref (frame6);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame6", frame6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame6);
  gtk_table_attach (GTK_TABLE (table25), frame6, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame6), 3);

  hbox18 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox18);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox18", hbox18,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox18);
  gtk_container_add (GTK_CONTAINER (frame6), hbox18);

  align_entry = gtk_entry_new ();
  gtk_widget_ref (align_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "align_entry", align_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (align_entry);
  gtk_box_pack_start (GTK_BOX (hbox18), align_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, align_entry, "Reference frame for alignment", NULL);

  align_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (align_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "align_browse", align_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (align_browse);
  gtk_box_pack_start (GTK_BOX (hbox18), align_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (align_browse), 2);
  gtk_tooltips_set_tip (tooltips, align_browse, "Browse", NULL);

  align_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (align_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "align_checkb", align_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (align_checkb);
  gtk_box_pack_start (GTK_BOX (hbox18), align_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, align_checkb, "Enable image alignment", NULL);

  frame10 = gtk_frame_new ("Gaussian Smoothing");
  gtk_widget_ref (frame10);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame10", frame10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame10);
  gtk_table_attach (GTK_TABLE (table25), frame10, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame10), 3);

  hbox22 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox22);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox22", hbox22,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox22);
  gtk_container_add (GTK_CONTAINER (frame10), hbox22);

  blur_spin_adj = gtk_adjustment_new (0.5, 0, 50, 0.1, 10, 0);
  blur_spin = gtk_spin_button_new (GTK_ADJUSTMENT (blur_spin_adj), 1, 1);
  gtk_widget_ref (blur_spin);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "blur_spin", blur_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (blur_spin);
  gtk_box_pack_start (GTK_BOX (hbox22), blur_spin, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, blur_spin, "FWHM of gaussin smoothing function (in pixels)", NULL);

  label90 = gtk_label_new ("FWHM           ");
  gtk_widget_ref (label90);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "label90", label90,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label90);
  gtk_box_pack_start (GTK_BOX (hbox22), label90, FALSE, FALSE, 0);
  gtk_misc_set_padding (GTK_MISC (label90), 3, 0);

  blur_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (blur_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "blur_checkb", blur_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (blur_checkb);
  gtk_box_pack_start (GTK_BOX (hbox22), blur_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, blur_checkb, "Smooth image after alignment", NULL);

  frame8 = gtk_frame_new ("Stacking");
  gtk_widget_ref (frame8);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame8", frame8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame8);
  gtk_table_attach (GTK_TABLE (table25), frame8, 1, 2, 3, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame8), 3);

  hbox20 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox20);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox20", hbox20,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox20);
  gtk_container_add (GTK_CONTAINER (frame8), hbox20);

  table26 = gtk_table_new (5, 2, TRUE);
  gtk_widget_ref (table26);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "table26", table26,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table26);
  gtk_box_pack_start (GTK_BOX (hbox20), table26, TRUE, TRUE, 3);
  gtk_table_set_col_spacings (GTK_TABLE (table26), 6);

  stack_method_optmenu = gtk_option_menu_new ();
  gtk_widget_ref (stack_method_optmenu);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "stack_method_optmenu", stack_method_optmenu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (stack_method_optmenu);
  gtk_table_attach (GTK_TABLE (table26), stack_method_optmenu, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, stack_method_optmenu, "Stacking method", NULL);
  stack_method_optmenu_menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (stack_method_optmenu), stack_method_optmenu_menu);

  stack_sigmas_spin_adj = gtk_adjustment_new (10, 0.5, 10, 0.25, 10, 0);
  stack_sigmas_spin = gtk_spin_button_new (GTK_ADJUSTMENT (stack_sigmas_spin_adj), 1, 1);
  gtk_widget_ref (stack_sigmas_spin);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "stack_sigmas_spin", stack_sigmas_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (stack_sigmas_spin);
  gtk_table_attach (GTK_TABLE (table26), stack_sigmas_spin, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, stack_sigmas_spin, "Acceptance band width for mean-median, kappa-sigma and avsigclip", NULL);

  stack_iter_spin_adj = gtk_adjustment_new (1, 1, 10, 1, 10, 0);
  stack_iter_spin = gtk_spin_button_new (GTK_ADJUSTMENT (stack_iter_spin_adj), 1, 0);
  gtk_widget_ref (stack_iter_spin);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "stack_iter_spin", stack_iter_spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (stack_iter_spin);
  gtk_table_attach (GTK_TABLE (table26), stack_iter_spin, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, stack_iter_spin, "Max number of iterations for kappa-sigma and avgsigclip", NULL);

  bg_match_off_rb = gtk_radio_button_new_with_label (bgalign_group, "Off");
  bgalign_group = gtk_radio_button_group (GTK_RADIO_BUTTON (bg_match_off_rb));
  gtk_widget_ref (bg_match_off_rb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "bg_match_off_rb", bg_match_off_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bg_match_off_rb);
  gtk_table_attach (GTK_TABLE (table26), bg_match_off_rb, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, bg_match_off_rb, "Disable background matching", NULL);

  bg_match_add_rb = gtk_radio_button_new_with_label (bgalign_group, "Additive");
  bgalign_group = gtk_radio_button_group (GTK_RADIO_BUTTON (bg_match_add_rb));
  gtk_widget_ref (bg_match_add_rb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "bg_match_add_rb", bg_match_add_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bg_match_add_rb);
  gtk_table_attach (GTK_TABLE (table26), bg_match_add_rb, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, bg_match_add_rb, "Match background of frames before stacking by adding a bias", NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bg_match_add_rb), TRUE);

  bg_match_mul_rb = gtk_radio_button_new_with_label (bgalign_group, "Multiplicative");
  bgalign_group = gtk_radio_button_group (GTK_RADIO_BUTTON (bg_match_mul_rb));
  gtk_widget_ref (bg_match_mul_rb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "bg_match_mul_rb", bg_match_mul_rb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bg_match_mul_rb);
  gtk_table_attach (GTK_TABLE (table26), bg_match_mul_rb, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, bg_match_mul_rb, "Match background of frames by scaling", NULL);

  stack_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (stack_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "stack_checkb", stack_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (stack_checkb);
  gtk_table_attach (GTK_TABLE (table26), stack_checkb, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, stack_checkb, "Enable frame stacking", NULL);

  label83 = gtk_label_new ("Iteration Limit");
  gtk_widget_ref (label83);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "label83", label83,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label83);
  gtk_table_attach (GTK_TABLE (table26), label83, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label83), 0, 1);

  label82 = gtk_label_new ("Sigmas ");
  gtk_widget_ref (label82);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "label82", label82,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label82);
  gtk_table_attach (GTK_TABLE (table26), label82, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label82), 0, 1);

  label106 = gtk_label_new ("Bg matching");
  gtk_widget_ref (label106);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "label106", label106,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label106);
  gtk_table_attach (GTK_TABLE (table26), label106, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label106), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label106), 0, 1);
  gtk_misc_set_padding (GTK_MISC (label106), 3, 0);

  frame2 = gtk_frame_new ("Bias frame");
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame2", frame2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_table_attach (GTK_TABLE (table25), frame2, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 3);

  hbox14 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox14);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox14", hbox14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox14);
  gtk_container_add (GTK_CONTAINER (frame2), hbox14);

  bias_entry = gtk_entry_new ();
  gtk_widget_ref (bias_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "bias_entry", bias_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bias_entry);
  gtk_box_pack_start (GTK_BOX (hbox14), bias_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, bias_entry, "Bias file name", NULL);

  bias_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (bias_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "bias_browse", bias_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bias_browse);
  gtk_box_pack_start (GTK_BOX (hbox14), bias_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bias_browse), 2);
  gtk_tooltips_set_tip (tooltips, bias_browse, "Browse", NULL);

  bias_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (bias_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "bias_checkb", bias_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bias_checkb);
  gtk_box_pack_start (GTK_BOX (hbox14), bias_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, bias_checkb, "Enable bias subtraction step", NULL);

  gggg = gtk_frame_new ("Photometry options");
  gtk_widget_ref (gggg);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "gggg", gggg,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (gggg);
  gtk_table_attach (GTK_TABLE (table25), gggg, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (gggg), 3);

  hbox27 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox27);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox27", hbox27,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox27);
  gtk_container_add (GTK_CONTAINER (gggg), hbox27);

  phot_reuse_wcs_checkb = gtk_check_button_new_with_label ("Reuse WCS");
  gtk_widget_ref (phot_reuse_wcs_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "phot_reuse_wcs_checkb", phot_reuse_wcs_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phot_reuse_wcs_checkb);
  gtk_box_pack_start (GTK_BOX (hbox27), phot_reuse_wcs_checkb, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, phot_reuse_wcs_checkb, "Use the current WCS from the image window rather than fitting it for each frame", NULL);

  frame17 = gtk_frame_new ("Photometry recipe file");
  gtk_widget_ref (frame17);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "frame17", frame17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame17);
  gtk_table_attach (GTK_TABLE (table25), frame17, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame17), 3);

  hbox29 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox29);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "hbox29", hbox29,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox29);
  gtk_container_add (GTK_CONTAINER (frame17), hbox29);

  recipe_entry = gtk_entry_new ();
  gtk_widget_ref (recipe_entry);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "recipe_entry", recipe_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (recipe_entry);
  gtk_box_pack_start (GTK_BOX (hbox29), recipe_entry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, recipe_entry, "Name of recipe file. Enter _TYCHO_ to generate a recipe on-the-fly using Tycho2 data, _OBJECT_ to search the rcp path for <object>.rcp or _AUTO_ to try first by object name, and if that fails create a Tycho recipe", NULL);

  recipe_browse = gtk_button_new_with_label (" ... ");
  gtk_widget_ref (recipe_browse);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "recipe_browse", recipe_browse,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (recipe_browse);
  gtk_box_pack_start (GTK_BOX (hbox29), recipe_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (recipe_browse), 2);
  gtk_tooltips_set_tip (tooltips, recipe_browse, "Browse", NULL);

  phot_en_checkb = gtk_check_button_new_with_label ("Enable");
  gtk_widget_ref (phot_en_checkb);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "phot_en_checkb", phot_en_checkb,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phot_en_checkb);
  gtk_box_pack_start (GTK_BOX (hbox29), phot_en_checkb, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, phot_en_checkb, "Enable photometry of frames", NULL);

  label80 = gtk_label_new ("Setup");
  gtk_widget_ref (label80);
  gtk_object_set_data_full (GTK_OBJECT (image_processing), "label80", label80,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label80);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook3), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook3), 1), label80);

  gtk_object_set_data (GTK_OBJECT (image_processing), "tooltips", tooltips);

  return image_processing;
}


GtkWidget*
create_par_edit (void)
{
  GtkWidget *par_edit;
  GtkWidget *vbox27;
  GtkWidget *hpaned1;
  GtkWidget *scrolledwindow6;
  GtkWidget *viewport4;
  GtkWidget *vbox29;
  GtkWidget *par_title_label;
  GtkWidget *par_descr_label;
  GtkWidget *par_type_label;
  GtkWidget *par_combo;
  GtkWidget *par_combo_entry;
  GtkWidget *par_status_label;
  GtkWidget *par_fname_label;
  GtkWidget *hbuttonbox16;
  GtkWidget *par_save;
  GtkWidget *par_default;
  GtkWidget *par_load;
  GtkWidget *par_close;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  par_edit = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (par_edit), "par_edit", par_edit);
  gtk_container_set_border_width (GTK_CONTAINER (par_edit), 3);
  gtk_window_set_title (GTK_WINDOW (par_edit), "Gcx Options");
  gtk_window_set_position (GTK_WINDOW (par_edit), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (par_edit), 600, 400);

  vbox27 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox27);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "vbox27", vbox27,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox27);
  gtk_container_add (GTK_CONTAINER (par_edit), vbox27);

  hpaned1 = gtk_hpaned_new ();
  gtk_widget_ref (hpaned1);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "hpaned1", hpaned1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hpaned1);
  gtk_box_pack_start (GTK_BOX (vbox27), hpaned1, TRUE, TRUE, 0);
  gtk_paned_set_position (GTK_PANED (hpaned1), 292);

  scrolledwindow6 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow6);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "scrolledwindow6", scrolledwindow6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow6);
  gtk_paned_pack1 (GTK_PANED (hpaned1), scrolledwindow6, FALSE, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  viewport4 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport4);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "viewport4", viewport4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport4);
  gtk_container_add (GTK_CONTAINER (scrolledwindow6), viewport4);

  vbox29 = gtk_vbox_new (FALSE, 6);
  gtk_widget_ref (vbox29);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "vbox29", vbox29,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox29);
  gtk_paned_pack2 (GTK_PANED (hpaned1), vbox29, TRUE, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (vbox29), 3);

  par_title_label = gtk_label_new ("Select an Option");
  gtk_widget_ref (par_title_label);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_title_label", par_title_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_title_label);
  gtk_box_pack_start (GTK_BOX (vbox29), par_title_label, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (par_title_label), TRUE);

  par_descr_label = gtk_label_new (" ");
  gtk_widget_ref (par_descr_label);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_descr_label", par_descr_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_descr_label);
  gtk_box_pack_start (GTK_BOX (vbox29), par_descr_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (par_descr_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (par_descr_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (par_descr_label), 7.45058e-09, 0.5);

  par_type_label = gtk_label_new (" ");
  gtk_widget_ref (par_type_label);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_type_label", par_type_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_type_label);
  gtk_box_pack_start (GTK_BOX (vbox29), par_type_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (par_type_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (par_type_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (par_type_label), 7.45058e-09, 0.5);

  par_combo = gtk_combo_new ();
  gtk_widget_ref (par_combo);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_combo", par_combo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (vbox29), par_combo, FALSE, FALSE, 0);

  par_combo_entry = GTK_COMBO (par_combo)->entry;
  gtk_widget_ref (par_combo_entry);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_combo_entry", par_combo_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_combo_entry);

  par_status_label = gtk_label_new (" ");
  gtk_widget_ref (par_status_label);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_status_label", par_status_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_status_label);
  gtk_box_pack_start (GTK_BOX (vbox29), par_status_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (par_status_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (par_status_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (par_status_label), 7.45058e-09, 0.5);

  par_fname_label = gtk_label_new ("");
  gtk_widget_ref (par_fname_label);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_fname_label", par_fname_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_fname_label);
  gtk_box_pack_start (GTK_BOX (vbox29), par_fname_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (par_fname_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (par_fname_label), 0, 0.5);

  hbuttonbox16 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox16);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "hbuttonbox16", hbuttonbox16,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox16);
  gtk_box_pack_start (GTK_BOX (vbox27), hbuttonbox16, FALSE, TRUE, 0);

  par_save = gtk_button_new_with_label ("Save");
  gtk_widget_ref (par_save);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_save", par_save,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_save);
  gtk_container_add (GTK_CONTAINER (hbuttonbox16), par_save);
  GTK_WIDGET_SET_FLAGS (par_save, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, par_save, "Save parameters to ~/.gcxrc", NULL);

  par_default = gtk_button_new_with_label ("Default");
  gtk_widget_ref (par_default);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_default", par_default,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_default);
  gtk_container_add (GTK_CONTAINER (hbuttonbox16), par_default);
  GTK_WIDGET_SET_FLAGS (par_default, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, par_default, "Restore default value of parameter", NULL);

  par_load = gtk_button_new_with_label ("Reload");
  gtk_widget_ref (par_load);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_load", par_load,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_load);
  gtk_container_add (GTK_CONTAINER (hbuttonbox16), par_load);
  GTK_WIDGET_SET_FLAGS (par_load, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, par_load, "Reload options file", NULL);

  par_close = gtk_button_new_with_label ("Close");
  gtk_widget_ref (par_close);
  gtk_object_set_data_full (GTK_OBJECT (par_edit), "par_close", par_close,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (par_close);
  gtk_container_add (GTK_CONTAINER (hbuttonbox16), par_close);
  GTK_WIDGET_SET_FLAGS (par_close, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip (tooltips, par_close, "Close Dialog", NULL);

  gtk_object_set_data (GTK_OBJECT (par_edit), "tooltips", tooltips);

  return par_edit;
}

GtkWidget*
create_entry_prompt (void)
{
  GtkWidget *entry_prompt;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox30;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbuttonbox17;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  entry_prompt = gtk_dialog_new ();
  gtk_object_set_data (GTK_OBJECT (entry_prompt), "entry_prompt", entry_prompt);
  gtk_window_set_title (GTK_WINDOW (entry_prompt), "gcx prompt");
  GTK_WINDOW (entry_prompt)->type = GTK_WINDOW_TOPLEVEL;
  gtk_window_set_position (GTK_WINDOW (entry_prompt), GTK_WIN_POS_MOUSE);
  gtk_window_set_modal (GTK_WINDOW (entry_prompt), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (entry_prompt), 400, 150);
  gtk_window_set_policy (GTK_WINDOW (entry_prompt), TRUE, TRUE, FALSE);

  dialog_vbox1 = GTK_DIALOG (entry_prompt)->vbox;
  gtk_object_set_data (GTK_OBJECT (entry_prompt), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  vbox30 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox30);
  gtk_object_set_data_full (GTK_OBJECT (entry_prompt), "vbox30", vbox30,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox30);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox30, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox30), 6);

  label = gtk_label_new ("Enter value");
  gtk_widget_ref (label);
  gtk_object_set_data_full (GTK_OBJECT (entry_prompt), "label", label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox30), label, TRUE, TRUE, 0);
  gtk_misc_set_padding (GTK_MISC (label), 6, 13);

  entry = gtk_entry_new ();
  gtk_widget_ref (entry);
  gtk_object_set_data_full (GTK_OBJECT (entry_prompt), "entry", entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (vbox30), entry, FALSE, FALSE, 0);

  dialog_action_area1 = GTK_DIALOG (entry_prompt)->action_area;
  gtk_object_set_data (GTK_OBJECT (entry_prompt), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);

  hbuttonbox17 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox17);
  gtk_object_set_data_full (GTK_OBJECT (entry_prompt), "hbuttonbox17", hbuttonbox17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox17);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbuttonbox17, TRUE, TRUE, 0);

  ok_button = gtk_button_new_with_label ("OK");
  gtk_widget_ref (ok_button);
  gtk_object_set_data_full (GTK_OBJECT (entry_prompt), "ok_button", ok_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox17), ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (ok_button, "clicked", accel_group,
                              GDK_Return, 0,
                              GTK_ACCEL_VISIBLE);

  cancel_button = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (cancel_button);
  gtk_object_set_data_full (GTK_OBJECT (entry_prompt), "cancel_button", cancel_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox17), cancel_button);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

  gtk_widget_grab_focus (entry);
  gtk_widget_grab_default (ok_button);
  gtk_window_add_accel_group (GTK_WINDOW (entry_prompt), accel_group);

  return entry_prompt;
}

GtkWidget*
create_guide_window (void)
{
  GtkWidget *guide_window;
  GtkWidget *guide_vbox;
  GtkWidget *hbox24;
  GtkWidget *scrolled_window;
  GtkWidget *viewport5;
  GtkWidget *image_alignment;
  GtkWidget *image;
  GtkWidget *vbox32;
  guint guide_run_key;
  GtkWidget *guide_run;
  guint guide_dark_key;
  GtkWidget *guide_dark;
  guint guide_find_star_key;
  GtkWidget *guide_find_star;
  guint guide_calibrate_key;
  GtkWidget *guide_calibrate;
  GtkWidget *hseparator8;
  GtkWidget *label92;
  GtkWidget *guide_exp_combo;
  GList *guide_exp_combo_items = NULL;
  GtkWidget *guide_exp_combo_entry;
  GtkWidget *guide_options;
  GtkWidget *alignment1;
  GtkWidget *guide_box_darea;
  GtkWidget *statuslabel2;
  GtkAccelGroup *accel_group;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  accel_group = gtk_accel_group_new ();

  guide_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (guide_window), "guide_window", guide_window);
  gtk_window_set_title (GTK_WINDOW (guide_window), "Guiding");
  gtk_window_set_default_size (GTK_WINDOW (guide_window), 640, 400);

  guide_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (guide_vbox);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_vbox", guide_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_vbox);
  gtk_container_add (GTK_CONTAINER (guide_window), guide_vbox);

  hbox24 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox24);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "hbox24", hbox24,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox24);
  gtk_box_pack_start (GTK_BOX (guide_vbox), hbox24, TRUE, TRUE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolled_window);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "scrolled_window", scrolled_window,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (hbox24), scrolled_window, TRUE, TRUE, 0);

  viewport5 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport5);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "viewport5", viewport5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport5);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport5);

  image_alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_widget_ref (image_alignment);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "image_alignment", image_alignment,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (image_alignment);
  gtk_container_add (GTK_CONTAINER (viewport5), image_alignment);

  image = gtk_drawing_area_new ();
  gtk_widget_ref (image);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "image", image,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (image_alignment), image);

  vbox32 = gtk_vbox_new (FALSE, 3);
  gtk_widget_ref (vbox32);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "vbox32", vbox32,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox32);
  gtk_box_pack_start (GTK_BOX (hbox24), vbox32, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox32), 12);

  guide_run = gtk_toggle_button_new_with_label ("");
  guide_run_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (guide_run)->child),
                                   "_Run");
  gtk_widget_add_accelerator (guide_run, "clicked", accel_group,
                              guide_run_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  gtk_widget_ref (guide_run);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_run", guide_run,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_run);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_run, FALSE, FALSE, 0);

  guide_dark = gtk_button_new_with_label ("");
  guide_dark_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (guide_dark)->child),
                                   "_Dark Frame");
  gtk_widget_add_accelerator (guide_dark, "clicked", accel_group,
                              guide_dark_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  gtk_widget_ref (guide_dark);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_dark", guide_dark,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_dark);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_dark, FALSE, FALSE, 0);

  guide_find_star = gtk_button_new_with_label ("");
  guide_find_star_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (guide_find_star)->child),
                                   "_Guide Star");
  gtk_widget_add_accelerator (guide_find_star, "clicked", accel_group,
                              guide_find_star_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  gtk_widget_ref (guide_find_star);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_find_star", guide_find_star,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_find_star);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_find_star, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, guide_find_star, "Set one of the user-selected stars as a guide star; if no user selected stars exist, search for a suitable star", NULL);

  guide_calibrate = gtk_toggle_button_new_with_label ("");
  guide_calibrate_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (guide_calibrate)->child),
                                   "_Calibrate");
  gtk_widget_add_accelerator (guide_calibrate, "clicked", accel_group,
                              guide_calibrate_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  gtk_widget_ref (guide_calibrate);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_calibrate", guide_calibrate,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_calibrate);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_calibrate, FALSE, FALSE, 0);

  hseparator8 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator8);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "hseparator8", hseparator8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator8);
  gtk_box_pack_start (GTK_BOX (vbox32), hseparator8, FALSE, TRUE, 3);

  label92 = gtk_label_new ("Exposure");
  gtk_widget_ref (label92);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "label92", label92,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label92);
  gtk_box_pack_start (GTK_BOX (vbox32), label92, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label92), 7.45058e-09, 1);

  guide_exp_combo = gtk_combo_new ();
  gtk_widget_ref (guide_exp_combo);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_exp_combo", guide_exp_combo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_exp_combo);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_exp_combo, FALSE, FALSE, 0);
  guide_exp_combo_items = g_list_append (guide_exp_combo_items, (gpointer) "1/8");
  guide_exp_combo_items = g_list_append (guide_exp_combo_items, (gpointer) "1/4");
  guide_exp_combo_items = g_list_append (guide_exp_combo_items, (gpointer) "1/2");
  guide_exp_combo_items = g_list_append (guide_exp_combo_items, (gpointer) "1");
  guide_exp_combo_items = g_list_append (guide_exp_combo_items, (gpointer) "2");
  guide_exp_combo_items = g_list_append (guide_exp_combo_items, (gpointer) "4");
  gtk_combo_set_popdown_strings (GTK_COMBO (guide_exp_combo), guide_exp_combo_items);
  g_list_free (guide_exp_combo_items);

  guide_exp_combo_entry = GTK_COMBO (guide_exp_combo)->entry;
  gtk_widget_ref (guide_exp_combo_entry);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_exp_combo_entry", guide_exp_combo_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_exp_combo_entry);
  gtk_widget_set_usize (guide_exp_combo_entry, 64, -2);
  gtk_entry_set_text (GTK_ENTRY (guide_exp_combo_entry), "2");

  guide_options = gtk_button_new_with_label ("Options");
  gtk_widget_ref (guide_options);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_options", guide_options,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_options);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_options, FALSE, FALSE, 0);

  alignment1 = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_widget_ref (alignment1);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "alignment1", alignment1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (alignment1);
  gtk_box_pack_start (GTK_BOX (vbox32), alignment1, TRUE, TRUE, 0);

  guide_box_darea = gtk_drawing_area_new ();
  gtk_widget_ref (guide_box_darea);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "guide_box_darea", guide_box_darea,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (guide_box_darea);
  gtk_container_add (GTK_CONTAINER (alignment1), guide_box_darea);

  statuslabel2 = gtk_label_new ("");
  gtk_widget_ref (statuslabel2);
  gtk_object_set_data_full (GTK_OBJECT (guide_window), "statuslabel2", statuslabel2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (statuslabel2);
  gtk_box_pack_start (GTK_BOX (guide_vbox), statuslabel2, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (statuslabel2), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (statuslabel2), 7.45058e-09, 0.5);
  gtk_misc_set_padding (GTK_MISC (statuslabel2), 3, 3);

  gtk_object_set_data (GTK_OBJECT (guide_window), "tooltips", tooltips);

  gtk_window_add_accel_group (GTK_WINDOW (guide_window), accel_group);

  return guide_window;
}

GtkWidget*
create_mband_dialog (void)
{
  GtkWidget *mband_dialog;
  GtkWidget *mband_vbox;
  GtkWidget *notebook4;
  GtkWidget *ofr_scw;
  GtkWidget *label93;
  GtkWidget *sob_scw;
  GtkWidget *label94;
  GtkWidget *bands_scw;
  GtkWidget *label96;
  GtkWidget *status_label;

  mband_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (mband_dialog), "mband_dialog", mband_dialog);
  gtk_window_set_title (GTK_WINDOW (mband_dialog), "Multiframe Reduction");
  gtk_window_set_position (GTK_WINDOW (mband_dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (mband_dialog), 720, 400);

  mband_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (mband_vbox);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "mband_vbox", mband_vbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (mband_vbox);
  gtk_container_add (GTK_CONTAINER (mband_dialog), mband_vbox);

  notebook4 = gtk_notebook_new ();
  gtk_widget_ref (notebook4);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "notebook4", notebook4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook4);
  gtk_box_pack_start (GTK_BOX (mband_vbox), notebook4, TRUE, TRUE, 0);

  ofr_scw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (ofr_scw);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "ofr_scw", ofr_scw,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (ofr_scw);
  gtk_container_add (GTK_CONTAINER (notebook4), ofr_scw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ofr_scw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  label93 = gtk_label_new ("Frames");
  gtk_widget_ref (label93);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "label93", label93,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label93);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook4), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook4), 0), label93);

  sob_scw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (sob_scw);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "sob_scw", sob_scw,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (sob_scw);
  gtk_container_add (GTK_CONTAINER (notebook4), sob_scw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sob_scw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  label94 = gtk_label_new ("Stars");
  gtk_widget_ref (label94);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "label94", label94,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label94);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook4), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook4), 1), label94);

  bands_scw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (bands_scw);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "bands_scw", bands_scw,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bands_scw);
  gtk_container_add (GTK_CONTAINER (notebook4), bands_scw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (bands_scw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  label96 = gtk_label_new ("Bands");
  gtk_widget_ref (label96);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "label96", label96,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label96);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook4), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook4), 2), label96);

  status_label = gtk_label_new ("");
  gtk_widget_ref (status_label);
  gtk_object_set_data_full (GTK_OBJECT (mband_dialog), "status_label", status_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (status_label);
  gtk_box_pack_start (GTK_BOX (mband_vbox), status_label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (status_label), 7.45058e-09, 0.5);
  gtk_misc_set_padding (GTK_MISC (status_label), 6, 2);

  return mband_dialog;
}

GtkWidget*
create_query_log_window (void)
{
  GtkWidget *query_log_window;
  GtkWidget *vbox33;
  GtkWidget *scrolledwindow7;
  GtkWidget *query_log_text;
  GtkWidget *query_stop_toggle;

  query_log_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (query_log_window), "query_log_window", query_log_window);
  gtk_window_set_title (GTK_WINDOW (query_log_window), "Download Stars");
  gtk_window_set_modal (GTK_WINDOW (query_log_window), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (query_log_window), 500, 200);

  vbox33 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox33);
  gtk_object_set_data_full (GTK_OBJECT (query_log_window), "vbox33", vbox33,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox33);
  gtk_container_add (GTK_CONTAINER (query_log_window), vbox33);

  scrolledwindow7 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow7);
  gtk_object_set_data_full (GTK_OBJECT (query_log_window), "scrolledwindow7", scrolledwindow7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow7);
  gtk_box_pack_start (GTK_BOX (vbox33), scrolledwindow7, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow7), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  query_log_text = gtk_text_view_new ();
  gtk_widget_ref (query_log_text);

  gtk_object_set_data_full (GTK_OBJECT (query_log_window), "query_log_text", query_log_text,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (query_log_text);
  gtk_container_add (GTK_CONTAINER (scrolledwindow7), query_log_text);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(query_log_text), FALSE);

  query_stop_toggle = gtk_toggle_button_new_with_label ("Stop Transfer");
  gtk_widget_ref (query_stop_toggle);
  gtk_object_set_data_full (GTK_OBJECT (query_log_window), "query_stop_toggle", query_stop_toggle,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (query_stop_toggle);
  gtk_box_pack_start (GTK_BOX (vbox33), query_stop_toggle, FALSE, FALSE, 0);

  return query_log_window;
}

