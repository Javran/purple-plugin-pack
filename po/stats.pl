#!/usr/bin/perl

# Copyright 2003 Nathan Walp <faceprint@faceprint.com>
#
# Parts Copyright 2004 Gary Kramlich <grim@reaperworld.com>
# Parts Copyright 2004 Stu Tomlinson <stu@nosnilmot.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301  USA
#

use Locale::Language;

$PACKAGE = "plugin_pack";

$lang{en_CA} = "English (Canadian)";
$lang{en_GB} = "English (British)";
$lang{en_AU} = "English (Australian)";
$lang{pt_BR} = "Portuguese (Brazilian)";
$lang{'sr@Latn'} = "Serbian (Latin)";
$lang{sv_SE} = "Swedish (Sweden)";
$lang{zh_CN} = "Chinese (Simplified)";
$lang{zh_TW} = "Chinese (Traditional)";

opendir(DIR, ".") || die "can't open directory: $!";
@pos = grep { /\.po$/ && -f } readdir(DIR);
foreach (@pos) { s/\.po$//; };
closedir DIR;

@pos = sort @pos;

$now = `date`;

system("./intltool-update --pot > /dev/null");

$_ = `msgfmt --statistics $PACKAGE.pot -o /dev/null 2>&1`;

die "unable to get total: $!" unless (/(\d+) untranslated messages/);

$total = $1;

print "<html>\n";
print "<head><title>Guifications i18n statistics</title></head>\n";
print "<body>\n";
print "<table cellspacing='0' cellpadding='0' border='0' bgcolor='#888888' width='100%' class='i18n'><tr><td><table cellspacing='1' cellpadding='2' border='0' width='100%'>\n";

print "<tr bgcolor='#e0e0e0'><th>language</th><th style='background: #339933;'>trans</th><th style='background: #339933;'>%</th><th style='background: #333399;'>fuzzy</th><th style='background: #333399;'>%</th><th style='background: #dd3333;'>untrans</th><th style='background: #dd3333;'>%</th><th>&nbsp;</th></tr>\n";

foreach $index (0 .. $#pos) {
	$trans = $fuzz = $untrans = 0;
	$po = $pos[$index];
	print STDERR "$po..." if($ARGV[0] eq '-v');
	system("msgmerge $po.po $PACKAGE.pot -o $po.new 2>/dev/null");
	$_ = `msgfmt --statistics $po.new -o /dev/null 2>&1`;
	chomp;
	if(/(\d+) translated message/) { $trans = $1; }
	if(/(\d+) fuzzy translation/) { $fuzz = $1; }
	if(/(\d+) untranslated message/) { $untrans = $1; }
	$transp = 100 * $trans / $total;
	$fuzzp = 100 * $fuzz / $total;
	$untransp = 100 * $untrans / $total;
	if($index % 2) {
		$class = " class='even'";
		$color = " bgcolor='#e0e0e0'";
	} else {
		$class = " class='odd'";
		$color = " bgcolor='#d0e0ff'";
	}
	$name = "";
	$name = $lang{$po};
	$name = code2language($po) unless $name ne "";
	$name = "???" unless $name ne "";
	printf "<tr$color><td$class>%s (<a href=\"%s.po\">%s.po</a>)</td><td$class>%d</td><td$class>%0.2f</td><td$class>%d</td><td$class>%0.2f</td><td$class>%d</td><td$class>%0.2f</td><td$class>",
	$name, $po, $po, $trans, $transp, $fuzz, $fuzzp, $untrans, $untransp;
	printf "<img src='bar_g.gif' height='15' width='%0.0f' alt='green'/>", $transp*2
	unless $transp*2 < 0.5;
	printf "<img src='bar_b.gif' height='15' width='%0.0f' alt='blue'/>", $fuzzp*2
	unless $fuzzp*2 < 0.5;
	printf "<img src='bar_r.gif' height='15' width='%0.0f' alt='red'/>", $untransp*2
	unless $untransp*2 < 0.5;
	print "</tr>\n";
	unlink("$po.new");
	print STDERR "done ($untrans untranslated strings).\n" if($ARGV[0] eq '-v');
}
print "</table></td></tr></table>\n";
print "Latest $PACKAGE.pot generated $now: <a href='$PACKAGE.pot'>$PACKAGE.pot</a><br />\n";
print "</body>\n";
print "</html>\n";

