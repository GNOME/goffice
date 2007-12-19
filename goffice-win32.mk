if WITH_WIN32

noinst_DATA = local.def

local.def: stamp-local.def
	@true

# note the [<space><tab>] in the regular expressions below
#     some versions of sed on win32 do not support the \t escape.
stamp-local.def: $(LIB_PUBLIC_HDRS) Makefile $(top_srcdir)/tools/dumpdef.pl
	hdrs='$(LIB_PUBLIC_HDRS)'; \
	hdrs_list=''; \
	for hdr in $$hdrs; do \
	  if test -f $(srcdir)/$$hdr; then \
	    hdrs_list="$$hdrs_list $(srcdir)/$$hdr"; \
	  else \
	    hdrs_list="$$hdrs_list $$hdr"; \
	  fi; \
	done; \
	cat $(top_builddir)/goffice/goffice-config.h $$hdrs_list | \
		sed -e 's/^#[ 	]*include[ 	]\+.*$$//g' | \
		$(CPP) $(AM_CPPFLAGS) "-DGO_VAR_DECL=__declspec(dllexport)" -P - > xgen-localdef.1 && \
	perl $(top_srcdir)/tools/dumpdef.pl \
		xgen-localdef.1 > xgen-localdef.2 \
	&& (cmp -s xgen-localdef.2 local.def || \
		cp xgen-localdef.2 local.def) \
	&& rm -f xgen-localdef.1 xgen-localdef.2 \
	&& echo timestamp > $@	

endif
