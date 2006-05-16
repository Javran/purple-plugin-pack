#*************************************************************************
#* Buddy Edit Makefile
#* by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
#* Licenced under the GNU General Public Licence version 2.
#*************************************************************************/

VERSION=0.4pre2

# Use internal timezone library rather than system one: yes or no
PRIVATE_TZLIB ?= yes

# Use Custom GTK widget for timezone selection: yes or no
CUSTOM_GTK ?= yes

# For choosing which version to compile against
# This is name of the pkgconfig file to use

GAIM_NAME ?= gaim

# Where to install the modules
INSTALL_PATH=$(HOME)/.$(GAIM_NAME)/plugins/

CFLAGS=-O2 -g -Wall `pkg-config --cflags glib-2.0 $(GAIM_NAME)` -DPLUGIN_VERSION=$(VERSION)
LDFLAGS=`pkg-config --libs glib-2.0 $(GAIM_NAME)`
CC=gcc

ifeq ($(PRIVATE_TZLIB),yes)
CFLAGS += -DPRIVATE_TZLIB
PRIVATE_TZLIB_OBJS=localtime.o
endif

ifeq ($(CUSTOM_GTK),yes)
CFLAGS += -DCUSTOM_GTK
PRIVATE_TZLIB_OBJS += gtktimezone.o
endif

LIB_TARGETS=buddyedit.so buddytimezone.so buddynotes.so buddylang.so
EXE_TARGETS=timetest recursetest gtktimezonetest

all: $(LIB_TARGETS) $(EXE_TARGETS)

recursetest: recurse.o recursetest.o

timetest: timetest.o localtime.o

gtktimezonetest.o: gtktimezonetest.c
	$(CC) -c $(CFLAGS) `pkg-config --cflags gtk+-2.0` -o $@ $<

gtktimezone.o: gtktimezone.c
	$(CC) -c $(CFLAGS) `pkg-config --cflags gtk+-2.0` -o $@ $<

gtktimezonetest: gtktimezonetest.o recurse.o
	$(CC) $(LDFLAGS) -lgtk-x11-2.0 -lgdk-x11-2.0 -o $@ $^

buddytimezone.so: buddytimezone.o $(PRIVATE_TZLIB_OBJS) recurse.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

buddyedit.so: buddyedit.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

buddynotes.so: buddynotes.o
	$(CC) -shared $(LDFLAGS) -o $@ $^

buddylang.o: buddylang.c
	$(CC) -c $(CFLAGS) `pkg-config --cflags gtkspell-2.0` -o $@ $<
buddylang.so: buddylang.o
	$(CC) -shared $(LDFLAGS) -lgtkspell -laspell -o $@ $^

clean:
	rm -f *.o $(LIB_TARGETS) $(EXE_TARGETS)

install: all
	install -m 755 -d $(INSTALL_PATH)
	install -m 644 buddytimezone.so $(INSTALL_PATH)
	install -m 644 buddyedit.so $(INSTALL_PATH)
	install -m 644 buddynotes.so $(INSTALL_PATH)
	install -m 644 buddylang.so $(INSTALL_PATH)

bundle:
	ln -s . buddyedit-$(VERSION)
	tar cvzf ../buddyedit-$(VERSION).tar.gz buddyedit-$(VERSION)/{*.c,*.h,Makefile,README,COPYING,Changelog}
	rm buddyedit-$(VERSION)

# Tests that all the different combinations compile. Probably only
# meaningful on my computer where 'gaim' = Gaim1.5 and 'gaim2' = Gaim2.0
# - Martijn
compiletest:
	for i in gaim gaim2 ; do for j in yes no ; do for k in yes no ; do \
		if [ $$i = gaim -a $$j = yes ] ; then continue ; fi ; \
		make clean ; \
		make all GAIM_NAME=$$i CUSTOM_GTK=$$j PRIVATE_TZLIB=$$k ; \
	done ; done ; done
