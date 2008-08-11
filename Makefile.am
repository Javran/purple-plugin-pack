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
	plugin_pack.list \
	plugin_pack.m4 \
	plugin_pack.py \
	plugin_pack.spec \
	plugin_pack.stats \
	pp_config.h.mingw \
	win_pp.mak \
	po/Makefile.in.in \
	po/Makefile.mingw

if INSTALL_I18N
PO_DIR=po
endif

DIST_SUBDIRS = common doc po $(PP_DIST_DIRS)

SUBDIRS = common doc $(PO_DIR) $(PP_BUILD_DIRS)

DISTCLEANFILES=\
	pp_config.h \
	intltool-extract \
	intltool-update \
	intltool-merge \
	plugin_pack.list \
	plugin_pack.stats

BUILT_SOURCES = pp_config.h plugin_pack.list plugin_pack.stats

$(OBJECTS): $(BUILT_SOURCES)

pp_config.h: pre_config.h
	cp -f po/Makefile po/.tmp-Makefile
	sed -e "s#localedir = .*#localedir = $(PURPLE_DATADIR)/locale#g" < po/.tmp-Makefile > po/Makefile
	rm -f po/.tmp-Makefile
	$(sedpath) 's/\#define PACKAGE/\#define PP_PACKAGE/g' pre_config.h > $@

signatures: dist
	echo $(DIST_ARCHIVES) | xargs -n 1 gpg -a -b

plugin_pack.stats: plugin_pack.py $(SUBDIRS)
	@$(PYTHON) $(top_srcdir)/plugin_pack.py stats > $@

plugin_pack.list: plugin_pack.py $(SUBDIRS)
	@$(PYTHON) $(top_srcdir)/plugin_pack.py list > $@
