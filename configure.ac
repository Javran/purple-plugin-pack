AC_INIT([purple-plugin_pack], [2.5.1mtn], [plugins-devel@lists.guifications.org])
AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(pre_config.h)

AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PREREQ([2.50])

AC_DEFINE_UNQUOTED(PP_VERSION, "AC_PACKAGE_VERSION", [Plugin Pack Version])

AC_PATH_PROG(sedpath, sed)
AC_DISABLE_STATIC

dnl #######################################################################
dnl # Initialize some variables that get passed to plugin_pack.py
dnl #######################################################################
DEPENDENCIES=""

dnl #######################################################################
dnl # Setup libtool
dnl #######################################################################
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"

dnl #######################################################################
dnl # I'm lazy and figured config.h is the best place for this ;)
dnl #######################################################################
AC_DEFINE_UNQUOTED(PP_WEBSITE, "http://purple.guifications.org/trac", [Plugin Pack Website]) 

dnl #######################################################################
dnl # Our header
dnl #######################################################################
AH_TOP([ /* our header */
#ifndef PP_CONFIG_H
#define PP_CONFIG_H
])
AH_BOTTOM([
#endif /* PP_CONFIG_H */
])

dnl #######################################################################
dnl # Good ol' gettext
dnl #######################################################################
AC_PROG_INTLTOOL

GETTEXT_PACKAGE=plugin_pack
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define the gettext package to be used.])
ALL_LINGUAS="en_AU es_ES fr vi"
AM_GLIB_GNU_GETTEXT

dnl #######################################################################
dnl # Look for the C compiler
dnl #######################################################################
CFLAGS_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$CFLAGS_save"

AC_ARG_ENABLE(debug, [  --enable-debug          compile with debugging support],,enable_debug=no)

if test "x$enable_debug" = "xyes" ; then
	AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.])
fi

if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -g3"
fi
AC_SUBST(CFLAGS)

dnl #######################################################################
dnl # Check for purple
dnl #######################################################################
PKG_CHECK_MODULES(PURPLE, purple,
[
	AC_DEFINE(HAVE_PURPLE, 1, [Define if we've found libpurple.])
])

HAVE_PURPLE="yes"
AM_CONDITIONAL(HAVE_PURPLE, true)
DEPENDENCIES="$DEPENDENCIES,purple"

if test x"$prefix" = x"NONE" ; then
	PURPLE_LIBDIR=`pkg-config --variable=libdir purple`
	PURPLE_DATADIR=`pkg-config --variable=datadir purple`
else
	PURPLE_LIBDIR="$libdir"
	PURPLE_DATADIR="$datadir"
fi

PURPLE_PIXMAPSDIR=""

# this is a hack but should work fine.
# we use the libpurple datadir for PP_LOCALEDIR since we are not breaking up
# the pot's yet, and need to make sure they goto the same place
PP_LOCALEDIR="PURPLE_DATADIR/locale"
AC_DEFINE_UNQUOTED(PP_LOCALEDIR, ["$PP_LOCALEDIR"], [The localedir to use])

if test x"$PURPLE_LIBDIR" != x"" ; then
	PURPLE_LIBDIR="$PURPLE_LIBDIR/purple-2"
fi

if test x"$PURPLE_DATADIR" != x"" ; then
	PURPLE_PIXMAPSDIR="$PURPLE_DATADIR/pixmaps/libpurple"
	AC_DEFINE_UNQUOTED(PURPLE_PIXMAPSDIR, "$PURPLE_PIXMAPSDIR", [Libpurple pixmaps directory])
fi

AC_SUBST(PURPLE_CFLAGS)
AC_SUBST(PURPLE_LIBS)
AC_SUBST(PURPLE_LIBDIR)
AC_SUBST(PURPLE_DATADIR)
AC_SUBST(PURPLE_PIXMAPSDIR)


dnl #######################################################################
dnl # Check for pidgin
dnl #######################################################################
PKG_CHECK_MODULES(PIDGIN, pidgin,
[
	AC_DEFINE(HAVE_PIDGIN, 1, [Define if we've found pidgin.])
	HAVE_PIDGIN="yes"
	AM_CONDITIONAL(HAVE_PIDGIN, true)
	DEPENDENCIES="$DEPENDENCIES,pidgin"
], [
	AC_MSG_RESULT([no])
	HAVE_PIDGIN="no"
	AM_CONDITIONAL(HAVE_PIDGIN, false)
])

if test x"$prefix" = x"NONE" ; then
	PIDGIN_LIBDIR=`pkg-config --variable=libdir pidgin`
	PIDGIN_DATADIR=`pkg-config --variable=datadir pidgin`
else
	PIDGIN_LIBDIR="$libdir"
	PIDGIN_DATADIR="$datadir"
fi

PIDGIN_PIXMAPSDIR=""

if test x"$PIDGIN_LIBDIR" != x"" ; then
	PIDGIN_LIBDIR="$PIDGIN_LIBDIR/pidgin"
fi

if test x"$PIDGIN_DATADIR" != x"" ; then
	PIDGIN_PIXMAPSDIR="$PIDGIN_DATADIR/pixmaps/pidgin"
	AC_DEFINE_UNQUOTED(PIDGIN_PIXMAPSDIR, "$PIDGIN_PIXMAPSDIR", [Pidgin pixmaps directory])
fi

AC_SUBST(PIDGIN_CFLAGS)
AC_SUBST(PIDGIN_LIBS)
AC_SUBST(PIDGIN_LIBDIR)
AC_SUBST(PIDGIN_DATADIR)
AC_SUBST(PIDGIN_PIXMAPSDIR)

dnl #######################################################################
dnl # Check for finch
dnl #######################################################################
PKG_CHECK_MODULES(FINCH, finch,
[
	AC_DEFINE(HAVE_FINCH, 1, [Define if we've found finch.])
	HAVE_FINCH="yes"
	AM_CONDITIONAL(HAVE_FINCH, true)
	DEPENDENCIES="$DEPENDENCIES,finch"
], [
	AC_MSG_RESULT([no])
	HAVE_FINCH="no"
	AM_CONDITIONAL(HAVE_FINCH, false)
])

if test x"$prefix" = x"NONE" ; then
	FINCH_LIBDIR=`pkg-config --variable=libdir finch`
	FINCH_DATADIR=`pkg-config --variable=datadir finch`
else
	FINCH_LIBDIR="$libdir"
	FINCH_DATADIR="$datadir"
fi

FINCH_PIXMAPSDIR=""

if test x"$FINCH_LIBDIR" != x"" ; then
	FINCH_LIBDIR="$FINCH_LIBDIR/finch"
fi

if test x"$FINCH_DATADIR" != x"" ; then
	FINCH_PIXMAPSDIR="$FINCH_DATADIR/pixmaps/finch"
	AC_DEFINE_UNQUOTED(FINCH_PIXMAPSDIR, "$FINCH_PIXMAPSDIR", [Finch pixmaps directory])
fi

AC_SUBST(FINCH_CFLAGS)
AC_SUBST(FINCH_LIBS)
AC_SUBST(FINCH_LIBDIR)
AC_SUBST(FINCH_DATADIR)
AC_SUBST(FINCH_PIXMAPSDIR)

dnl #######################################################################
dnl # check for gtk
dnl #######################################################################
HAVE_GLIB="no"
PKG_CHECK_MODULES(GLIB, [glib-2.0], HAVE_GLIB="yes", HAVE_GLIB="no")

if test x"$HAVE_GLIB" = x"no" ; then
	echo "glib development headers were not found.  glib development headers"
	echo "are required to build $PACKAGE."
	exit 1
fi
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

HAVE_GTK="no"
GTK_CFLAGS=""
GTK_LIBS=""
PKG_CHECK_MODULES(GTK, [gtk+-2.0], HAVE_GTK="yes", HAVE_GTK="no")
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl #######################################################################
dnl # check for gnt
dnl #######################################################################
HAVE_GNT="no"
GNT_CFLAGS=""
GNT_LIBS=""
PKG_CHECK_MODULES(GNT, [gnt], HAVE_GNT="yes", HAVE_GNT="no")
AC_SUBST(GNT_CFLAGS)
AC_SUBST(GNT_CFLAGS)

dnl #######################################################################
dnl # check for pango
dnl #######################################################################
HAVE_PANGO="no"
PANGO_CFLAGS=""
PANGO_LIBS=""
PKG_CHECK_MODULES(PANGO, [pango], HAVE_PANGO="yes", HAVE_PANGO="no")
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_CFLAGS)

if test x"$HAVE_PANGO" = x"yes" ; then
	DEPENDENCIES="$DEPENDENCIES,pango"
fi

dnl #######################################################################
dnl # check for cairo
dnl #######################################################################
HAVE_CAIRO="no"
CAIRO_CFLAGS=""
CAIRO_LIBS=""
PKG_CHECK_MODULES(CAIRO, [cairo], HAVE_CAIRO="yes", HAVE_CAIRO="no")
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_CFLAGS)

if test x"$HAVE_CAIRO" = x"yes" ; then
	DEPENDENCIES="$DEPENDENCIES,cairo"
fi

dnl #######################################################################
dnl # Check for talkfilters
dnl #######################################################################
AC_CHECK_HEADER(talkfilters.h, HAVE_TALKFILTERS=yes, AC_MSG_WARN([
*** GNU Talk Filters is required to build the talkfilters plugin;
*** please make sure you have the GNU Talk Filters development headers installed.
*** The latest version of GNU Talk Filters is available at
*** http://www.hyperrealm.com/talkfilters/talkfilters.html.])
HAVE_TALKFILTERS=no
)
AM_CONDITIONAL(USE_TALKFILTERS, test x"$HAVE_TALKFILTERS" = x"yes")
if test x"$HAVE_TALKFILTERS" = x"yes"; then
	dnl work out that the library exists
	AC_CHECK_LIB(talkfilters, gtf_filter_count, TALKFILTERS_LIBS="-ltalkfilters")
	AC_SUBST(TALKFILTERS_LIBS)

	DEPENDENCIES="$DEPENDENCIES,talkfilters"
fi

dnl #######################################################################
dnl # Check for switchspell
dnl #######################################################################
gtkspell=yes
PKG_CHECK_MODULES(GTKSPELL, gtkspell-2.0 >= 2.0.2, [], [gtkspell=no])
AC_SUBST(GTKSPELL_CFLAGS)
AC_SUBST(GTKSPELL_LIBS)

DEPENDENCIES="$DEPENDENCIES,gtkspell"

BUILD_SWITCH_SPELL=no

ASPELL_CFLAGS=""
APSELL_LIBS=""

ENCHANT_CFLAGS=""
ENCHANT_LIBS=""

if test x"$gtkspell" = x"yes" ; then
	AC_MSG_CHECKING([which backend gtkspell is compiled with])

	$PKG_CONFIG --static --libs gtkspell-2.0 | grep -q enchant
	if test $? -eq 0 ; then
		AC_MSG_RESULT([enchant])

		PKG_CHECK_MODULES(ENCHANT,
						  [enchant],
						  [BUILD_SWITCH_SPELL=yes],
						  [BUILD_SWITCH_SPELL=no])

		AC_DEFINE(HAVE_ENCHANT, 1, [define if we've found enchant])

		DEPENDENCIES="$DEPENDENCIES,enchant"
	else
		AC_MSG_RESULT([aspell])

		AC_CHECK_HEADER([aspell.h], HAVE_ASPELL_H=yes, HAVE_ASPELL_H=no)
		if test x"$HAVE_ASPELL_H" = x"yes" ; then
			AC_CHECK_LIB([aspell], [new_aspell_config],
						 [ASPELL_LIBS="-laspell"
						  BUILD_SWITCH_SPELL=yes],
						 [BUILD_SWITCH_SPELL=no])

			DEPENDENCIES="$DEPENDENCIES,aspell"
		else
			BUILD_SWITCH_SPELL=no
		fi
	fi
fi

if test x"$BUILD_SWITCH_SPELL" = x"no" ; then
	ASPELL_CFLAGS=""
	ASPELL_LIBS=""

	ENCHANT_CFLAGS=""
	ENCHANT_LIBS=""

	AM_CONDITIONAL(BUILD_SWITCH_SPELL, false)
else
	AM_CONDITIONAL(BUILD_SWITCH_SPELL, true)
fi

AC_SUBST(ASPELL_CFLAGS)
AC_SUBST(ASPELL_LIBS)

AC_SUBST(ENCHANT_CFLAGS)
AC_SUBST(ENCHANT_LIBS)

dnl #######################################################################
dnl # Check for xmms
dnl #######################################################################
XMMS_LIBS=""
XMMS_CFLAGS=""
HAVE_XMMS="no"

AC_PATH_PROG(XMMS_CONFIG, xmms-config, no)
if test x"$XMMS_CONFIG" != x"no" ; then
	AC_MSG_CHECKING([for xmms >= 1.0.0])
	# check the version of xmms config we found
	XMMS_VERSION=`$XMMS_CONFIG --version`
	if test x"$XMMS_VERSION" != x"" ; then
		XMMS_MAJOR=`echo $XMMS_VERSION | cut -d. -f1`
		XMMS_MINOR=`echo $XMMS_VERSION | cut -d. -f2`

		if test $XMMS_MAJOR -ge 1 -a $XMMS_MINOR -ge 0 ; then
			XMMS_LIBS=`$XMMS_CONFIG --libs`
			XMMS_CFLAGS=`$XMMS_CONFIG --cflags`

			HAVE_XMMS="yes"
		fi
	fi

	if test x"$HAVE_XMMS" = x"yes" ; then
		AC_MSG_RESULT([yes])

		DEPENDENCIES="$DEPENDENCIES,xmms"
	else
		AC_MSG_RESULT([no])
	fi
fi
AM_CONDITIONAL(USE_XMMS, test x"$HAVE_XMMS" = x"yes")
AC_SUBST(XMMS_LIBS)
AC_SUBST(XMMS_CFLAGS)

dnl #######################################################################
dnl # Check for some basic headers
dnl #######################################################################
AC_CHECK_HEADERS(regex.h)

dnl #######################################################################
dnl # Disable installation of translation files
dnl #######################################################################
AC_ARG_ENABLE(nls, AC_HELP_STRING([--enable-nls], [enable installation of translation files]), enable_i18n="$enableval", enable_i18n=yes)

AM_CONDITIONAL(INSTALL_I18N, test "x$enable_i18n" = "xyes")

dnl #######################################################################
dnl # Version stuff
dnl #######################################################################
AC_CONFIG_COMMANDS_PRE([
	if test -e VERSION; then
		cp -p VERSION VERSION.ac-save
	fi
])

AC_CONFIG_COMMANDS_POST([
	cmp VERSION VERSION.ac-save || touch -r VERSION.ac-save VERSION
	rm -f VERSION.ac-save
])

dnl #######################################################################
dnl # plugin_pack.py has already done our heavy lifting from the boot
dnl # strap.  So we'll include our config file it created and call it to
dnl # determine our build directories
dnl #######################################################################
AC_PATH_PROG([PYTHON], [python], [no])

dnl # include the config file we created during bootstrapping
m4_include([plugin_pack.m4])

dnl #######################################################################
dnl # Finish up
dnl #######################################################################
AC_OUTPUT([Makefile
		   common/Makefile
		   doc/Makefile
		   po/Makefile.in
		   VERSION
		   plugin_pack.spec
])

dnl #######################################################################
dnl # Ouput!!
dnl #######################################################################
echo;
echo $PACKAGE $VERSION Configuration complete
echo;
echo Debugging enabled................: $enable_debug
echo;

echo Build purple plugins.............: $HAVE_PURPLE
if test x"$HAVE_PURPLE" = x"yes" ; then
	echo Installing purple plugins to.....: `eval eval echo $PURPLE_LIBDIR`
	echo Installing purple plugin data to.: `eval eval echo $PURPLE_DATADIR`
	echo Purple plugins to be built.......:
	eval $PP_PURPLE_BUILD
fi
echo;

echo Build pidgin plugins.............: $HAVE_PIDGIN
if test x"$HAVE_PIDGIN" = x"yes" ; then
	echo Installing pidgin plugins to.....: `eval eval echo $PIDGIN_LIBDIR`
	echo Installing pidgin plugin data to.: `eval eval echo $PIDGIN_DATADIR`
	echo Pidgin plugins to be built.......:
	eval $PP_PIDGIN_BUILD
fi
echo;

echo Build finch plugins..............: $HAVE_FINCH
if test x"$HAVE_FINCH" = x"yes" ; then
	echo Installing finch plugins to......: `eval eval echo $FINCH_LIBDIR`
	echo Installing finch plugin data to..: `eval eval echo $FINCH_DATADIR`
	echo Finch plugins to be built........: none - THIS IS NORMAL
	# uncomment this when we have finch plugins
	# eval $PP_FINCH_BUILD
fi
echo;

echo Type make to compile
echo;
