all: vt100
CFLAGS=-O3
#For gcc: CFLAGS=-O3 -flto
#For clang: CFLAGS=-Ofast
check: vt100 ../test/test.sh
	cd ..; sh test/test.sh
WARN=-Wall -Wno-unused-parameter -Wno-parentheses -Wno-unused-result
SRC=main.c cpu.c sys.c ddt.c pusart.c \
	nvr.c keyboard.c video.c logger.c event.c rom.c sound.c render.c \
	sdl.c pty.c
DEPS=`sdl2-config --libs --cflags` -lSDL2_image
vt100: $(SRC) Makefile vt100.h xsdl.h
	$(CC) $(WARN) $(CFLAGS) -I. -o $@ $(SRC) $(DEPS)
