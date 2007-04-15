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
	pp_config.h \
	pp_config.h.mingw \
	plugin_pack.spec \
	menuconfig \
	win_pp.mak \
	po/AUTHORS \
	po/ChangeLog \
	po/Makefile.in.in \
	po/Makefile.mingw

DIST_SUBDIRS = common doc m4 po $(PP_DIST)

SUBDIRS = common doc m4 po $(PP_BUILD)

BUILT_SOURCES = pp_config.h

$(OBJECTS): $(BUILT_SOURCES)

pp_config.h: pre_config.h
	cp -f po/Makefile po/.tmp-Makefile
	sed -e "s#localedir = .*#localedir = $(PURPLE_DATADIR)/locale#g" < po/.tmp-Makefile > po/Makefile
	rm -f po/.tmp-Makefile
	$(sedpath) 's/\#define PACKAGE/\#define PP_PACKAGE/g' pre_config.h > $@

CLEANFILES=

ACLOCAL_AMFLAGS = -I m4

plugin_info:
	@echo Building: $(PP_BUILD)
	@echo;
	@echo Disting: $(PP_DIST)
