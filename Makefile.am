## Process this file with automake to produce Makefile.in

SUBDIRS = src docs

EXTRA_DIST = \
	README.vizquery \
	autogen.sh \
	gcx.glade

CLEANFILES = *~ docs/*~
DISTCLEANFILES = docs/Makefile

docs: src

realclean:
	$(MAKE) -C docs realclean

