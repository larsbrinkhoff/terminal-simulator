all: vt100
CFLAGS?=-O3 -DSHADERDIR=\"../common/\"
ifeq "$(shell uname)" "Darwin"
 LDFLAGS += -framework OpenGL
else
 LDFLAGS += -lGL
endif
#For gcc: CFLAGS=-O3 -flto
#For clang: CFLAGS=-Ofast
check: vt100 ../test/test.sh
	cd ..; sh test/test.sh
WARN=-Wall -Wno-unused-parameter -Wno-parentheses -Wno-unused-result
SRC=main.c cpu.c sys.c ddt.c pusart.c \
	nvr.c keyboard.c video.c rom.c sound.c render.c \
	../common/sdl.c ../common/opengl.c ../common/event.c \
	../common/logger.c ../common/pty.c
DEPS=`sdl2-config --libs --cflags` -lSDL2_image $(LDFLAGS)
vt100: $(SRC) Makefile vt100.h ../common/xsdl.h ../common/opengl.h
	$(CC) $(WARN) $(CFLAGS) -I. -I../common -o $@ $(SRC) $(DEPS)
