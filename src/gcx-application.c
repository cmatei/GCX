
#include "gcx-application.h"

struct _GcxApplication {
	GtkApplication parent_instance;

};

struct _GcxApplicationClass {
	GtkApplicationClass parent_class;
};


G_DEFINE_TYPE(GcxApplication, gcx_application, GTK_TYPE_APPLICATION);

static void
gcx_application_startup (GApplication *application)
{
	G_APPLICATION_CLASS (gcx_application_parent_class)->startup (application);
}

static void
gcx_application_shutdown (GApplication *application)
{
	G_APPLICATION_CLASS (gcx_application_parent_class)->shutdown (application);
}


extern GtkWidget *create_image_window();
static void
gcx_application_activate (GApplication *application)
{
	GtkWidget *window = create_image_window ();

	// new_window (application, NULL);
}

#if 0

static void
gcx_application_open (GApplication  *application,
		      GFile        **files,
		      gint           n_files,
		      const gchar   *hint)
{
//	gint i;
//	for (i = 0; i < n_files; i++)
//		new_window (application, files[i]);
}
#endif

static void
gcx_application_finalize (GObject *object)
{
	G_OBJECT_CLASS (gcx_application_parent_class)->finalize (object);
}

static void
gcx_application_init (GcxApplication *app)
{
}

static gboolean
gcx_application_local_command_line (GApplication *app,
				    gchar      ***args,
				    gint         *status)
{
	char *shortopts = "D:p:hP:V:vo:id:b:f:B:M:A:O:usa:T:S:nG:Nj:FcX:e:w";
	struct option longopts[] = {
		{"debug", required_argument, NULL, 'D'},

		{"dark", required_argument, NULL, 'd'},
		{"bias", required_argument, NULL, 'b'},
		{"flat", required_argument, NULL, 'f'},
		{"overscan", required_argument, NULL, '[' },
		{"badpix", required_argument, NULL, 'B'},
		{"extract-badpix", required_argument, NULL, 'X'},

		{ "sextract", required_argument, NULL, 'e' },

		{"add-bias", required_argument, NULL, 'A'},
		{"multiply", required_argument, NULL, 'M'},
		{"gaussian-blur", required_argument, NULL, 'G'},

		{"output", required_argument, NULL, 'o'},
		{"update-file", no_argument, NULL, 'u'},

		{"stack", no_argument, NULL, 's'},
		{"superflat", no_argument, NULL, 'F'},
		{"align", required_argument, NULL, 'a'},
		{"no-reduce", no_argument, NULL, 'N'},
		{"set", required_argument, NULL, 'S'},

		{"rcfile", required_argument, NULL, 'r'},
		{"obsfile", required_argument, NULL, 'O'},
		{"recipe", required_argument, NULL, 'p'},
//		{"convert-rcp", required_argument, NULL, '2'},
		{"to-pnm", no_argument, NULL, 'n'},
		{"import", required_argument, NULL, '4'},
		{"save-internal-cat", no_argument, NULL, '7'},
		{"extract-targets", required_argument, NULL, '8'},
		{"mag-limit", required_argument, NULL, '9'},
		{"merge", required_argument, NULL, '0'},
		{"set-target", required_argument, NULL, '_'},
		{"make-tycho-rcp", required_argument, NULL, ']'},
		{"make-cat-rcp", required_argument, NULL, '>'},
		{"wcs-fit", no_argument, NULL, 'w'},

		{"rep-to-table", required_argument, NULL, 'T'},
		{"phot-run", required_argument, NULL, 'P'},
		{"phot-run-aavso", required_argument, NULL, 'V'},
		{"interactive", no_argument, NULL, 'i'},
		{"help", no_argument, NULL, 'h'},
		{"help-all", no_argument, NULL, '3'},
		{"help-rep-conv", no_argument, NULL, '('},
		{"test", no_argument, NULL, ')'},
		{"version", no_argument, NULL, '1'},
		{"object", required_argument, NULL, 'j'},
		{"rcp-to-aavso", required_argument, NULL, '6'},
		{"multi-band-reduce", required_argument, NULL, '^'},
		{"options-doc", no_argument, NULL, '`'},
		{"scintillation", required_argument, NULL, ';'},

		{"demosaic", optional_argument, NULL, 'c' },

		{NULL, 0, NULL, 0}
	};

	setenv("LANG", "C", 1);

	init_ptable();

	/* FIXME: NULL arg should be gcx instance */
	gcx_load_rcfile (NULL, NULL);
	gtk_init (&ac, &av);

	debug_level = 0;

	while ((oc = getopt_long(ac, av, shortopts, longopts, NULL)) > 0) {
		switch(oc) {
		case ';':
			print_scint_table(optarg);
			exit(0);
		case '9':
			sscanf(optarg, "%f", &mag_limit);
			break;
		case '<':
			//make_gpsf = 1;
			break;
		case ')':
//			test_query();
			return 0;
			break;
		case '`':
			doc_printf_par(stdout, PAR_FIRST, 0);
			exit(0);
			break;
		case '^':
			strncpy(mband, optarg, 1023);
			break;
		case '8':
			strncpy(extrf, optarg, 1023);
			batch = 1;
			break;
		case '0':
			strncpy(mergef, optarg, 1023);
			batch = 1;
			break;
		case '_':
			strncpy(tobj, optarg, 1023);
			batch = 1;
			break;
		case ']':
			sscanf(optarg, "%f", &tycrcp_box);
			batch = 1;
			break;
		case '>':
			sscanf(optarg, "%f", &cat_box);
			batch = 1;
			break;
		case 'D':
			sscanf(optarg, "%d", &debug_level);
			break;
		case 'N':
			no_reduce = 1;
			break;
		case 'h':
			show_usage();
			exit(0);
		case '3':
			help_all();
			exit(0);
		case '(':
			help_rep_conv();
			exit(0);
		case '1':
			info_printf("%s\n", VERSION);
			exit(0);
		case ':':
			err_printf("option -%c requires an argument\n", optopt);
			exit(1);
		case '?':
			err_printf("Try `gcx -h' for more information\n");
			exit(1);
		case 'i':
			interactive = 1;
			break;
		case 'n':
			to_pnm = 1;
			batch = 1;
			break;
		case 'w':
			fit_wcs = 1;
			batch = 1;
			break;
		case 'O':
			strncpy(of, optarg, 1023);
			break;
		case 'o':
			strncpy(outf, optarg, 1023);
			break;
		case 'p':
			sscanf(optarg, "%800s", rf);
			break;
		case 'P':
			sscanf(optarg, "%800s", rf);
			run_phot = REP_ALL|REP_DATASET;
			batch = 1;
			break;
		case 'V':
			sscanf(optarg, "%800s", rf);
			run_phot = REP_TGT|REP_AAVSO;
			batch = 1;
			break;
		case '2':
			if (optarg) {
				if (optarg[0] == '-' && optarg[1] == 0)
					;
				else
					strncpy(rcf, optarg, 1023);
			}
			convert_recipe = 1;
			batch = 1;
			break;
		case '6':
			if (optarg) {
				if (optarg[0] == '-' && optarg[1] == 0)
					;
				else
					strncpy(rcf, optarg, 1023);
			}
			rcp_to_aavso = 1;
			batch = 1;
			break;
		case '7':
			save_internal_cat = 1;
			break;
		case 'S':
			if (optarg) {
				if (set_par_by_name(optarg))
					err_printf("error setting %s\n", optarg);
			}
			break;
		case 'T':
			if (optarg) {
				if (optarg[0] == '-' && optarg[1] == 0)
					;
				else
					strncpy(repf, optarg, 1023);
			}
			rep_to_table = 1;
			batch = 1;
			break;
		case '4':
			if (optarg)
				strncpy(ldf, optarg, 1023);
			break;
		case 'j':
			if (optarg)
				strncpy(obj, optarg, 1023);
//			interactive = 1;
			break;
		case 'u':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_INPLACE;
			update_files = 1;
			break;
		case 's':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_STACK;
			batch = 1;
			break;
		case 'F':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_STACK;
			ccdr->ops |= IMG_OP_BG_ALIGN_MUL;
			batch = 1;
			break;

		case '[':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_OVERSCAN;
			ccdr->overscan = P_DBL(CCDRED_OVERSCAN_PEDESTAL);

			v = strtod(optarg, &endp);
			if (endp != optarg) {
				ccdr->overscan = v;
			} else {
				ccdr->overscan = P_DBL(CCDRED_OVERSCAN_PEDESTAL);
			}
			batch = 1;
			break;
		case 'a':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_ALIGN;
			ccdr->alignref = image_file_new();
			ccdr->alignref->filename = strdup(optarg);
			batch = 1;
			break;
		case 'b':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->bias = image_file_new();
			ccdr->bias->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_BIAS;
			batch = 1;
			break;
		case 'f':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->flat = image_file_new();
			ccdr->flat->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_FLAT;
			batch = 1;
			break;
		case 'd':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->dark = image_file_new();
			ccdr->dark->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_DARK;
			batch = 1;
			break;
		case 'G':
			v = strtod(optarg, &endp);
			if (endp != optarg) {
				if (ccdr == NULL)
					ccdr = ccd_reduce_new();
				ccdr->blurv = v;
				ccdr->ops |= IMG_OP_BLUR;
			}
			batch = 1;
			break;
		case 'M':
			v = strtod(optarg, &endp);
			if (endp != optarg) {
				if (ccdr == NULL)
					ccdr = ccd_reduce_new();
				ccdr->mulv = v;
				ccdr->ops |= IMG_OP_MUL;
			}
			batch = 1;
			break;
		case 'A':
			v = strtod(optarg, &endp);
			if (endp != optarg) {
				if (ccdr == NULL)
					ccdr = ccd_reduce_new();
				ccdr->addv = v;
				ccdr->ops |= IMG_OP_ADD;
			}
			batch = 1;
			break;

		case 'B':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->bad_pix_map = calloc(1, sizeof(struct bad_pix_map));
			ccdr->bad_pix_map->filename = strdup(optarg);
			ccdr->ops |= IMG_OP_BADPIX;
			batch = 1;
			break;

		case 'X':
			extract_badpix = 1;
			strncpy(badpixf, optarg, 1023);
			break;

		case 'e':
			extract_stars = 1;
			strncpy(starf, optarg, 1023);
			break;

		case 'c':
			if (ccdr == NULL)
				ccdr = ccd_reduce_new();
			ccdr->ops |= IMG_OP_DEMOSAIC;
			batch = 1;
			break;
		}
	}

/* now run the various requested operations */

	if (rc_fn != NULL) {/* load extra rc file */
		gcx_load_rcfile (NULL, rc_fn);
	}

	if (extract_badpix) {
		extract_bad_pixels(badpixf, outf); /* does not return */
	}

	if (extract_stars) {
		extract_sources(starf, outf); /* does not return */
	}

	if (mband[0] != 0) {
		mb_reduce(mband, outf); /* does not return */
	}
	if (tycrcp_box > 0.0) {
		make_tycho_rcp(obj, tycrcp_box, outf, mag_limit); /* does not return */
	}
	if (cat_box > 0.0) {
		cat_rcp(obj, cat_box, outf, mag_limit); /* does not return */
	}
	if (mergef[0] != 0) {
		if (rf[0] == 0) {
			err_printf("Please specify a recipe file to merge to (with -p)\n");
			exit(1);
		}
		recipe_merge(rf, mergef, outf, mag_limit); /* does not return */
	}

	if (tobj[0] != 0) {
		if (rf[0] == 0) {
			err_printf("Please specify a recipe file to set target in (with -p)\n");
			exit(1);
		}
		recipe_set_tobj(rf, tobj, outf, mag_limit); /* does not return */
	}

	if (convert_recipe) { /* convert recipe */
		recipe_file_convert(rcf, outf); /* does not return */
	}

	if (save_internal_cat) { /* save int catalog */
		FILE *outfp;
		if (outf[0] != 0) {
			outfp = fopen(outf, "w");
			if (outfp == NULL) {
				err_printf("Cannot open file %s for writing\n%s\n",
					   outf, strerror(errno));
				exit(1);
			}
			output_internal_catalog(outfp, mag_limit);
			fclose(outfp);
			exit(0);
		}
		output_internal_catalog(stdout, mag_limit);
		exit(0);
	}

	if (rcp_to_aavso) { /* convert recipe */
		recipe_aavso_convert(rcf, outf); /* does not return */
	}

	if (rep_to_table) { /* convert a report into tabular format */
		report_convert(repf, outf); /* does not return */
	}

	if (ldf[0] != 0) { /* import landolt or or other table */
		d3_printf("%s import\n", ldf); /* does not return */
		catalog_file_convert(ldf, outf, mag_limit);
	}

	if (of[0] != 0) { /* search path for obs file */
		if ((strchr(of, '/') == NULL)) {
			file = find_file_in_path(of, P_STR(FILE_OBS_PATH));
			if (file != NULL) {
				strncpy(of, file, 1023);
				of[1023] = 0;
				free(file);
			}
		}
		d3_printf("of is %s\n", of);
	}

	if (rf[0] != 0) { /* search path for recipe file */
		if ((strchr(of, '/') == NULL)) {
			file = find_file_in_path(rf, P_STR(FILE_RCP_PATH));
			if (file != NULL) {
				strncpy(rf, file, 1023);
				rf[1023] = 0;
				free(file);
			}
		}
		d3_printf("rf is '%s'\n", rf);
	}

	if (ac > optind) { /* we build the image list */
		imfl = image_file_list_new();
		for (i = optind; i < ac; i++)
			add_image_file_to_list(imfl, av[i], 0);
	}

	if (ccdr != NULL) {
		if (batch && !interactive) {
			if ((outf != NULL && outf[0] != 0) || update_files) {
				if (imfl == NULL) {
					err_printf("No frames to process, exiting\n");
					ccd_reduce_release(ccdr);
					exit(1);
				}
				d3_printf("outf is |%s|\n", outf);
				ret = batch_reduce_frames(imfl, ccdr, outf);
				ccd_reduce_release(ccdr);
				image_file_list_release(imfl);
				exit(ret);
			} else {
				info_printf("no output file name: going interactive\n");
				interactive = 1;
			}
		}
	}

	if ((imfl != NULL) && (ccdr != NULL) && !no_reduce) {
		fr = reduce_frames_load(imfl, ccdr);
		if (fr == NULL)
			exit (1);
	}

	window = create_image_window();

	if (!batch || interactive) {
		gtk_widget_show_all(window);
	}
	set_imfl_ccdr(window, ccdr, imfl);

	if (imfl == NULL && obj[0] && fr == NULL) {
		fr = make_blank_obj_fr(obj);
		if (fr != NULL) {
			frame_to_window (fr, window);
			release_frame(fr);
		}
	}

	if ((ac > optind)) {
		if (fr == NULL) {
			fr = read_image_file(av[optind], P_STR(FILE_UNCOMPRESS),
					     P_INT(FILE_UNSIGNED_FITS),
					     default_cfa[P_INT(FILE_DEFAULT_CFA)]);
			if (fr) {
				rescan_fits_exp(fr, &fr->exp);
				rescan_fits_wcs(fr, &fr->fim);
			}
		}
		if (fr == NULL) {
			err_printf("cannot read file %s\n", av[optind]);
			exit(1);
		}
		if (obj[0] != 0)
			set_wcs_from_object(fr, obj, -P_DBL(WCS_SEC_PER_PIX));

		frame_to_window (fr, window);
		release_frame(fr);
		if (to_pnm) {
			GcxImageView *view = g_object_get_data (G_OBJECT(window), "image_view");
			ret = gcx_image_view_to_pnm_file (view, *outf ? outf : NULL, 0);
			if (ret) {
				err_printf("error writing pnm file\n");
				if (batch && !interactive)
					exit(1);
			}
			if (batch && !interactive)
				exit(0);
		}
		if (fit_wcs) {
			char *fn;
			int rtn;

			if (match_field_in_window_quiet(window))
				exit(2);

			fr = frame_from_window (window);
			wcs_to_fits_header(fr);

			fn = fr->name;
			if (file_is_zipped(fn)) {
				for (i = strlen(fn); i > 0; i--)
					if (fn[i] == '.') {
						fn[i] = 0;
						break;
					}
				rtn = write_gz_fits_frame(fr, fn, P_STR(FILE_COMPRESS));
			} else {
				rtn = write_fits_frame(fr, fn);
			}

			if (batch && !interactive) {
				if (rtn == 0)
					exit(0);
				else
					exit(3);
			}
		}
		if (rf[0] != 0) {
			FILE *output_file = NULL;
			load_rcp_to_window(window, rf, obj);
			match_field_in_window_quiet(window);
			if (run_phot) {
				if (outf[0] != 0) {
					output_file = fopen(outf, "a");
					if (output_file == NULL)
						err_printf("Cannot open file %s "
							   "for appending\n%s\n",
							   outf, strerror(errno));
				}
				if (output_file == NULL)
					phot_to_fd(window, stdout, run_phot);
				else {
					phot_to_fd(window, output_file, run_phot);
					fclose(output_file);
				}
			}
			if (batch && !interactive) {
				exit(0);

			}
		}
		d3_printf("calling gtk_main\n");
		gtk_main ();
		exit(0);
	}

	d3_printf("calling gtk_main, no frame\n");
	if (of[0] != 0) { /* run obs */
		if (run_obs_file(window, of))
			exit (1);
	}

	*status = 0;

	/* completely handled command-line */
	return TRUE;
}

static void
gcx_application_class_init (GcxApplicationClass *class)
{
	GApplicationClass *application_class = G_APPLICATION_CLASS (class);
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	application_class->startup  = gcx_application_startup;
	application_class->shutdown = gcx_application_shutdown;
	application_class->activate = gcx_application_activate;
//  application_class->open     = gcx_application_open;

	application_class->local_command_line = gcx_application_local_command_line;

	object_class->finalize = gcx_application_finalize;
}

GcxApplication *
gcx_application_new (void)
{
	GcxApplication *app;

	g_set_application_name ("Gcx");

	app = g_object_new (gcx_application_get_type (),
			    "application-id", "org.gcx.app",
			    "flags", G_APPLICATION_HANDLES_OPEN,
			    NULL);

	return app;
}


#if 0

#include <stdlib.h>
#include <gtk/gtk.h>

typedef struct
{
  GtkApplication parent_instance;

  guint quit_inhibit;
  GMenu *time;
  guint timeout;
} BloatPad;

typedef GtkApplicationClass BloatPadClass;

G_DEFINE_TYPE (BloatPad, bloat_pad, GTK_TYPE_APPLICATION)

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GVariant *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
activate_radio (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  g_action_change_state (G_ACTION (action), parameter);
}

static void
change_fullscreen_state (GSimpleAction *action,
                         GVariant      *state,
                         gpointer       user_data)
{
  if (g_variant_get_boolean (state))
    gtk_window_fullscreen (user_data);
  else
    gtk_window_unfullscreen (user_data);

  g_simple_action_set_state (action, state);
}

static void
change_busy_state (GSimpleAction *action,
                   GVariant      *state,
                   gpointer       user_data)
{
  GtkWindow *window = user_data;
  GApplication *application = G_APPLICATION (gtk_window_get_application (window));

  /* do this twice to test multiple busy counter increases */
  if (g_variant_get_boolean (state))
    {
      g_application_mark_busy (application);
      g_application_mark_busy (application);
    }
  else
    {
      g_application_unmark_busy (application);
      g_application_unmark_busy (application);
    }

  g_simple_action_set_state (action, state);
}

static void
change_justify_state (GSimpleAction *action,
                      GVariant      *state,
                      gpointer       user_data)
{
  GtkTextView *text = g_object_get_data (user_data, "bloatpad-text");
  const gchar *str;

  str = g_variant_get_string (state, NULL);

  if (g_str_equal (str, "left"))
    gtk_text_view_set_justification (text, GTK_JUSTIFY_LEFT);
  else if (g_str_equal (str, "center"))
    gtk_text_view_set_justification (text, GTK_JUSTIFY_CENTER);
  else if (g_str_equal (str, "right"))
    gtk_text_view_set_justification (text, GTK_JUSTIFY_RIGHT);
  else
    /* ignore this attempted change */
    return;

  g_simple_action_set_state (action, state);
}

static GtkClipboard *
get_clipboard (GtkWidget *widget)
{
  return gtk_widget_get_clipboard (widget, gdk_atom_intern_static_string ("CLIPBOARD"));
}

static void
window_copy (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GtkWindow *window = GTK_WINDOW (user_data);
  GtkTextView *text = g_object_get_data ((GObject*)window, "bloatpad-text");

  gtk_text_buffer_copy_clipboard (gtk_text_view_get_buffer (text),
                                  get_clipboard ((GtkWidget*) text));
}

static void
window_paste (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  GtkWindow *window = GTK_WINDOW (user_data);
  GtkTextView *text = g_object_get_data ((GObject*)window, "bloatpad-text");

  gtk_text_buffer_paste_clipboard (gtk_text_view_get_buffer (text),
                                   get_clipboard ((GtkWidget*) text),
                                   NULL,
                                   TRUE);

}

static void
activate_clear (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GtkWindow *window = GTK_WINDOW (user_data);
  GtkTextView *text = g_object_get_data ((GObject*)window, "bloatpad-text");

  gtk_text_buffer_set_text (gtk_text_view_get_buffer (text), "", -1);
}

static void
activate_clear_all (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GtkApplication *app = GTK_APPLICATION (user_data);
  GList *iter;

  for (iter = gtk_application_get_windows (app); iter; iter = iter->next)
    g_action_group_activate_action (iter->data, "clear", NULL);
}

static void
text_buffer_changed_cb (GtkTextBuffer *buffer,
                        gpointer       user_data)
{
  GtkWindow *window = user_data;
  BloatPad *app;
  gint old_n, n;

  app = (BloatPad *) gtk_window_get_application (window);

  n = gtk_text_buffer_get_char_count (buffer);
  if (n > 0)
    {
      if (!app->quit_inhibit)
        app->quit_inhibit = gtk_application_inhibit (GTK_APPLICATION (app),
                                                     gtk_application_get_active_window (GTK_APPLICATION (app)),
                                                     GTK_APPLICATION_INHIBIT_LOGOUT,
                                                     "bloatpad can't save, so you can't logout; erase your text");
    }
  else
    {
      if (app->quit_inhibit)
        {
          gtk_application_uninhibit (GTK_APPLICATION (app), app->quit_inhibit);
          app->quit_inhibit = 0;
        }
    }

  g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (window), "clear")), n > 0);

  if (n > 0)
    {
      GSimpleAction *spellcheck;
      spellcheck = g_simple_action_new ("spell-check", NULL);
      g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (spellcheck));
    }
  else
    g_action_map_remove_action (G_ACTION_MAP (window), "spell-check");

  old_n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (buffer), "line-count"));
  n = gtk_text_buffer_get_line_count (buffer);
  g_object_set_data (G_OBJECT (buffer), "line-count", GINT_TO_POINTER (n));

  if (old_n < 3 && n == 3)
    {
      GNotification *n;
      n = g_notification_new ("Three lines of text");
      g_notification_set_body (n, "Keep up the good work!");
      g_notification_add_button (n, "Start over", "app.clear-all");
      g_application_send_notification (G_APPLICATION (app), "three-lines", n);
      g_object_unref (n);
    }
}

static GActionEntry win_entries[] = {
  { "copy", window_copy, NULL, NULL, NULL },
  { "paste", window_paste, NULL, NULL, NULL },
  { "fullscreen", activate_toggle, NULL, "false", change_fullscreen_state },
  { "busy", activate_toggle, NULL, "false", change_busy_state },
  { "justify", activate_radio, "s", "'left'", change_justify_state },
  { "clear", activate_clear, NULL, NULL, NULL }

};

static void
new_window (GApplication *app,
            GFile        *file)
{
  GtkWidget *window, *grid, *scrolled, *view;
  GtkWidget *toolbar;
  GtkToolItem *button;
  GtkWidget *sw, *box, *label;

  window = gtk_application_window_new (GTK_APPLICATION (app));
  gtk_window_set_default_size ((GtkWindow*)window, 640, 480);
  g_action_map_add_action_entries (G_ACTION_MAP (window), win_entries, G_N_ELEMENTS (win_entries), window);
  gtk_window_set_title (GTK_WINDOW (window), "Bloatpad");

  grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (window), grid);

  toolbar = gtk_toolbar_new ();
  button = gtk_toggle_tool_button_new ();
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "format-justify-left");
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.justify::left");
  gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (button));

  button = gtk_toggle_tool_button_new ();
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "format-justify-center");
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.justify::center");
  gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (button));

  button = gtk_toggle_tool_button_new ();
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "format-justify-right");
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.justify::right");
  gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (button));

  button = gtk_separator_tool_item_new ();
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (button), FALSE);
  gtk_tool_item_set_expand (GTK_TOOL_ITEM (button), TRUE);
  gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (button));

  button = gtk_tool_item_new ();
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (button), box);
  label = gtk_label_new ("Fullscreen:");
  gtk_container_add (GTK_CONTAINER (box), label);
  sw = gtk_switch_new ();
  gtk_widget_set_valign (sw, GTK_ALIGN_CENTER);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (sw), "win.fullscreen");
  gtk_container_add (GTK_CONTAINER (box), sw);
  gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (button));

  gtk_grid_attach (GTK_GRID (grid), toolbar, 0, 0, 1, 1);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_hexpand (scrolled, TRUE);
  gtk_widget_set_vexpand (scrolled, TRUE);
  view = gtk_text_view_new ();

  g_object_set_data ((GObject*)window, "bloatpad-text", view);

  gtk_container_add (GTK_CONTAINER (scrolled), view);

  gtk_grid_attach (GTK_GRID (grid), scrolled, 0, 1, 1, 1);

  if (file != NULL)
    {
      gchar *contents;
      gsize length;

      if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
        {
          GtkTextBuffer *buffer;

          buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
          gtk_text_buffer_set_text (buffer, contents, length);
          g_free (contents);
        }
    }
  g_signal_connect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)), "changed",
                    G_CALLBACK (text_buffer_changed_cb), window);
  text_buffer_changed_cb (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)), window);

  gtk_widget_show_all (GTK_WIDGET (window));
}



static void
new_activated (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_activate (app);
}

static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  gtk_show_about_dialog (NULL,
                         "program-name", "Bloatpad",
                         "title", "About Bloatpad",
                         "comments", "Not much to say, really.",
                         NULL);
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_quit (app);
}

static void
combo_changed (GtkComboBox *combo,
               gpointer     user_data)
{
  GtkEntry *entry = g_object_get_data (user_data, "entry");
  const gchar *action;
  gchar **accels;
  gchar *str;

  action = gtk_combo_box_get_active_id (combo);

  if (!action)
    return;

  accels = gtk_application_get_accels_for_action (gtk_window_get_application (user_data), action);
  str = g_strjoinv (",", accels);
  g_strfreev (accels);

  gtk_entry_set_text (entry, str);
}

static void
response (GtkDialog *dialog,
          guint      response_id,
          gpointer   user_data)
{
  GtkEntry *entry = g_object_get_data (user_data, "entry");
  GtkComboBox *combo = g_object_get_data (user_data, "combo");
  const gchar *action;
  const gchar *str;
  gchar **accels;

  if (response_id == GTK_RESPONSE_CLOSE)
    {
      gtk_widget_destroy (GTK_WIDGET (dialog));
      return;
    }

  action = gtk_combo_box_get_active_id (combo);

  if (!action)
    return;

  str = gtk_entry_get_text (entry);
  accels = g_strsplit (str, ",", 0);

  gtk_application_set_accels_for_action (gtk_window_get_application (user_data), action, (const gchar **) accels);
  g_strfreev (accels);
}

static void
edit_accels (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GtkApplication *app = user_data;
  GtkWidget *combo;
  GtkWidget *entry;
  gchar **actions;
  GtkWidget *dialog;
  gint i;

  dialog = gtk_dialog_new ();
  gtk_window_set_application (GTK_WINDOW (dialog), app);
  actions = gtk_application_list_action_descriptions (app);
  combo = gtk_combo_box_text_new ();
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), combo);
  for (i = 0; actions[i]; i++)
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), actions[i], actions[i]);
  g_signal_connect (combo, "changed", G_CALLBACK (combo_changed), dialog);
  entry = gtk_entry_new ();
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), entry);
  gtk_dialog_add_button (GTK_DIALOG (dialog), "Close", GTK_RESPONSE_CLOSE);
  gtk_dialog_add_button (GTK_DIALOG (dialog), "Set", GTK_RESPONSE_APPLY);
  g_signal_connect (dialog, "response", G_CALLBACK (response), dialog);
  g_object_set_data (G_OBJECT (dialog), "combo", combo);
  g_object_set_data (G_OBJECT (dialog), "entry", entry);

  gtk_widget_show_all (dialog);
}

static gboolean
update_time (gpointer user_data)
{
  BloatPad *bloatpad = user_data;
  GDateTime *now;
  gchar *time;

  while (g_menu_model_get_n_items (G_MENU_MODEL (bloatpad->time)))
    g_menu_remove (bloatpad->time, 0);

  g_message ("Updating the time menu (which should be open now)...");

  now = g_date_time_new_now_local ();
  time = g_date_time_format (now, "%c");
  g_menu_append (bloatpad->time, time, NULL);
  g_date_time_unref (now);
  g_free (time);

  return G_SOURCE_CONTINUE;
}

static void
time_active_changed (GSimpleAction *action,
                     GVariant      *state,
                     gpointer       user_data)
{
  BloatPad *bloatpad = user_data;

  if (g_variant_get_boolean (state))
    {
      if (!bloatpad->timeout)
        {
          bloatpad->timeout = g_timeout_add (1000, update_time, bloatpad);
          update_time (bloatpad);
        }
    }
  else
    {
      if (bloatpad->timeout)
        {
          g_source_remove (bloatpad->timeout);
          bloatpad->timeout = 0;
        }
    }

  g_simple_action_set_state (action, state);
}

static GActionEntry app_entries[] = {
  { "new", new_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL },
  { "edit-accels", edit_accels },
  { "time-active", NULL, NULL, "false", time_active_changed },
  { "clear-all", activate_clear_all }
};

static void
dump_accels (GtkApplication *app)
{
  gchar **actions;
  gint i;

  actions = gtk_application_list_action_descriptions (app);
  for (i = 0; actions[i]; i++)
    {
      gchar **accels;
      gchar *str;

      accels = gtk_application_get_accels_for_action (app, actions[i]);

      str = g_strjoinv (",", accels);
      g_print ("%s -> %s\n", actions[i], str);
      g_strfreev (accels);
      g_free (str);
    }
  g_strfreev (actions);
}


int
main (int argc, char **argv)
{
  BloatPad *bloat_pad;
  int status;
  const gchar *accels[] = { "F11", NULL };

  bloat_pad = bloat_pad_new ();

  gtk_application_set_accels_for_action (GTK_APPLICATION (bloat_pad),
                                         "win.fullscreen", accels);

  status = g_application_run (G_APPLICATION (bloat_pad), argc, argv);

  g_object_unref (bloat_pad);

  return status;
}


#endif
