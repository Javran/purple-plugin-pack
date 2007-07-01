#!/bin/sh
#
# This is a script for converting the glade-generated interface.c file to a
# smartear-interface.c which will be friendly with gaim's plugin interface.

IN=interface.c
OUT=smartear-interface.c

cat $IN | \
sed -e '
# hookup to config_vbox instead of config
    s/GLADE_HOOKUP\([^c]*\)config/GLADE_HOOKUP\1config_vbox/
# return config_vbox instead of config
    s/return.*config;/return config_vbox;/
# hookup objects without referencing them (only in create_config())
    /create_config/,/return/ {
	s/GLADE_HOOKUP_OBJECT /GLADE_HOOKUP_OBJECT_NO_REF /
    }
# remove all references to "config"
    /\<config\>/d
# dont show config_vbox
    /gtk_widget_show.*config_vbox/d
# remove #include lines
    /^#include/d' \
> $OUT
