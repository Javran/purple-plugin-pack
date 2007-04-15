AC_INIT([purple-plugin_pack], [1.0mtn], [guifications-devel@lists.sourceforge.net])
AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(pre_config.h)

AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PREREQ([2.50])

AC_DEFINE_UNQUOTED(PP_VERSION, "AC_PACKAGE_VERSION", [Plugin Pack Version])

AC_PATH_PROG(sedpath, sed)
AC_DISABLE_STATIC

dnl This disables check of libtool for these compilers
CXX=no
F77=no
GCJ=no

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
ALL_LINGUAS="en_AU es_ES"
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

PURPLE_LIBDIR=`pkg-config --variable=libdir purple`
PURPLE_DATADIR=`pkg-config --variable=datadir purple`

if test x"$PURPLE_LIBDIR" != x"" ; then
	PURPLE_LIBDIR="$PURPLE_LIBDIR/purple"
fi

if test x"$PURPLE_DATADIR" != x"" ; then
	PURPLE_DATADIR="$PURPLE_DATADIR/purple"
fi

AC_SUBST(PURPLE_CFLAGS)
AC_SUBST(PURPLE_LIBS)
AC_SUBST(PURPLE_LIBDIR)
AC_SUBST(PURPLE_DATADIR)

dnl #######################################################################
dnl # Check for pidgin
dnl #######################################################################
PKG_CHECK_MODULES(PIDGIN, pidgin,
[
	AC_DEFINE(HAVE_PIDGIN, 1, [Define if we've found pidgin.])
	HAVE_PIDGIN="yes"
	AM_CONDITIONAL(HAVE_PIDGIN, true)
], [
	HAVE_PIDGIN="no"
	AM_CONDITIONAL(HAVE_PIDGIN, false)
])

PIDGIN_LIBDIR=`pkg-config --variable=libdir pidgin`
PIDGIN_DATADIR=`pkg-config --variable=datadir pidgin`

if test x"$PIDGIN_LIBDIR" != x"" ; then
	PIDGIN_LIBDIR="$PIDGIN_LIBDIR/pidgin"
fi

if test x"$PIDGIN_DATADIR" != x"" ; then
	PIDGIN_DATADIR="$PIDGIN_DATADIR/pidgin"
fi

AC_SUBST(PIDGIN_CFLAGS)
AC_SUBST(PIDGIN_LIBS)
AC_SUBST(PIDGIN_LIBDIR)
AC_SUBST(PIDGIN_DATADIR)

dnl #######################################################################
dnl # Check for finch
dnl #######################################################################
PKG_CHECK_MODULES(FINCH, finch,
[
	AC_DEFINE(HAVE_FINCH, 1, [Define if we've found finch.])
	HAVE_FINCH="yes"
	AM_CONDITIONAL(HAVE_FINCH, true)
], [
	HAVE_FINCH="no"
	AM_CONDITIONAL(HAVE_FINCH, false)
])

FINCH_LIBDIR=`pkg-config --variable=libdir finch`
FINCH_DATADIR=`pkg-config --variable=datadir finch`

if test x"$FINCH_LIBDIR" != x"" ; then
	FINCH_LIBDIR="$FINCH_LIBDIR/finch"
fi

if test x"$FINCH_DATADIR" != x"" ; then
	FINCH_DATADIR="$FINCH_DATADIR/finch"
fi

AC_SUBST(FINCH_CFLAGS)
AC_SUBST(FINCH_LIBS)
AC_SUBST(FINCH_LIBDIR)
AC_SUBST(FINCH_DATADIR)

dnl #######################################################################
dnl # check for gtk and other dependencies
dnl #######################################################################
AM_PATH_GLIB_2_0(2.0.0,,AC_MSG_ERROR([
*** GLib 2.0 is required to build AC_PACKAGE_NAME; please make sure you have the
*** GLib development headers installed. The latest version of GLib is always
*** available at http://www.gtk.org/.]))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AM_PATH_GTK_2_0(2.0.0,,AC_MSG_ERROR([
*** GTK+ 2.0 is required to build AC_PACKAGE_NAME; please make sure you have the
*** GTK+ development headers installed. The latest version of GTK+ is always
*** available at http://www.gtk.org/.]))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl #######################################################################
dnl # Plugin dependency checking
dnl #######################################################################

# talkfilters
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
fi

# switchspell
gtkspell=yes
PKG_CHECK_MODULES(GTKSPELL, gtkspell-2.0 >= 2.0.2, , [gtkspell=no])
AC_SUBST(GTKSPELL_CFLAGS)
AC_SUBST(GTKSPELL_LIBS)
AM_CONDITIONAL(HAVE_GTKSPELL, test x"$gtkspell" = x"yes")

# xmmsremote
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
	else
		AC_MSG_RESULT([no])
	fi
fi
AM_CONDITIONAL(USE_XMMS, test x"$HAVE_XMMS" = x"yes")
AC_SUBST(XMMS_LIBS)
AC_SUBST(XMMS_CFLAGS)

AC_CHECK_HEADERS(regex.h)

dnl #######################################################################
dnl # Run our plugin checking
dnl #######################################################################
AM_BUILD_PLUGIN_LIST()

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
dnl # Finish up
dnl #######################################################################
AC_OUTPUT([
	Makefile
	common/Makefile
	doc/Makefile
	m4/Makefile
	po/Makefile.in
	VERSION
	plugin_pack.spec
	album/Makefile
	autorejoin/Makefile
	awaynotify/Makefile
	bash/Makefile
	bit/Makefile
	blistops/Makefile
	broadcast/Makefile
	buddytime/Makefile
	chronic/Makefile
	dice/Makefile
	difftopic/Makefile
	eight_ball/Makefile
	flip/Makefile
	groupmsg/Makefile
	gRIM/Makefile
	ignorance/Makefile
	irchelper/Makefile
	irssi/Makefile
	lastseen/Makefile
	listhandler/Makefile
	mystatusbox/Makefile
	napster/Makefile
	nicksaid/Makefile
	oldlogger/Makefile
	plonkers/Makefile
	schedule/Makefile
	sepandtab/Makefile
	showoffline/Makefile
	simfix/Makefile
	slashexec/Makefile
	sslinfo/Makefile
	stocker/Makefile
	switchspell/Makefile
	talkfilters/Makefile
	xchat-chats/Makefile
	xmmsremote/Makefile
	xmmsremote/pixmaps/Makefile
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
	if test x"$PP_PURPLE_BUILD" = x"" ; then
		echo Purple plugins to be built.......: none
	else
		echo Purple plugins to be built.......:
		echo $PP_PURPLE_BUILD | xargs -n 4 echo "	"
	fi
fi
echo;

echo Build pidgin plugins.............: $HAVE_PIDGIN
if test x"$HAVE_PIDGIN" = x"yes" ; then
	echo Installing pidgin plugins to.....: `eval eval echo $PIDGIN_LIBDIR`
	echo Installing pidgin plugin data to.: `eval eval echo $PIDGIN_DATADIR`
	if test x"$PP_PIDGIN_BUILD" = x"" ; then
		echo Pidgin plugins to be built.......: none
	else
		echo Pidgin plugins to be built.......:
		echo $PP_PIDGIN_BUILD | xargs -n 4 echo "	"
	fi
fi
echo;

echo Build finch plugins..............: $HAVE_FINCH
if test x"$HAVE_FINCH" = x"yes" ; then
	echo Installing finch plugins to......: `eval eval echo $FINCH_LIBDIR`
	echo Installing finch plugin data to..: `eval eval echo $FINCH_DATADIR`
	if test x"$PP_FINCH_BUILD" = x"" ; then
		echo Finch plugins to be built........: none
	else
		echo Finch plugins to be built........:
		echo $PP_FINCH_BUILD | xargs -n 4 echo "	"
	fi
fi
echo;

echo Type make to compile
echo;
