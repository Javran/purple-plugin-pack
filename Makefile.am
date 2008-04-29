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
	plugin_pack.m4 \
	plugin_pack.py \
	plugin_pack.spec \
	menuconfig \
	win_pp.mak \
	po/AUTHORS \
	po/ChangeLog \
	po/Makefile.in.in \
	po/Makefile.mingw

if INSTALL_I18N
PO_DIR=po
endif

DIST_SUBDIRS = common doc po $(PP_PURPLE) $(PP_PIDGIN) $(PP_FINCH)

SUBDIRS = common doc $(PO_DIR) $(PP_PURPLE_BUILD) $(PP_PIDGIN_BUILD) $(PP_FINCH_BUILD)

DISTCLEANFILES=\
	pp_config.h \
	intltool-extract \
	intltool-update \
	intltool-merge

BUILT_SOURCES = pp_config.h

$(OBJECTS): $(BUILT_SOURCES)

pp_config.h: pre_config.h
	cp -f po/Makefile po/.tmp-Makefile
	sed -e "s#localedir = .*#localedir = $(PURPLE_DATADIR)/locale#g" < po/.tmp-Makefile > po/Makefile
	rm -f po/.tmp-Makefile
	$(sedpath) 's/\#define PACKAGE/\#define PP_PACKAGE/g' pre_config.h > $@

ACLOCAL_AMFLAGS = -I m4

signatures: dist
	echo $(DIST_ARCHIVES) | xargs -n 1 gpg -a -b

info:
	@echo $(PP_DIST_DIRS)

stats:
	@$(PYTHON) plugin_pack.py stats
