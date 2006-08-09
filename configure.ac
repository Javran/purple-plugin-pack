AC_INIT([gaim-plugin_pack], [1.0svn], [guifications-devel@lists.sourceforge.net])
AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(pre_config.h)

AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PREREQ([2.50])

AC_DEFINE_UNQUOTED(GPP_VERSION, "AC_PACKAGE_VERSION", [Plugin Pack Version])

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
AC_DEFINE_UNQUOTED(GPP_WEBSITE, "http://gaim.guifications.org/trac", [Plugin Pack Website]) 

dnl #######################################################################
dnl # Our header
dnl #######################################################################

AH_TOP([ /* our header */
#ifndef GPP_CONFIG_H
#define GPP_CONFIG_H
])
AH_BOTTOM([
#endif /* GPP_CONFIG_H */
])

dnl #######################################################################
dnl # Good ol' gettext
dnl #######################################################################
ALL_LINGUAS="en_AU es_ES"
AM_GNU_GETTEXT_VERSION(0.10.40)
AM_GNU_GETTEXT

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
dnl # Check for gaim
dnl #######################################################################

PKG_CHECK_MODULES(GAIM, gaim,
[
	AC_DEFINE(HAVE_GAIM, 1, [Define if we've found gaim.])
])

AC_SUBST(GAIM_CFLAGS)
GAIM_LIBDIR=`pkg-config --variable=libdir gaim`
GAIM_DATADIR=`pkg-config --variable=datadir gaim`
AC_SUBST(GAIM_LIBDIR)
AC_SUBST(GAIM_DATADIR)

dnl #######################################################################
dnl # check for gtk and other dependencies
dnl #######################################################################

AM_PATH_GLIB_2_0(2.0.0,,AC_MSG_ERROR([
*** GLib 2.0 is required to build AC_PACKAGE_NAME; please make sure you have the
*** GLib development headers installed. The latest version of GLib is always
*** available at http://www.gtk.org/.]))
AC_SUBST(GLIB_CFLAGS)

AM_PATH_GTK_2_0(2.0.0,,AC_MSG_ERROR([
*** GTK+ 2.0 is required to build AC_PACKAGE_NAME; please make sure you have the
*** GTK+ development headers installed. The latest version of GTK+ is always
*** available at http://www.gtk.org/.]))
AC_SUBST(GTK_CFLAGS)

dnl #######################################################################
dnl # Plugin dependency checking
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
fi

AM_PATH_XMMS(1.0.0, HAVE_XMMS=yes, AC_MSG_WARN([
*** XMMS >= 1.0 is required to build Gaim-XMMS-Remote; please make sure you
*** have the XMMS development headers installed.  The latest version of XMMS
*** is always available at http://www.xmms.org/.])
HAVE_XMMS=no
)
AM_CONDITIONAL(USE_XMMS, test x"$HAVE_XMMS" = x"yes")

dnl #######################################################################
dnl # Run our plugin checking
dnl #######################################################################
AM_BUILD_PLUGIN_LIST()

dnl #######################################################################
dnl # Check if plugins are enabled
dnl #######################################################################
want_plugins=yes
AM_CONDITIONAL(PLUGINS, test "$want_plugins" = "yes")

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
	gaim-plugin_pack.spec
	common/Makefile
	doc/Makefile
	intl/Makefile
	m4/Makefile
	po/Makefile.in
	VERSION
	autoaccept/Makefile
	autoreply/Makefile
	bash/Makefile
	bit/Makefile
	blistops/Makefile
	buddynote/Makefile
	chronic/Makefile
	convcolors/Makefile
	dice/Makefile
	eight_ball/Makefile
	flip/Makefile
	gaim-schedule/Makefile
	gaim-xchat/Makefile
	gaim-xmms-remote/Makefile
	gaim-xmms-remote/pixmaps/Makefile
	groupmsg/Makefile
	gRIM/Makefile
	irssidate/Makefile
	lastseen/Makefile
	listhandler/Makefile
	offlinemsg/Makefile
	markerline/Makefile
	mystatusbox/Makefile
	nicksaid/Makefile
	newline/Makefile
	oldlogger/Makefile
	plonkers/Makefile
	sepandtab/Makefile
	showoffline/Makefile
	simfix/Makefile
	slashexec/Makefile
	sslinfo/Makefile
	stocker/Makefile
	talkfilters/Makefile
])

dnl #######################################################################
dnl # Ouput!!
dnl #######################################################################

echo;
echo $PACKAGE $VERSION Configuration complete
echo;
echo Debugging enabled................: $enable_debug
echo;
echo Building plugins:
echo $PP_BUILDDIRS | xargs -n 4 echo "	"
echo;
echo Type make to compile
echo;
