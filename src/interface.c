
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms-compat.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "params.h"


GtkWidget*
create_pstar (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/pstar.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "pstar"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_camera_control (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/camera.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "camera_control"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_wcs_edit (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/wcsedit.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "wcs_edit"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_show_text (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/showtext.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "show_text"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_create_recipe (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/recipe.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "create_recipe"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_yes_no (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/yesno.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "yes_no"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_image_processing (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/ccdreduction.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "image_processing"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}


GtkWidget*
create_par_edit (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/paredit.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "par_edit"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_entry_prompt (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/entryprompt.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "entry_prompt"));

	g_object_unref (G_OBJECT(builder));

	return widget;
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
//  guint guide_run_key;
  GtkWidget *guide_run;
//  guint guide_dark_key;
  GtkWidget *guide_dark;
//  guint guide_find_star_key;
  GtkWidget *guide_find_star;
//  guint guide_calibrate_key;
  GtkWidget *guide_calibrate;
  GtkWidget *hseparator8;
  GtkWidget *label92;
  GtkWidget *guide_exp_combo;
  GtkWidget *guide_exp_combo_entry;
  GtkWidget *guide_options;
  GtkWidget *alignment1;
  GtkWidget *guide_box_darea;
  GtkWidget *statuslabel2;
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  guide_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_set_data (G_OBJECT (guide_window), "guide_window", guide_window);
  gtk_window_set_title (GTK_WINDOW (guide_window), "Guiding");
  gtk_window_set_default_size (GTK_WINDOW (guide_window), 640, 400);

  guide_vbox = gtk_vbox_new (FALSE, 0);
  g_object_ref (guide_vbox);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_vbox", guide_vbox,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_vbox);
  gtk_container_add (GTK_CONTAINER (guide_window), guide_vbox);

  hbox24 = gtk_hbox_new (FALSE, 0);
  g_object_ref (hbox24);
  g_object_set_data_full (G_OBJECT (guide_window), "hbox24", hbox24,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hbox24);
  gtk_box_pack_start (GTK_BOX (guide_vbox), hbox24, TRUE, TRUE, 0);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  g_object_ref (scrolled_window);
  g_object_set_data_full (G_OBJECT (guide_window), "scrolled_window", scrolled_window,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (hbox24), scrolled_window, TRUE, TRUE, 0);

  viewport5 = gtk_viewport_new (NULL, NULL);
  g_object_ref (viewport5);
  g_object_set_data_full (G_OBJECT (guide_window), "viewport5", viewport5,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (viewport5);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport5);

  image_alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  g_object_ref (image_alignment);
  g_object_set_data_full (G_OBJECT (guide_window), "image_alignment", image_alignment,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (image_alignment);
  gtk_container_add (GTK_CONTAINER (viewport5), image_alignment);

  image = gtk_drawing_area_new ();
  g_object_ref (image);
  g_object_set_data_full (G_OBJECT (guide_window), "image", image,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (image_alignment), image);

  vbox32 = gtk_vbox_new (FALSE, 3);
  g_object_ref (vbox32);
  g_object_set_data_full (G_OBJECT (guide_window), "vbox32", vbox32,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (vbox32);
  gtk_box_pack_start (GTK_BOX (hbox24), vbox32, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox32), 12);

  guide_run = gtk_toggle_button_new_with_mnemonic ("_Run");
//  guide_run_key = gtk_label_get_mnemonic_keyval (GTK_LABEL (GTK_BIN (guide_run)->child));
//  gtk_widget_add_accelerator (guide_run, "clicked", accel_group,
//			      guide_run_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  g_object_ref (guide_run);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_run", guide_run,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_run);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_run, FALSE, FALSE, 0);

  guide_dark = gtk_button_new_with_mnemonic ("_Dark Frame");
//  guide_dark_key = gtk_label_get_mnemonic_keyval (GTK_LABEL (GTK_BIN (guide_dark)->child));
//  gtk_widget_add_accelerator (guide_dark, "clicked", accel_group,
//                              guide_dark_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  g_object_ref (guide_dark);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_dark", guide_dark,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_dark);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_dark, FALSE, FALSE, 0);

  guide_find_star = gtk_button_new_with_mnemonic ("_Guide Star");
//  guide_find_star_key = gtk_label_get_mnemonic_keyval (GTK_LABEL (GTK_BIN (guide_find_star)->child));
//  gtk_widget_add_accelerator (guide_find_star, "clicked", accel_group,
//                              guide_find_star_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  g_object_ref (guide_find_star);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_find_star", guide_find_star,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_find_star);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_find_star, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (guide_find_star, "Set one of the user-selected stars as a guide star; if no user selected stars exist, search for a suitable star");

  guide_calibrate = gtk_toggle_button_new_with_mnemonic ("_Calibrate");
//  guide_calibrate_key = gtk_label_get_mnemonic_keyval (GTK_LABEL (GTK_BIN (guide_calibrate)->child));
//  gtk_widget_add_accelerator (guide_calibrate, "clicked", accel_group,
//                              guide_calibrate_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);
  g_object_ref (guide_calibrate);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_calibrate", guide_calibrate,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_calibrate);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_calibrate, FALSE, FALSE, 0);

  hseparator8 = gtk_hseparator_new ();
  g_object_ref (hseparator8);
  g_object_set_data_full (G_OBJECT (guide_window), "hseparator8", hseparator8,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (hseparator8);
  gtk_box_pack_start (GTK_BOX (vbox32), hseparator8, FALSE, TRUE, 3);

  label92 = gtk_label_new ("Exposure");
  g_object_ref (label92);
  g_object_set_data_full (G_OBJECT (guide_window), "label92", label92,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (label92);
  gtk_box_pack_start (GTK_BOX (vbox32), label92, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label92), 7.45058e-09, 1);

  guide_exp_combo = gtk_combo_box_text_new_with_entry ();
  g_object_ref (guide_exp_combo);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_exp_combo", guide_exp_combo,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_exp_combo);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_exp_combo, FALSE, FALSE, 0);

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (guide_exp_combo), "1/8");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (guide_exp_combo), "1/4");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (guide_exp_combo), "1/2");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (guide_exp_combo), "1");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (guide_exp_combo), "2");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (guide_exp_combo), "4");

  guide_exp_combo_entry = gtk_bin_get_child (GTK_BIN(guide_exp_combo));
  g_object_ref (guide_exp_combo_entry);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_exp_combo_entry", guide_exp_combo_entry,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_exp_combo_entry);
  gtk_widget_set_size_request (guide_exp_combo_entry, 64, -1);
  gtk_entry_set_text (GTK_ENTRY (guide_exp_combo_entry), "2");

  guide_options = gtk_button_new_with_label ("Options");
  g_object_ref (guide_options);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_options", guide_options,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_options);
  gtk_box_pack_start (GTK_BOX (vbox32), guide_options, FALSE, FALSE, 0);

  alignment1 = gtk_alignment_new (0.5, 0.5, 0, 0);
  g_object_ref (alignment1);
  g_object_set_data_full (G_OBJECT (guide_window), "alignment1", alignment1,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (alignment1);
  gtk_box_pack_start (GTK_BOX (vbox32), alignment1, TRUE, TRUE, 0);

  guide_box_darea = gtk_drawing_area_new ();
  g_object_ref (guide_box_darea);
  g_object_set_data_full (G_OBJECT (guide_window), "guide_box_darea", guide_box_darea,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (guide_box_darea);
  gtk_container_add (GTK_CONTAINER (alignment1), guide_box_darea);

  statuslabel2 = gtk_label_new ("");
  g_object_ref (statuslabel2);
  g_object_set_data_full (G_OBJECT (guide_window), "statuslabel2", statuslabel2,
                            (GDestroyNotify) g_object_unref);
  gtk_widget_show (statuslabel2);
  gtk_box_pack_start (GTK_BOX (guide_vbox), statuslabel2, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (statuslabel2), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (statuslabel2), 7.45058e-09, 0.5);
  gtk_misc_set_padding (GTK_MISC (statuslabel2), 3, 3);

  gtk_window_add_accel_group (GTK_WINDOW (guide_window), accel_group);

  return guide_window;
}


GtkWidget*
create_mband_dialog (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/multiband.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "mband_dialog"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_query_log_window (void)
{
	GtkBuilder *builder = gtk_builder_new_from_resource ("/org/gcx/ui/query_window.ui");
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object (builder, "query_window"));

	g_object_unref (G_OBJECT(builder));

	return widget;
}

GtkWidget*
create_imadj_dialog (void)
{
	GtkWidget *imadj_dialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *hist_scrolled_win;
	GtkWidget *viewport1;
	GtkWidget *hist_area;
	GtkWidget *vbox2;
	GtkWidget *frame3;
	GtkWidget *hbox2;
	GtkWidget *channel_combo;
	GtkWidget *label3;
	GObject   *gamma_spin_adj;
	GtkWidget *gamma_spin;
	GtkWidget *label4;
	GObject   *toe_spin_adj;
	GtkWidget *toe_spin;
	GtkWidget *label5;
	GtkWidget *log_hist_check;
	GtkWidget *frame2;
	GtkWidget *table1;
	GtkWidget *hseparator1;
	GObject   *low_cut_spin_adj;
	GtkWidget *low_cut_spin;
	GObject   *high_cut_spin_adj;
	GtkWidget *high_cut_spin;
	GObject   *offset_spin_adj;
	GtkWidget *offset_spin;
	GtkWidget *label2;
	GtkWidget *label1;
	GtkWidget *table2;
	GtkWidget *cuts_darker;
	GtkWidget *cuts_sharper;
	GtkWidget *cuts_duller;
	GtkWidget *cuts_auto;
	GtkWidget *cuts_min_max;
	GtkWidget *cuts_brighter;
	GtkWidget *dialog_action_area1;
	GtkWidget *hbuttonbox1;
	GtkWidget *hist_close;
	GtkWidget *hist_apply;
	GtkWidget *hist_redraw;

	imadj_dialog = gtk_dialog_new ();
	g_object_set_data (G_OBJECT (imadj_dialog), "imadj_dialog", imadj_dialog);
	gtk_window_set_title (GTK_WINDOW (imadj_dialog), ("Curves / Histogram"));
	//  GTK_WINDOW (imadj_dialog)->type = GTK_WINDOW_DIALOG;

	dialog_vbox1 = gtk_dialog_get_content_area (GTK_DIALOG(imadj_dialog));
	g_object_set_data (G_OBJECT (imadj_dialog), "dialog_vbox1", dialog_vbox1);
	gtk_widget_show (dialog_vbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	g_object_ref (vbox1);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "vbox1", vbox1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

	hist_scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	g_object_ref (hist_scrolled_win);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_scrolled_win", hist_scrolled_win,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hist_scrolled_win);
	gtk_box_pack_start (GTK_BOX (vbox1), hist_scrolled_win, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hist_scrolled_win), 2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (hist_scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

	//gtk_range_set_update_policy (GTK_RANGE (GTK_SCROLLED_WINDOW (hist_scrolled_win)->hscrollbar), GTK_POLICY_NEVER);

	viewport1 = gtk_viewport_new (NULL, NULL);
	g_object_ref (viewport1);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "viewport1", viewport1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (viewport1);
	gtk_container_add (GTK_CONTAINER (hist_scrolled_win), viewport1);

	hist_area = gtk_drawing_area_new ();
	g_object_ref (hist_area);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_area", hist_area,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hist_area);
	gtk_container_add (GTK_CONTAINER (viewport1), hist_area);

	vbox2 = gtk_vbox_new (FALSE, 0);
	g_object_ref (vbox2);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "vbox2", vbox2,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, TRUE, 0);

	frame3 = gtk_frame_new (("Curve/Histogram"));
	g_object_ref (frame3);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "frame3", frame3,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (frame3);
	gtk_box_pack_start (GTK_BOX (vbox2), frame3, TRUE, TRUE, 0);

	hbox2 = gtk_hbox_new (FALSE, 0);
	g_object_ref (hbox2);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hbox2", hbox2,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hbox2);
	gtk_container_add (GTK_CONTAINER (frame3), hbox2);

	channel_combo = gtk_combo_box_text_new ();
	g_object_ref (channel_combo);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "chanel_combo", channel_combo,
				(GDestroyNotify) g_object_unref);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(channel_combo), "Channel");
	//gtk_combo_box_set_active (GTK_COMBO_BOX(channel_combo), 0);
	gtk_widget_show (channel_combo);
	gtk_box_pack_start (GTK_BOX (hbox2), channel_combo, FALSE, FALSE, 0);

	label3 = gtk_label_new (("Gamma"));
	g_object_ref (label3);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "label3", label3,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (hbox2), label3, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label3), 5, 0);

	gamma_spin_adj = gtk_adjustment_new (1, 0.1, 10, 0.1, 1, 0.0);
	gamma_spin = gtk_spin_button_new (GTK_ADJUSTMENT (gamma_spin_adj), 1, 1);
	g_object_ref (gamma_spin);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "gamma_spin", gamma_spin,
				(GDestroyNotify) g_object_unref);
	gtk_widget_set_size_request (GTK_WIDGET(&(GTK_SPIN_BUTTON(gamma_spin)->entry)), 60, -1);
	gtk_widget_show (gamma_spin);
	gtk_box_pack_start (GTK_BOX (hbox2), gamma_spin, FALSE, TRUE, 0);

	label4 = gtk_label_new (("Toe"));
	g_object_ref (label4);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "label4", label4,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (hbox2), label4, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label4), 6, 0);

	toe_spin_adj = gtk_adjustment_new (0, 0, 0.4, 0.002, 0.1, 0);
	toe_spin = gtk_spin_button_new (GTK_ADJUSTMENT (toe_spin_adj), 0.002, 3);
	g_object_ref (toe_spin);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "toe_spin", toe_spin,
				(GDestroyNotify) g_object_unref);
	gtk_widget_set_size_request(GTK_WIDGET(&(GTK_SPIN_BUTTON(toe_spin)->entry)), 60, -1);
	gtk_widget_show (toe_spin);
	gtk_box_pack_start (GTK_BOX (hbox2), toe_spin, FALSE, FALSE, 0);

	label5 = gtk_label_new (("Offset"));
	g_object_ref (label5);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "label5", label5,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (label5);
	gtk_box_pack_start (GTK_BOX (hbox2), label5, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label5), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label5), 5, 0);

	offset_spin_adj = gtk_adjustment_new (0, 0, 1, 0.01, 1, 0.0);
	offset_spin = gtk_spin_button_new (GTK_ADJUSTMENT (offset_spin_adj), 0.01, 2);
	g_object_ref (offset_spin);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "offset_spin", offset_spin,
				(GDestroyNotify) g_object_unref);
	gtk_widget_set_size_request(GTK_WIDGET(&(GTK_SPIN_BUTTON(offset_spin)->entry)), 60, -1);
	gtk_widget_show (offset_spin);
	gtk_box_pack_start (GTK_BOX (hbox2), offset_spin, FALSE, TRUE, 0);

	log_hist_check = gtk_check_button_new_with_label (("Log"));
	g_object_ref (log_hist_check);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "log_hist_check", log_hist_check,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (log_hist_check);
	gtk_box_pack_start (GTK_BOX (hbox2), log_hist_check, FALSE, FALSE, 0);

	frame2 = gtk_frame_new (("Cuts"));
	g_object_ref (frame2);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "frame2", frame2,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (frame2);
	gtk_box_pack_start (GTK_BOX (vbox2), frame2, TRUE, TRUE, 0);

	table1 = gtk_table_new (3, 3, FALSE);
	g_object_ref (table1);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "table1", table1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame2), table1);

	hseparator1 = gtk_hseparator_new ();
	g_object_ref (hseparator1);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hseparator1", hseparator1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hseparator1);
	gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 3, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

	low_cut_spin_adj = gtk_adjustment_new (1, -65535, 65535, 2, 10, 0);
	low_cut_spin = gtk_spin_button_new (GTK_ADJUSTMENT (low_cut_spin_adj), 2, 0);
	g_object_ref (low_cut_spin);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "low_cut_spin", low_cut_spin,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (low_cut_spin);
	gtk_table_attach (GTK_TABLE (table1), low_cut_spin, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	high_cut_spin_adj = gtk_adjustment_new (1, -65535, 65535, 10, 10, 0);
	high_cut_spin = gtk_spin_button_new (GTK_ADJUSTMENT (high_cut_spin_adj), 10, 0);
	g_object_ref (high_cut_spin);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "high_cut_spin", high_cut_spin,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (high_cut_spin);
	gtk_table_attach (GTK_TABLE (table1), high_cut_spin, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	label2 = gtk_label_new (("High"));
	g_object_ref (label2);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "label2", label2,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table1), label2, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label2), 6, 0);

	label1 = gtk_label_new (("Low"));
	g_object_ref (label1);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "label1", label1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (label1);
	gtk_table_attach (GTK_TABLE (table1), label1, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label1), 6, 0);

	table2 = gtk_table_new (2, 3, TRUE);
	g_object_ref (table2);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "table2", table2,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (table2);
	gtk_table_attach (GTK_TABLE (table1), table2, 2, 3, 1, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 3);

	cuts_darker = gtk_button_new_with_label (("Darker"));
	g_object_ref (cuts_darker);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_darker", cuts_darker,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (cuts_darker);
	gtk_table_attach (GTK_TABLE (table2), cuts_darker, 2, 3, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	cuts_sharper = gtk_button_new_with_label (("Sharper"));
	g_object_ref (cuts_sharper);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_sharper", cuts_sharper,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (cuts_sharper);
	gtk_table_attach (GTK_TABLE (table2), cuts_sharper, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	cuts_duller = gtk_button_new_with_label (("Flatter"));
	g_object_ref (cuts_duller);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_duller", cuts_duller,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (cuts_duller);
	gtk_table_attach (GTK_TABLE (table2), cuts_duller, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	cuts_auto = gtk_button_new_with_label (("Auto Cuts"));
	g_object_ref (cuts_auto);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_auto", cuts_auto,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (cuts_auto);
	gtk_table_attach (GTK_TABLE (table2), cuts_auto, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	cuts_min_max = gtk_button_new_with_label (("Min-Max"));
	g_object_ref (cuts_min_max);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_min_max", cuts_min_max,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (cuts_min_max);
	gtk_table_attach (GTK_TABLE (table2), cuts_min_max, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);

	cuts_brighter = gtk_button_new_with_label (("Brighter"));
	g_object_ref (cuts_brighter);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "cuts_brighter", cuts_brighter,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (cuts_brighter);
	gtk_table_attach (GTK_TABLE (table2), cuts_brighter, 2, 3, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND), 0, 0);

	dialog_action_area1 = gtk_dialog_get_action_area (GTK_DIALOG(imadj_dialog));
	g_object_set_data (G_OBJECT (imadj_dialog), "dialog_action_area1", dialog_action_area1);
	gtk_widget_show (dialog_action_area1);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 3);

	hbuttonbox1 = gtk_hbutton_box_new ();
	g_object_ref (hbuttonbox1);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hbuttonbox1", hbuttonbox1,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hbuttonbox1);
	gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbuttonbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbuttonbox1), 3);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_EDGE);
	gtk_box_set_spacing (GTK_BOX (hbuttonbox1), 4);

	//gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox1), 0, 0);
	//gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox1), 15, -1);

	hist_close = gtk_button_new_with_label (("Close"));
	g_object_ref (hist_close);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_close", hist_close,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hist_close);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), hist_close);
	gtk_widget_set_can_default (hist_close, TRUE);

	hist_apply = gtk_button_new_with_label (("Apply"));
	g_object_ref (hist_apply);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_apply", hist_apply,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hist_apply);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), hist_apply);
	gtk_widget_set_can_default (hist_apply, TRUE);

	hist_redraw = gtk_button_new_with_label (("Redraw"));
	g_object_ref (hist_redraw);
	g_object_set_data_full (G_OBJECT (imadj_dialog), "hist_redraw", hist_redraw,
				(GDestroyNotify) g_object_unref);
	gtk_widget_show (hist_redraw);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), hist_redraw);
	gtk_widget_set_can_default (hist_redraw, TRUE);

	return imadj_dialog;
}
