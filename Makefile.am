EXTRA_DIST = \
	AUTHORS \
	COPYING \
	ChangeLog \
	INSTALL \
	INSTALL.WIN32 \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	Makefile.am \
	Makefile.mingw \
	NEWS \
	README \
	VERSION \
	VERSION.in \
	gpp_config.h \
	gpp_config.h.mingw \
	gaim-plugin_pack.spec \
	po/AUTHORS \
	po/ChangeLog \
	po/Makefile.in.in \
	po/Makefile.mingw

DIST_SUBDIRS = common doc m4 po $(PP_DISTDIRS)

SUBDIRS = common doc m4 po $(PP_BUILDDIRS)

BUILT_SOURCES = gpp_config.h

$(OBJECTS): $(BUILT_SOURCES)

gpp_config.h: pre_config.h
	cp -f po/Makefile po/.tmp-Makefile
	sed -e "s#localedir = .*#localedir = $(GAIM_DATADIR)/locale#g" < po/.tmp-Makefile > po/Makefile
	rm -f po/.tmp-Makefile
	$(sedpath) 's/\#define PACKAGE/\#define GPP_PACKAGE/g' pre_config.h > $@

CLEANFILES=

ACLOCAL_AMFLAGS = -I m4
