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
	PP_PURPLE=""
	PP_PURPLE_ABUSIVE=""
	PP_PURPLE_BUILD=""

	PP_PIDGIN=""
	PP_PIDGIN_ABUSIVE=""
	PP_PIDGIN_BUILD=""

	PP_FINCH=""
	PP_FINCH_ABUSIVE=""
	PP_FINCH_BUILD=""

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
				PP_PURPLE_BUILD="$PP_PURPLE_BUILD $d"
			fi

			PP_PURPLE="$PP_PURPLE $d"
		elif test -f $d/.pidgin-plugin ; then
			if test -f $d/.abusive ; then
				PP_PIDGIN_ABUSIVE="$PP_PIDGIN_ABUSIVE $d"
			fi

			if test -f $d/.build ; then
				PP_PIDGIN_BUILD="$PP_PIDGIN_BUILD $d"
			fi

			PP_PIDGIN="$PP_PIDGIN $d"
		elif test -f $d/.finch-plugin ; then
			if test -f $d/.abusive ; then
				PP_FINCH_ABUSIVE="$PP_FINCH_ABUSIVE $d"
			fi

			if test -f $d/.build ; then
				PP_FINCH_BUILD="$PP_FINCH_BUILD $d"
			fi

			PP_FINCH="$PP_FINCH $d"
		fi
	done;

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
	AC_SUBST(PP_PURPLE)
	AC_SUBST(PP_PURPLE_ABUSIVE)
	AC_SUBST(PP_PURPLE_BUILD)

	AC_SUBST(PP_PIDGIN)
	AC_SUBST(PP_PIDGIN_ABUSIVE)
	AC_SUBST(PP_PIDGIN_BUILD)

	AC_SUBST(PP_FINCH)
	AC_SUBST(PP_FINCH_ABUSIVE)
	AC_SUBST(PP_FINCH_BUILD)
])
