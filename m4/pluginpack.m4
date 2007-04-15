dnl ###########################################################################
dnl # m4 Build Helper for the gaim plugin pack
dnl # Copyright (C) 2005-2007 Gary Kramlich <grim@reaperworld.com>
dnl #
dnl # awk foo and other sanity graciously provided by Caleb Gilmour
dnl #
dnl ###########################################################################
dnl # AM_BUILD_PLUGIN_LIST
dnl #
dnl # Searches a dir, for subdir's which have a .plugin file defining them
dnl # as plugins for the plugin pack.
dnl #
dnl ###########################################################################

AC_DEFUN([AM_BUILD_PLUGIN_LIST],
[dnl

	echo "HAVE_PURPLE: $HAVE_PURPLE"
	echo "HAVE_PIDGIN: $HAVE_PIDGIN"
	echo "HAVE_FINCH: $HAVE_FINCH"

	PP_DIST=""
	PP_AVAILABLE=""
	PP_ABUSIVE=""

	PP_PURPLE=""
	PP_PURPLE_ABUSIVE=""

	PP_PIDGIN=""
	PP_PIDGIN_ABUSIVE=""

	PP_FINCH=""
	PP_FINICH_ABUSIVE=""

	dnl #######################################################################
	dnl # Build a list of all the available plugins
	dnl #######################################################################
	for d in *; do
		if ! test -d $d; then
			continue
		fi

		if test -f $d/.purple-plugin ; then
			if test -f $d/.abusive ; then
				PP_PURPLE_ABUSIVE="$PP_PURPLE_ABUSIVE $d"
			fi

			if test -f $d/.build ; then
				PP_PURPLE="$PP_PURPLE $d"
			fi

			PP_DIST="$PP_DIST $d"
		elif test -f $d/.pidgin-plugin ; then
			if test -f $d/.abusive ; then
				PP_PIDGIN_ABUSIVE="$PP_PIDGIN_ABUSIVE $d"
			fi

			if test -f $d/.build ; then
				PP_PIDGIN="$PP_PIDGIN $d"
			fi

			PP_DIST="$PP_DIST $d"
		elif test -f $d/.finch-plugin ; then
			if test -f $d/.abusive ; then
				PP_FINCH_ABUSIVE="$PP_FINCH_ABUSIVE $d"
			fi

			if test -f $d/.build ; then
				PP_FINCH="$PP_FINCH $d"
			fi

			PP_DIST="$PP_DIST $d"
		fi
	done;

	dnl #######################################################################
	dnl # Determine the available plugins
	dnl #######################################################################
	if test x"$HAVE_PURPLE" = x"yes" ; then
		PP_AVAILABLE="$PP_AVAILABLE $PP_PURPLE"
		PP_ABUSIVE="$PP_ABUSIVE $PP_PURPLE_ABUSIVE"
	fi

	if test x"$HAVE_PIDGIN" = x"yes" ; then
		PP_AVAILABLE="$PP_AVAILABLE $PP_PIDGIN"
		PP_ABUSIVE="$PP_ABUSIVE $PP_PIDGIN_ABUSIVE"
	fi

	if test x"$HAVE_FINCH" = x"yes" ; then
		PP_AVAILABLE="$PP_AVAILABLE $PP_FINCH"
		PP_ABUSIVE="$PP_ABUSIVE $PP_FINCH_ABUSIVE"
	fi

	dnl #######################################################################
	dnl # Add our argument
	dnl #######################################################################
	AC_ARG_WITH(plugins,
				AC_HELP_STRING([--with-plugins], [what plugins to build]),
				,with_plugins=all)

	dnl #######################################################################
	dnl # Now determine which ones have been selected
	dnl #######################################################################
	if test x$with_plugins = xdefault ; then
		tmp_SUB="$PP_AVAILABLE"
	else
		exp_plugins=`echo "$with_plugins" | sed 's/,/ /g'`
		for p in "$PP_AVAILABLE $PP_ABUSIVE"; do
			for r in $exp_plugins; do
				if test x"$r" = x"$p" ; then
					tmp_SUB="$tmp_SUB $p"
				fi
			done
		done
	fi

	dnl # remove duplicates
	PP_BUILD=`echo $tmp_SUB | awk '{for (i = 1; i <= NF; i++) { print $i } }' | sort | uniq | xargs echo `

	dnl # add the abusive plugins to the dist
	PP_DIST="$PP_AVAILABLE $PP_ABUSIVE"

	dnl #######################################################################
	dnl # substitue our sub dirs
	dnl #######################################################################
	AC_SUBST(PP_BUILD)
	AC_SUBST(PP_DIST)

	echo "PP_PURPLE.........: $PP_PURPLE"
	echo "PP_PURPLE_ABUSIVE.: $PP_PURPLE_ABUSIVE"
	echo "PP_PIDGIN.........: $PP_PIDGIN"
	echo "PP_PIDGIN_ABUSIVE.: $PP_PIDGIN_ABUSIVE"
	echo "PP_FINCH..........: $PP_FINCH"
	echo "PP_FINCH_ABUSIVE..: $PP_FINCH_ABUSIVE"
])
