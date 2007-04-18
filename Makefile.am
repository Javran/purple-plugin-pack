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

DIST_SUBDIRS = common doc m4 po $(PP_PURPLE) $(PP_PIDGIN) $(PP_FINCH)

SUBDIRS = common doc m4 po $(PP_PURPLE_BUILD) $(PP_PIDGIN_BUILD) $(PP_FINCH_BUILD)

BUILT_SOURCES = pp_config.h

$(OBJECTS): $(BUILT_SOURCES)

pp_config.h: pre_config.h
	cp -f po/Makefile po/.tmp-Makefile
	sed -e "s#localedir = .*#localedir = $(PURPLE_DATADIR)/locale#g" < po/.tmp-Makefile > po/Makefile
	rm -f po/.tmp-Makefile
	$(sedpath) 's/\#define PACKAGE/\#define PP_PACKAGE/g' pre_config.h > $@

CLEANFILES=

ACLOCAL_AMFLAGS = -I m4

info:
	@echo "---------------------------------------"
	@echo "Plugin Pack Info"
	@echo "---------------------------------------"
	@echo "Purple Plugins:"
	@echo "	Available..: $(PP_PURPLE)"
	@echo "	Abusive....: $(PP_PURPLE_ABUSIVE)"
	@echo "	Incomplete.: $(PP_PURPLE_INCOMPLETE)"
	@echo "	Building...: $(PP_PURPLE_BUILD)"
	@echo "---------------------------------------"
	@echo "Pidgin Plugins:"
	@echo "	Available..: $(PP_PIDGIN)"
	@echo "	Abusive....: $(PP_PIDGIN_ABUSIVE)"
	@echo "	Incomplete.: $(PP_PIDGIN_INCOMPLETE)"
	@echo "	Building...: $(PP_PIDGIN_BUILD)"
	@echo "---------------------------------------"
	@echo "Finch Plugins:"
	@echo "	Available..: $(PP_FINCH)"
	@echo "	Abusive....: $(PP_FINCH_ABUSIVE)"
	@echo "	Incomplete.: $(PP_FINCH_INCOMPLETE)"
	@echo "	Building...: $(PP_FINCH_BUILD)"
	@echo "---------------------------------------"
