INCLUDES=-I.. -I../../compat

lib_LTLIBRARIES = libpsiconv.la
libpsiconv_la_SOURCES = configuration.c error.c misc.c checkuid.c list.c \
                        buffer.c data.c image.c unicode.c \
                        parse_common.c parse_driver.c parse_formula.c \
                        parse_layout.c parse_image.c parse_page.c  \
                        parse_simple.c parse_texted.c parse_word.c \
			parse_sheet.c \
                        generate_simple.c generate_layout.c generate_driver.c \
                        generate_common.c generate_texted.c generate_page.c \
                        generate_word.c generate_image.c
libpsiconv_la_LDFLAGS = -version-info 10:2:4
libpsiconv_la_LIBADD = ../../compat/libcompat.la
libpsiconv_la_CFLAGS = -DPSICONVETCDIR=\"@PSICONVETCDIR@\"

psiconvincludedir = $(includedir)/psiconv
psiconvinclude_HEADERS = configuration.h data.h parse.h list.h \
                         parse_routines.h \
                         error.h generate_routines.h generate.h common.h \
                         buffer.h unicode.h
noinst_HEADERS = image.h
nodist_psiconvinclude_HEADERS = general.h

BUILT_SOURCES = psiconv.conf.man
man5_MANS = psiconv.conf.man
EXTRA_DIST = psiconv.conf.man.in
