#! /bin/sh
PACKAGE="pidgin-plugin_pack"

(intltoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have intltool installed to compile $PACKAGE";
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

echo "Running libtoolize, please ignore non-fatal messages...."
echo n | libtoolize --copy --force || exit;
echo;

libtoolize -c -f --automake
intltoolize --force --copy
aclocal -I m4 || exit;
autoheader || exit;
automake --add-missing --copy
autoconf || exit;
automake || exit;

echo "Running ./configure $@"
echo;
./configure $@
