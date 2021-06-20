# Simulation of the VT100 hardware

[![Build](https://github.com/larsbrinkhoff/terminal-simulator/actions/workflows/build.yml/badge.svg)](https://github.com/larsbrinkhoff/terminal-simulator/actions/workflows/build.yml)

### About

This is a software simulation of the VT100 hardware.  The original
firmware ROM is built in and executed by an 8080 emulator.  Other
components include video display with character generator ROM,
settings NVRAM, Intel 8251 USART, and a keyboard matrix scanner.  The
Advanced Video Option is not included.

To build this, you need to have the SDL2 and SDL2_image libraries
installed.

<img src="https://user-images.githubusercontent.com/775050/121336737-1a279100-c91c-11eb-87fb-bd015b20e7fa.png" width="200"> <img src="https://user-images.githubusercontent.com/775050/121338972-53610080-c91e-11eb-94aa-8c670bb2e8e3.png" width="200"> <img src="https://user-images.githubusercontent.com/775050/121336830-2dd2f780-c91c-11eb-84b6-e7dacf5324d0.png" width="200">

### Usage

The command line syntax is `vt100 [-f] [-D] [-R test] program/device`.

- `-f` enters full screen.  Toggle with <kbd>F11</kbd>.
- `-D` enters a PDP-10 style DDT for debugging the firmware.
- `-R test` runs a CP/M program; this is only for testing.
- `-C` turns capslock into control.
- `-Q` disables use of OpenGL.
- `-N` field rate.
- `program/device` is any command to run as a child process providing I/O,
  or a character device assumed to be a serial port.

<kbd>F9</kbd> is the SET-UP key.  See a [VT100 User
Guide](https://vt100.net/docs/vt100-ug/chapter1.html) for instructions.
<kbd>Control</kbd>+<kbd>F11</kbd> exits the simlator.

### 3D Printed Model

This simulator was inspired by Michael Gardi's 3D printed model, see his
[instructions](https://www.instructables.com/23-Scale-VT100-Terminal-Reproduction/)
and [GitHub files](https://github.com/kidmirage/2-3-Scale-VT100-Terminal-Reproduction).

This is my printing progress so far:
![VT100 3D print](https://retrocomputingforum.com/uploads/default/original/2X/c/c4f3cae595903887ff446df226e7f89e31eb5b14.jpeg)
