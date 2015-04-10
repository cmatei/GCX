/*******************************************************************************
  Copyright(c) 2011 Matei Conovici. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Contact Information: mconovici@gmail.com
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "gcx.h"
#include "gcximageview.h"
#include "params.h"
#include "obsdata.h"
#include "catalogs.h"
#include "sourcesdraw.h"
#include "gui.h"
#include "wcs.h"
#include "demosaic.h"
#include "interface.h"
#include "skyview.h"

struct _GcxSkyviewQueryPrivate {
	GcxImageView *iv;

	GtkWidget *target_entry;
	GtkWidget *resolver_combotext;
	GtkWidget *pixels_entry;
	GtkWidget *size_entry;
	GtkWidget *survey_combotext;
	GtkWidget *rotation_entry;
	GtkWidget *sampler_combotext;
	GtkWidget *fetch_toggle;
};

struct _GcxSkyviewQuery {
	GtkWindow parent_instance;

	GcxSkyviewQueryPrivate *priv;
};

struct _GcxSkyviewQueryClass {
	GtkWindowClass parent_class;
};

G_DEFINE_TYPE_WITH_PRIVATE(GcxSkyviewQuery, gcx_skyview_query, GTK_TYPE_WINDOW);

static void skyview_fetch_button_toggled(GtkWidget *button, gpointer window);

static void
gcx_skyview_query_init (GcxSkyviewQuery *self)
{
	gtk_widget_init_template (GTK_WIDGET (self));

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GCX_TYPE_SKYVIEW_QUERY, GcxSkyviewQueryPrivate);
}

static void
gcx_skyview_query_class_init (GcxSkyviewQueryClass *klass)
{
	gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
						     "/org/gcx/ui/skyview.ui");

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       target_entry);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       resolver_combotext);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       pixels_entry);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       size_entry);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       survey_combotext);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       rotation_entry);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       sampler_combotext);

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass),
						       GcxSkyviewQuery,
						       fetch_toggle);

	gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
						 skyview_fetch_button_toggled);
}

GtkWidget *
gcx_skyview_query_new(GcxImageView *iv)
{
	GcxSkyviewQuery *sq = GCX_SKYVIEW_QUERY (g_object_new (GCX_TYPE_SKYVIEW_QUERY, NULL));

//	if (iv) {
//		sq->priv->iv = iv;
//		g_object_ref (iv);
//	}


	return GTK_WIDGET (sq);
}

static char *
__uri_for_query (GcxSkyviewQueryPrivate *priv)
{
	char *q_position, *q_pixels = NULL, *q_size = NULL, *q_rotation = NULL;
	char *uri = NULL;
	char **strv;

	/* position: non-empty */
	q_position = (char *) gtk_entry_get_text (GTK_ENTRY (priv->target_entry));
	q_position = (strlen (q_position) > 0) ? g_strdup (q_position) : NULL;

	/* pixels: n or n, m */
	strv = g_strsplit (gtk_entry_get_text (GTK_ENTRY (priv->pixels_entry)), ",", -1);
	if (g_strv_length (strv) == 1) {
		q_pixels = g_strdup_printf ("%d", atoi (strv[0]));
	} else if (g_strv_length (strv) == 2) {
		q_pixels = g_strdup_printf ("%d,%d", atoi(strv[0]), atoi(strv[1]));
	}
	g_strfreev (strv);

	/* size: n or n, m */
	strv = g_strsplit (gtk_entry_get_text (GTK_ENTRY (priv->size_entry)), ",", -1);
	if (g_strv_length (strv) == 1) {
		q_size = g_strdup_printf ("%f", strtod(strv[0], NULL));
	} else if (g_strv_length (strv) == 2) {
		q_size = g_strdup_printf ("%f,%f", strtod(strv[0], NULL), strtod(strv[1], NULL));
	}
	g_strfreev (strv);

	/* rotation */
	q_rotation = g_strdup_printf ("%f", strtod (gtk_entry_get_text (GTK_ENTRY (priv->rotation_entry)), NULL));

	if (!q_position || !q_pixels || !q_size)
		goto out;


	uri = g_strdup_printf("%s?Position='%s'&Resolver=%s&Pixels=%s&Size=%s&Survey=%s&Rotation=%s&Sampler=%s&"
			      "Coordinates=J2000&Projection=Tan&float=on&Scaling=linear&return=filename",
			      P_STR(QUERY_SKYVIEW_RUNQUERY_URL),
			      q_position,
			      gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (priv->resolver_combotext)),
			      q_pixels,
			      q_size,
			      gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (priv->survey_combotext)),
			      q_rotation,
			      gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (priv->sampler_combotext)));

out:
	g_free (q_position);
	g_free (q_pixels);
	g_free (q_size);
	g_free (q_rotation);

	return uri;
}

static void
skyview_fetch_button_toggled(GtkWidget *button, gpointer window)
{
	GcxSkyviewQuery *sq = GCX_SKYVIEW_QUERY (window);
	GtkWidget *fetch_toggle = sq->priv->fetch_toggle;
	gchar *uri;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (fetch_toggle))) {
		/* fetch */

		if ((uri = __uri_for_query (sq->priv)) == NULL) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fetch_toggle), FALSE);
			return;
		}

		d3_printf("uri=%s\n", uri);

		gtk_button_set_label (GTK_BUTTON (fetch_toggle), _("Cancel"));
	} else {
		/* cancel */

		gtk_button_set_label (GTK_BUTTON (fetch_toggle), _("Fetch"));
		d3_printf("cancel!\n");
	}


	return;
}


static void delete_download(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	g_return_if_fail(data != NULL);
	g_object_set_data(G_OBJECT(data), "download", NULL);
}

static int logw_print(char *msg, void *data)
{
	GtkWidget *logw = data;
	GtkWidget *text;
	GtkToggleButton *stopb;

	text = g_object_get_data(G_OBJECT(logw), "query_log_text");
	g_return_val_if_fail(text != NULL, 0);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text), GTK_WRAP_CHAR);

	gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
					 msg, -1);

	while (gtk_events_pending())
		gtk_main_iteration();

	stopb = g_object_get_data(G_OBJECT(logw), "query_stop_toggle");
	if (gtk_toggle_button_get_active(stopb)) {
		gtk_toggle_button_set_active(stopb, 0);
		return 1;
	}
	return 0;
}

static int fetch_skyview_image(GtkWidget *window, const char *position, const char *survey, const char *field)
{
	GtkWidget *logw, *im_window;
	char *epos, *esurvey, *efield;
	char cmd[1024];
	char *fn = NULL;
	FILE *vq;
	fd_set fds;
	struct timeval tv;
	int ret;
	char *p, *line;
	size_t ll;
	struct ccd_frame *fr;

	ll = 256;
	line = malloc(ll);
	line[0] = 0;

	epos    = g_uri_escape_string(position, NULL, FALSE);
	esurvey = g_uri_escape_string(survey, NULL, FALSE);
	efield  = g_uri_escape_string(field, NULL, FALSE);

	snprintf(cmd, 1024, "%s -q -O - \"%s?Position='%s'"
		 "&survey=%s&Size=%s&Pixels=%d&coordinates=J2000"
		 "&projection=Tan&float=on&scaling=Linear"
		 "&resolver=SIMBAD-NED&Sampler=LI"
		 "&return=filename\"",
		 P_STR(QUERY_WGET),
		 P_STR(QUERY_SKYVIEW_RUNQUERY_URL),
		 epos, esurvey, efield,
		 P_INT(QUERY_SKYVIEW_PIXELS));

	g_free(epos); g_free(esurvey); g_free(efield);

	vq = popen(cmd, "r");
	if (vq == NULL) {
		err_printf("cannot run wget (%s)\n", strerror(errno));
		return -1;
	}

	/* FIXME: should probably use g_spawn_async here, and refactor
	   query.c to use the same mechanism */
	logw = create_query_log_window();
	g_object_set_data_full(G_OBJECT(window), "download",
			       logw, (GDestroyNotify)(gtk_widget_destroy));
	g_signal_connect (G_OBJECT (logw), "delete_event",
			  G_CALLBACK (delete_download), window);
	gtk_widget_show(logw);


	logw_print("Running SkyView query ", logw);

	do {
		FD_ZERO(&fds);
		FD_SET(fileno(vq), &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 500000;
		errno = 0;
		ret = select(fileno(vq) + 1, &fds, NULL, NULL, &tv);
		if (ret == 0 || errno || !FD_ISSET(fileno(vq), &fds)) {
			if (logw_print(".", logw))
				break;

			continue;
		}
		ret = getline(&line, &ll, vq);
		if (ret <= 0)
			continue;
	} while (ret >= 0);

	pclose(vq);

	logw_print("\n", logw);

	if ((p = strchr(line, '\n')) != NULL)
		*p = 0;

	if (strncasecmp(line, "skv", 3) || strlen(line) > 16)
		goto err_out;

	/* destination filename */
	snprintf(cmd, 1024, "%s/%s.fits", P_STR(QUERY_SKYVIEW_DIR), line);
	fn = strdup(cmd);

	snprintf(cmd, 1024, "Fetching to %s.fits ", line);
	logw_print(cmd, logw);

	snprintf(cmd, 1024, "%s -q -O %s %s/%s.fits",
		 P_STR(QUERY_WGET), fn,
		 P_STR(QUERY_SKYVIEW_TEMPSPACE_URL), line);

	vq = popen(cmd, "r");
	if (vq == NULL) {
		err_printf("cannot run wget (%s)\n", strerror(errno));
		goto err_out;
	}

	ll = 256;
	line[0] = 0;

	do {
		FD_ZERO(&fds);
		FD_SET(fileno(vq), &fds);
		tv.tv_sec = 0;
		tv.tv_usec = 200000;
		errno = 0;
		ret = select(fileno(vq) + 1, &fds, NULL, NULL, &tv);
		if (ret == 0 || errno || !FD_ISSET(fileno(vq), &fds)) {
			if (logw_print(".", logw))
				break;

			continue;
		}

		/* consume the event, if any */
		ret = getline(&line, &ll, vq);
		if (ret <= 0)
			continue;
	} while (ret >= 0);

	pclose(vq);

	/* load it in the im_window */
	im_window = g_object_get_data (G_OBJECT(window), "im_window");
	fr = read_image_file(fn, P_STR(FILE_UNCOMPRESS), P_INT(FILE_UNSIGNED_FITS),
			     default_cfa[P_INT(FILE_DEFAULT_CFA)]);
	if (fr == NULL) {
		err_printf_sb2(im_window, "Error opening %s", fn);
		goto err_out;
	}

	rescan_fits_wcs(fr, &fr->fim);
	rescan_fits_exp(fr, &fr->exp);
	frame_to_window (fr, im_window);
	release_frame(fr);

extern void set_last_open(gpointer object, char *file_class, char *path);

	set_last_open(im_window, "last_open_fits", strdup(fn));

	/* possibly remove the file */
	if (!P_INT(QUERY_SKYVIEW_KEEPFILES))
		unlink(fn);

	/* meh */
	ret = 0;
	goto out;

err_out:
	ret = -1;

out:
	g_object_set_data(G_OBJECT(window), "download", NULL);
	free(line);
	free(fn);

	return ret;

}


static gboolean skyview_close_window (GtkWidget *widget, GdkEvent *event, gpointer im_window)
{
	g_object_set_data (G_OBJECT(im_window), "skyview", NULL);
	return FALSE;
}

static void skyview_cancel(GtkWidget *button, gpointer window)
{
	GtkWidget *im_window = g_object_get_data (G_OBJECT(window), "im_window");

	if (im_window)
		g_object_set_data (G_OBJECT(im_window), "skyview", NULL);

	gtk_widget_destroy (window);
}

#if 0

static void skyview_fetch_stop_cb(GtkWidget *button, gpointer window)
{
	GcxSkyviewQuery *skyview = GCX_SKYVIEW_QUERY (window);
	d3_printf("fetch/stop!\n");

	g_warn_if_fail (GCX_IS_SKYVIEW_QUERY (skyview));

	d3_printf("target=%s\n", gtk_entry_get_text(GTK_ENTRY(skyview->priv->target_entry)));

	return;
	pos   = gtk_entry_get_text (GTK_ENTRY(wpos));
	field = gtk_entry_get_text (GTK_ENTRY(wfield));

	model = gtk_combo_box_get_model (GTK_COMBO_BOX(wcombo));
	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(wcombo), &iter))
		return;

	gtk_tree_model_get_value(model, &iter, 1, &val);
	survey = g_value_get_string(&val);

	if (!fetch_skyview_image(window, pos, survey, field)) {
		skyview_cancel(button, window);
	}

	g_value_unset (&val);
}

#endif

#if 0
static GtkWidget *create_skyview(gpointer window)
{
	GtkBuilder *builder;
	GtkWidget *skyview;
	static char *widgets[] = { "skyview_position", "skyview_fieldsize",
				   "skyview_survey", "skyview_survey_store",
				   "skyview_fetch", "skyview_cancel",
				   NULL };
	int i;
	GError *err = NULL;

	builder = gtk_builder_new ();

	gtk_builder_add_from_string (builder, skyview_xml, strlen(skyview_xml), &err);

	skyview  = GTK_WIDGET (gtk_builder_get_object (builder, "skyview"));

	for (i = 0; widgets[i]; i++) {
		GObject *o = gtk_builder_get_object (builder, widgets[i]);
		g_object_set_data (G_OBJECT(skyview), widgets[i], o);
	}

	GtkComboBox *combo = GTK_COMBO_BOX (g_object_get_data (G_OBJECT(skyview), "skyview_survey"));

	GtkCellRenderer * cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell, TRUE );
	gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( combo ), cell, "text", 0, NULL );


	g_object_unref (builder);

	g_object_set_data (G_OBJECT(window), "skyview", skyview);
	g_object_set_data (G_OBJECT(skyview), "im_window", window);

	g_signal_connect(G_OBJECT(skyview), "delete-event",
			 G_CALLBACK(skyview_close_window), window);

	g_signal_connect(G_OBJECT(g_object_get_data(G_OBJECT(skyview), "skyview_fetch")),
			 "clicked",
			 G_CALLBACK(skyview_fetch), skyview);

	g_signal_connect(G_OBJECT(g_object_get_data(G_OBJECT(skyview), "skyview_cancel")),
			 "clicked",
			 G_CALLBACK(skyview_cancel), skyview);

	/* fill in defaults */
	struct wcs *wcs = g_object_get_data(G_OBJECT(window), "wcs_of_window");
	if (wcs) {
		static char buf[64];
		snprintf(buf, 64, "%.5f, %.5f", wcs->xref, wcs->yref);
		gtk_entry_set_text (GTK_ENTRY(g_object_get_data(G_OBJECT(skyview), "skyview_position")), buf);
	}

	gtk_entry_set_text (GTK_ENTRY(g_object_get_data(G_OBJECT(skyview), "skyview_fieldsize")), "1.5");

	return skyview;
}

#endif

void act_download_skyview(GtkAction *action, gpointer window)
{
	GtkWidget *dialog;

	dialog = g_object_get_data (G_OBJECT(window), "skyview");
	if (dialog == NULL) {
		dialog = gcx_skyview_query_new (NULL);
		gtk_widget_show_all (dialog);
	} else {
		gtk_widget_show (dialog);
		gdk_window_raise (gtk_widget_get_window(GTK_WIDGET(dialog)));
	}
}
