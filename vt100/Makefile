all: vt100
CFLAGS=-O3
#For gcc: CFLAGS=-O3 -flto
#For clang: CFLAGS=-Ofast
check: vt100 ../test/test.sh
	cd ..; sh test/test.sh
WARN=-Wall -Wno-unused-parameter -Wno-parentheses -Wno-unused-result
SRC=main.c cpu.c sys.c ddt.c terminal.c pusart.c brightness.c \
	nvr.c keyboard.c video.c logger.c event.c rom.c sound.c render.c
DEPS=`sdl2-config --libs --cflags` -lm -lpthread
vt100: $(SRC) Makefile terminal.h vt100.h
	$(CC) $(WARN) $(CFLAGS) -I. -o $@ $(SRC) $(DEPS)
