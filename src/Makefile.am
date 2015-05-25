## Process this file with automake to produce Makefile.in

SUBDIRS = gsc

AM_CPPFLAGS = @GTK_CFLAGS@ @libcurl_CFLAGS@

AM_CFLAGS = -Wall
bin_PROGRAMS = gcx

gcx_SOURCES = \
	main.c main.h gcx.c gcx.h \
        ccd_frame.c dslr.c median.c badpix.c \
        edb.c aphot.c worldpos.c sources.c \
        warp.c errlog.c use_dcraw.c \
        ccd.h dslr.h tiff.c jpeg.c \
	cameragui.c cameragui.h catalogs.c catalogs.h \
	filegui.c filegui.h gui.c gui.h imadjust.c \
	interface.c interface.h obsdata.c obsdata.h obslist.c \
	obslist.h params.c params.h photometry.c \
	sourcesdraw.c sourcesdraw.h staredit.c \
	textgui.c wcs.c wcs.h \
	helpmsg.c helpmsg.h wcsedit.c recipy.c recipy.h recipygui.c symbols.h\
	tycho2.c tycho2.h ucac4.c ucac4.h report.c \
	astro.c astro.h \
	reduce.c reduce.h misc.c misc.h reducegui.c\
	initparams.c starlist.c	guidegui.c \
	guide.c guide.h multiband.c multiband.h mbandgui.c plots.c plots.h \
	mbandrep.c starfile.c getline.h synth.c psf.c psf.h dsimplex.c dsimplex.h \
	basename.c dirname.c libgen.h query.c query.h plate.c \
	demosaic.c demosaic.h skyview.c skyview.h \
	tele_indi.c tele_indi.h camera_indi.c camera_indi.h \
	fwheel_indi.c fwheel_indi.h focus_indi.c focus_indi.h \
	common_indi.c common_indi.h \
	gcximageview.c gcximageview.h \
	gcxparamsdialog.c gcxparamsdialog.h \
	gcxexposuredialog.c gcxexposuredialog.h \
        \
	libindiclient/lilxml.h libindiclient/lilxml.c \
	libindiclient/base64.h libindiclient/base64.c \
	libindiclient/indi.h libindiclient/indi.c \
	libindiclient/indigui.h libindiclient/gtk/indigui.c \
	libindiclient/indi_io.h libindiclient/gtk/indi_io.c \
	libindiclient/indi_config.h libindiclient/gtk/indi_config.c \
	libindiclient/indi_list.h libindiclient/gtk/indi_list.c \
	libindiclient/gtk/indisave.h libindiclient/gtk/indisave.c \
	libindiclient/gtk/gtkled.h libindiclient/gtk/gtkled.c \
	gcxresources.c

        # focus.c focus.h focusgui.c

resource_files=$(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies $(builddir)/gcx.gresource.xml)

gcxresources.c: gcx.gresource.xml $(resource_files)
	$(GLIB_COMPILE_RESOURCES) $< --target=$@ --generate-source

gcx_LDADD = gsc/libgsc.a @GTK_LIBS@ @libcurl_LIBS@

CLEANFILES = gcxresources.c *~
