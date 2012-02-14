AM_CPPFLAGS = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(GOFFICE_CFLAGS)		\
	$(GTK_MAC_CFLAGS)		\
	-DGOFFICE_COMPILATION

GOFFICE_PLUGIN_FLAGS = $(GOFFICE_PLUGIN_LDFLAGS)
goffice_include_dir = $(includedir)/libgoffice-@GOFFICE_API_VER@/goffice

include $(top_srcdir)/goffice-win32.mk

all-local: list_of_sources

list_of_sources:
	rm -f $@.tmp
	touch $@.tmp
	for i in $(SOURCES) $(HEADERS); do \
		echo $$i | grep '\.[ch]$$' >> $@.tmp; \
	done
	test -z "${SUBDIRS}" || for s in ${SUBDIRS}; do \
		test -e $$s/list_of_sources && \
		perl -pe "s{^}{$$s/}" $$s/list_of_sources >> $@.tmp; \
	done
	mv $@.tmp $@
