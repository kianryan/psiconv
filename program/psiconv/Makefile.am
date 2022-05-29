INCLUDES=-I../.. -I../../lib -I../../compat

bin_PROGRAMS = psiconv
psiconv_SOURCES = psiconv.c general.c magick-aux.c \
		  gen_txt.c gen_image.c gen_xhtml.c gen_html4.c

psiconv_LDADD = ../../lib/psiconv/libpsiconv.la @LIB_MAGICK@ @LIB_DMALLOC@
psiconv_noinstHEADERS = gen.h psiconv.h magick-aux.h general.h
man1_MANS = psiconv.man

EXTRA_DIST = gen.h psiconv.h magick-aux.h general.h psiconv.man
