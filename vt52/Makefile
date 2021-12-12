CFLAGS=-I. -I../common -I../vcd -O3 -DSHADERDIR=\"../common/\"

ifeq "$(shell uname)" "Darwin"
 LDFLAGS += -framework OpenGL
else
 LDFLAGS += -lGL
endif

SRC=main.c cpu.c rom.c timing.c video.c keyboard.c render.c uart.c \
	../common/pty.c ../common/event.c ../common/rs232.c \
	../common/sdl.c ../common/opengl.c ../common/logger.c \
	../vcd/vcd.c
DEPS=`sdl2-config --libs --cflags` -lSDL2_image $(LDFLAGS)
WARN=-Wall -Wno-unused-parameter -Wno-parentheses -Wno-unused-result

all: ../vcd/vcd.c vt52

vt52: $(SRC) defs.h ../common/xsdl.h ../common/opengl.h
	$(CC) $(CFLAGS) $(WARN) -o $@ $(SRC) $(DEPS)

../vcd/vcd.c:
	git submodule update --init

clean:
	rm -f vt52 *.o
