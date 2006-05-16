#*************************************************************************
#* Buddy Edit Makefile
#* by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
#* Licenced under the GNU General Public Licence version 2.
#*************************************************************************/

VERSION=0.4pre1
# yes or no
PRIVATE_TZLIB ?= yes

CFLAGS=-O2 -g -Wall `pkg-config --cflags glib-2.0 gaim` -DPLUGIN_VERSION=$(VERSION)
LDFLAGS=`pkg-config --libs glib-2.0 gaim`
CC=gcc

ifeq ($(PRIVATE_TZLIB),yes)
CFLAGS += -DPRIVATE_TZLIB
PRIVATE_TZLIB_OBJS=localtime.o
endif

LIB_TARGETS=buddyedit.so buddytimezone.so buddynotes.so buddylang.so
EXE_TARGETS=timetest recursetest gtktimezonetest

all: $(LIB_TARGETS) $(EXE_TARGETS)

recursetest: recurse.o recursetest.o

timetest: timetest.o localtime.o

gtktimezonetest.o: gtktimezonetest.c
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
	install -m 755 -d $(HOME)/.gaim/plugins
	install -m 644 buddytimezone.so $(HOME)/.gaim/plugins/
	install -m 644 buddyedit.so $(HOME)/.gaim/plugins/
	install -m 644 buddynotes.so $(HOME)/.gaim/plugins/
	install -m 644 buddylang.so $(HOME)/.gaim/plugins/

bundle:
	ln -s . buddyedit-$(VERSION)
	tar cvzf ../buddyedit-$(VERSION).tar.gz buddyedit-$(VERSION)/{*.c,*.h,Makefile,README,COPYING,Changelog}
	rm buddyedit-$(VERSION)

