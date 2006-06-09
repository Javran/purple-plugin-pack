#! /bin/sh
PACKAGE="gaim-plugin_pack"

SETUP_GETTEXT=./setup-gettext

($SETUP_GETTEXT --gettext-tool) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have gettext installed to compile $PACKAGE";
	echo;
	exit;
}

(libtoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have libtool installed to compile $PACKAGE";
	echo;
	exit;
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile $PACKAGE";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile $PACKAGE";
	echo;
	exit;
}

echo "Generating configuration files for $PACKAGE, please wait...."
echo;

# Backup po/ChangeLog because gettext likes to change it
cp -p po/ChangeLog po/ChangeLog.save

echo "Running gettextize, please ignore non-fatal messages...."
$SETUP_GETTEXT

#restore pl/ChangeLog
mv po/ChangeLog.save po/ChangeLog

echo "Running libtoolize, please ignore non-fatal messages...."
echo n | libtoolize --copy --force || exit;
echo;

aclocal -I m4 || exit;
autoheader || exit;
automake --add-missing --copy
autoconf || exit;
automake || exit;

echo "Running ./configure $@"
echo;
./configure $@
