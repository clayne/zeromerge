CC=gcc
CFLAGS=-O2 -pipe -fstrict-aliasing
CFLAGS += -Wall -Wextra -Wwrite-strings -Wcast-align -Wstrict-aliasing -pedantic -Wstrict-overflow -Wstrict-prototypes -Wpointer-arith -Wundef
CFLAGS += -Wshadow -Wfloat-equal -Wstrict-overflow=5 -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code -Wformat=2 -Winit-self

all: zeromerge

zeromerge: zeromerge.c

install:
	install -D -o root -g root -m 0755 -s zeromerge $$DESTDIR/usr/bin/zeromerge

uninstall:
	rm -f $$DESTDIR/usr/bin/zeromerge

clean:
	rm -f zeromerge *~ *.o

test: zeromerge
	./test.sh
