/*******************************************************************************
  Copyright(c) 2015 Matei Conovici. All rights reserved.

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

// gcx.c: main application object

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "gcx.h"
#include "params.h"

struct _Gcx {
	GObject parent_instance;

	GtkWidget *image_window;
};

struct _GcxClass {
	GObjectClass parent_class;
};


G_DEFINE_TYPE(Gcx, gcx, G_TYPE_OBJECT);



static void
gcx_init (Gcx *gcx)
{
}


static void
gcx_class_init (GcxClass *klass)
{
}

Gcx *
gcx_new ()
{
	return GCX(g_object_new (GCX_TYPE_GCX, NULL));
}


static int
load_par_file(char *fn, GcxPar p)
{
	FILE *fp;
	int ret;

	fp = fopen(fn, "r");
	if (fp == NULL) {
		err_printf("load_par_file: cannot open %s\n", fn);
		return -1;
	}

	ret = fscan_params(fp, p);
	fclose(fp);
	return ret;
}

static int
save_par_file(char *fn, GcxPar p)
{
	FILE *fp;
	fp = fopen(fn, "w");
	if (fp == NULL) {
		err_printf("save_par_file: cannot open %s\n", fn);
		return -1;
	}
	fprint_params(fp, p);
	fclose(fp);
	return 0;
}

int
gcx_load_rcfile (Gcx *gcx, char *filename)
{
	uid_t my_uid;
	struct passwd *passwd;
	char *rcname;
	int ret = -1;

	if (filename) {
		ret = load_par_file (filename, PAR_NULL);
	} else {
		my_uid = getuid();
		passwd = getpwuid(my_uid);
		if (passwd == NULL) {
			err_printf("load_params_rc: cannot determine home directoy\n");
			return -1;
		}
		if (-1 != asprintf(&rcname, "%s/.gcxrc", passwd->pw_dir)) {
			ret = load_par_file(rcname, PAR_NULL);
			free(rcname);
		}
	}
	return ret;
}

int
gcx_save_rcfile (Gcx *gcx)
{
	uid_t my_uid;
	struct passwd *passwd;
	char *rcname;
	int ret = -1;

	my_uid = getuid();
	passwd = getpwuid(my_uid);
	if (passwd == NULL) {
		err_printf("save_params_rc: cannot determine home directoy\n");
		return -1;
	}
	if (-1 != asprintf(&rcname, "%s/.gcxrc", passwd->pw_dir)) {
		ret = save_par_file(rcname, PAR_NULL);
		free(rcname);
	}
	return ret;
}
