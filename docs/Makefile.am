
# The pre-compiled documentation is included in the dist, so you don't
# need latex et. all installed to build it.

PICTS	= aucyg-res-std.gpl aucyg-wres-std.gpl v-res-mag-zponly.gpl v-se-color.gpl \
	  v-se-color-zponly.gpl zp-am-1.gpl zp-am-2.gpl zp-t-1.gpl zp-t-2.gpl

BASEPICTS = $(PICTS:.gpl=)
EPS_PICTS = $(PICTS:.gpl=.eps)
PDF_PICTS = $(PICTS:.gpl=.pdf)


EXTRA_DIST	= gcx.ltx gpl2eps.gp $(PICTS) \
		  html gcx.pdf gcx.dvi gcx.1

man1_MANS	= gcx.1
doc_DATA	= gcx.pdf gcx.dvi

OPTFILES	= options.ltx cmdline.txt repconv.txt

# slow and safe
.NOTPARALLEL:

all-local: gcx.dvi gcx.pdf html/index.html

# use html from build-dir (e.g. distcheck alternate dir compilation)
# if available, or fallback to srcdir
install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/html
	if test -d html; then 							\
		$(INSTALL_DATA) html/* $(DESTDIR)$(docdir)/html ;		\
	else									\
		$(INSTALL_DATA) $(srcdir)/html/* $(DESTDIR)$(docdir)/html ; 	\
	fi

uninstall-hook:
	( cd $(DESTDIR)$(docdir) && rm -rf html )

define do_extras
	../src/gcx --options-doc > options.ltx
	-echo '\begin{verbatim}' > cmdline.txt
	../src/gcx --help 2>> cmdline.txt
	-echo '\end{verbatim}' >> cmdline.txt
	-echo '\begin{verbatim}' > repconv.txt
	../src/gcx --help-rep-conv >> repconv.txt
	-echo '\end{verbatim}' >> repconv.txt

	for f in $(BASEPICTS); do 					\
		gnuplot $(srcdir)/gpl2eps.gp $$f.gpl > $$f.eps ;	\
		epstopdf $$f.eps > $$f.pdf ;				\
	done
endef


gcx.dvi: gcx.ltx
	$(do_extras)
	while ( latex $(srcdir)/gcx.ltx ; \
	        grep -q "Rerun to get cross-references right" gcx.log ) do true ; \
	done

# do these after the .dvi, so we have options, pictures, .aux and .toc
gcx.pdf: gcx.ltx gcx.dvi
	pdflatex $(srcdir)/gcx.ltx

html/index.html: gcx.ltx gcx.dvi
	latex2html --split=3 --dir html --mkdir $(srcdir)/gcx.ltx

clean-local:
	rm -f *~ *.aux *.toc *.log $(OPTFILES) $(EPS_PICTS) $(PDF_PICTS)

distclean-local: clean-local

realclean:
	rm -rf gcx.dvi gcx.pdf html

