dnl ###########################################################################
dnl # m4 Build Helper for the gaim plugin pack
dnl # Copyright (C) 2005 Gary Kramlich <amc_grim@users.sf.net>
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
	tmp_SUBDIRS=""
	PP_DISTDIRS=""
	PP_BUILDDIRS=""

	dnl #######################################################################
	dnl # Build a list of all the available plugins
	dnl #######################################################################
	for d in *; do
		if test -d $d; then
			if test -f $d/.abusive; then
				PP_ABUSIVE="$PP_ABUSIVE $d"
			elif test -f $d/.plugin; then
				PP_DISTDIRS="$PP_DISTDIRS $d"
			elif test -f $d/.build; then
				PP_BUILDDIRS="$PP_BUILDDIRS $d"
			fi
		fi
	done;

	dnl #######################################################################
	dnl # Add our argument
	dnl #######################################################################
	AC_ARG_WITH(plugins,
				AC_HELP_STRING([--with-plugins], [what plugins to build]),
				,with_plugins=default)

	dnl #######################################################################
	dnl # Now determine which ones have been selected
	dnl #######################################################################
	if test x$with_plugins = xall ; then
		tmp_SUBDIRS=$PP_DISTDIRS
	elif test x$with_plugins = xdefault ; then
		tmp_SUBDIRS=$PP_BUILDDIRS
	else
		exp_plugins=`echo "$with_plugins" | sed 's/,/ /g'`
		for p in $PP_DISTDIRS; do
			for r in $exp_plugins; do
				if test x$r = x$p ; then
					tmp_SUBDIRS="$tmp_SUBDIRS $p"
				fi
			done
		done
	fi

	dnl # remove duplicates
	PP_BUILDDIRS=`echo $tmp_SUBDIRS | awk '{for (i = 1; i <= NF; i++) { print $i } }' | sort | uniq | xargs echo `

	dnl # add the abusive plugins to the dist
	PP_DISTDIRS="$PP_DISTDIRS $PP_ABUSIVE"

	dnl #######################################################################
	dnl # substitue our sub dirs
	dnl #######################################################################
	AC_SUBST(PP_BUILDDIRS)
	AC_SUBST(PP_DISTDIRS)
])
