CC=gcc
CFLAGS=-O2 -pipe -fstrict-aliasing -D_FILE_OFFSET_BITS=64
CFLAGS += -Wall -Wextra -Wwrite-strings -Wcast-align -Wstrict-aliasing -pedantic -Wstrict-overflow -Wstrict-prototypes -Wpointer-arith -Wundef
CFLAGS += -Wshadow -Wfloat-equal -Wstrict-overflow=5 -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code -Wformat=2 -Winit-self

OBJS = zeromerge.o

ifeq ($(OS), Windows_NT)
	PROGRAM_NAME ?= zeromerge.exe
        UNAME_S=$(shell uname -s)
        ifeq ($(UNAME_S), MINGW32_NT-5.1)
                OBJS += winres_xp.o
        else
                OBJS += winres.o
        endif
else
	PROGRAM_NAME ?= zeromerge
endif

all: $(PROGRAM_NAME)

static: $(PROGRAM_NAME)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM_NAME) $(OBJS) -static

static_stripped: $(PROGRAM_NAME)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM_NAME) $(OBJS) -static
	strip $(PROGRAM_NAME)

$(PROGRAM_NAME): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM_NAME) $(OBJS)

stripped: $(PROGRAM_NAME)
	strip $(PROGRAM_NAME)

winres.o: winres.rc winres.manifest.xml
	./tune_winres.sh
	windres winres.rc winres.o

winres_xp.o: winres_xp.rc
	./tune_winres.sh
	windres winres_xp.rc winres_xp.o

install:
	install -D -o root -g root -m 0755 -s $(PROGRAM_NAME) $$DESTDIR/usr/bin/$(PROGRAM_NAME)

uninstall:
	rm -f $$DESTDIR/usr/bin/$(PROGRAM_NAME)

clean:
	rm -f $(PROGRAM_NAME) *~ *.o test_output.bin

test: zeromerge
	./test.sh
