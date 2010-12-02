
PICTS	= aucyg-res-std.gpl aucyg-wres-std.gpl v-res-mag-zponly.gpl v-se-color.gpl \
	  v-se-color-zponly.gpl zp-am-1.gpl zp-am-2.gpl zp-t-1.gpl zp-t-2.gpl

EPSPICT	= $(PICTS:.gpl=.eps)
PDFPICT = $(PICTS:.gpl=.pdf)


EXTRA_DIST	= Makefile.in gcx.ltx gpl2eps.gp $(PICTS) \
		  html gcx.pdf gcx.dvi gcx.1

man1_MANS	= gcx.1
doc_DATA	= gcx.pdf gcx.dvi

htmldir		= $(docdir)/html

all-local: gcx.pdf gcx.dvi gcx.html

install-data-hook:
	$(MKDIR_P) $(htmldir)
	$(INSTALL_DATA) html/* $(htmldir)

.gpl.eps:
	gnuplot gpl2eps.gp $^ > $@

.gpl.pdf:
	gnuplot gpl2eps.gp $^ | epstopdf -f > $@

pictures: $(EPSPICT) $(PDFPICT)
	touch pictures

options: ../src/gcx
	../src/gcx --options-doc >options.ltx
	echo '\begin{verbatim}' >cmdline.txt
	../src/gcx --help 2>>cmdline.txt
	echo '\end{verbatim}' >>cmdline.txt
	echo '\begin{verbatim}' >repconv.txt
	../src/gcx --help-rep-conv >>repconv.txt
	echo '\end{verbatim}' >>repconv.txt
	touch options

gcx.pdf: gcx.ltx options pictures
	pdflatex gcx.ltx

gcx.dvi: gcx.ltx options pictures
	latex gcx.ltx 

gcx.html: gcx.ltx options pictures
	latex2html --split=3 --dir html --mkdir gcx.ltx
	touch gcx.html

clean:
	rm -f *~ *.aux *.toc *.log *.eps *.dvi *.pdf
	rm -f gcx.html options pictures
	rm -rf html

distclean: clean

