/*******************************************************************************
  Copyright(c) 2009 Geoffrey Hausheer. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
  
  
  Contact Information: gcx@phracturedblue.com <Geoffrey Hausheer>
*******************************************************************************/

#include <gtk/gtk.h>

#include "../indi.h"
#include "../indigui.h"

static void camera_capture_cb(struct indi_prop_t *iprop, void *data)
{
	FILE *fh;
	char str[80];
	static int img_count;
	struct indi_elem_t *ielem = indi_find_first_elem(iprop);

	sprintf(str, "test%03d.fits", img_count++);
	printf("Writing: %s\n", str);
	fh = fopen(str, "w+");
	fwrite(ielem->value.blob.data, ielem->value.blob.size, 1, fh);
	fclose(fh);
}

static void find_blob_cb(struct indi_prop_t *iprop, void *data)
{
	if (iprop->type == INDI_PROP_BLOB) {
		printf("Found blob\n");
		indi_dev_enable_blob(iprop->idev, 1);
	        indi_prop_add_cb(iprop, camera_capture_cb, NULL);
	}
}

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
	gtk_main_quit ();
	return FALSE;
}


int main(int argc, char **argv) {
	struct indi_t *indi;

	gtk_init (&argc, &argv);
	indi = indi_init("localhost", 7624, "INDI_gtk");
	indi_device_add_cb(indi, "", find_blob_cb, NULL);
	g_signal_connect (G_OBJECT (indi->window), "delete_event",
		G_CALLBACK (delete_event), NULL);

	gtk_widget_show_all(GTK_WIDGET(indi->window));
	gtk_main();
	return 0;
}
