INCLUDES=-I../.. -I../../lib -I../../compat

psiconvetcdir = @PSICONVETCDIR@
psiconvetc_DATA = psiconv.conf.eg

EXTRA_DIST=psiconv.conf.eg

install-data-hook:
	if ! test -f $(DESTDIR)$(psiconvetcdir)/psiconv.conf; then \
	  cp $(DESTDIR)$(psiconvetcdir)/psiconv.conf.eg \
	     $(DESTDIR)$(psiconvetcdir)/psiconv.conf; \
	fi
