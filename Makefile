#*************************************************************************
#* Buddy Edit Makefile
#* by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
#* Licenced under the GNU General Public Licence version 2.
#*************************************************************************/


CFLAGS=-O2 -g -Wall -I/usr/include/gaim `pkg-config --cflags glib-2.0`
LDFLAGS=`pkg-config --libs glib-2.0`
CC=gcc

all: buddyedit.so buddytimezone.so buddynotes.so buddylang.so timetest recursetest

recursetest: recurse.o recursetest.o

timetest: timetest.o localtime.o

buddytimezone.so: buddytimezone.o localtime.o recurse.o
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
	tar cvzf ../buddyedit.tar.gz *.c *.h Makefile README
