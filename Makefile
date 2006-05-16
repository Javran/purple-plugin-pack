#*************************************************************************
#* Buddy Edit Makefile
#* by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
#* Licenced under the GNU General Public Licence version 2.
#*************************************************************************/

VERSION=0.3
# yes or no
PRIVATE_TZLIB=yes

CFLAGS=-O2 -g -Wall `pkg-config --cflags glib-2.0 gaim` -DPLUGIN_VERSION=$(VERSION)
LDFLAGS=`pkg-config --libs glib-2.0 gaim`
CC=gcc

ifeq ($(PRIVATE_TZLIB),yes)
CFLAGS += -DPRIVATE_TZLIB
PRIVATE_TZLIB_OBJS=localtime.o
endif

all: buddyedit.so buddytimezone.so buddynotes.so buddylang.so timetest recursetest gtktimezonetest

recursetest: recurse.o recursetest.o

timetest: timetest.o localtime.o

gtktimezonetest.o: gtktimezonetest.c
	gcc -c $(CFLAGS) `pkg-config --cflags gtk+-2.0` -o $@ $<

gtktimezonetest: gtktimezonetest.o recurse.o
	gcc $(LDFLAGS) -lgtk-x11-2.0 -lgdk-x11-2.0 -o $@ $^


buddytimezone.so: buddytimezone.o $(PRIVATE_TZLIB_OBJS) recurse.o
	gcc -shared $(LDFLAGS) -o $@ $^

buddyedit.so: buddyedit.o
	gcc -shared $(LDFLAGS) -o $@ $^

buddynotes.so: buddynotes.o
	gcc -shared $(LDFLAGS) -o $@ $^

buddylang.o: buddylang.c
	gcc -c $(CFLAGS) `pkg-config --cflags gtkspell-2.0` -o $@ $<
buddylang.so: buddylang.o
	gcc -shared $(LDFLAGS) -lgtkspell -laspell -o $@ $^

clean:
	rm -f *.o *.so

install: all
	install -m 755 -d $(HOME)/.gaim/plugins
	install -m 644 buddytimezone.so $(HOME)/.gaim/plugins/
	install -m 644 buddyedit.so $(HOME)/.gaim/plugins/
	install -m 644 buddynotes.so $(HOME)/.gaim/plugins/
	install -m 644 buddylang.so $(HOME)/.gaim/plugins/

bundle:
	ln -s . buddyedit-$(VERSION)
	tar cvzf ../buddyedit-$(VERSION).tar.gz buddyedit-$(VERSION)/{*.c,*.h,Makefile,README,COPYING,Changelog}
	rm buddyedit-$(VERSION)

