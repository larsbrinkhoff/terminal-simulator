# Simulation of the VT100 hardware

[![Build](https://github.com/larsbrinkhoff/terminal-simulator/actions/workflows/build.yml/badge.svg)](https://github.com/larsbrinkhoff/terminal-simulator/actions/workflows/build.yml)

### About

This is a software simulation of the VT100 hardware.  The original
firmware ROM is built in and executed by an 8080 emulator.  Other
components include video display with character generator ROM,
settings NVRAM, Intel 8251 USART, and a keyboard matrix scanner.  The
Advanced Video Option is not included.

### Usage

The command line syntax is `vt100 [-f] [-D] [-R test] program/device`.

- `-f` enters full screen.  Toggle with <kbd>F11</kbd>.
- `-D` enters a PDP-10 style DDT for debugging the firmware.
- `-R test` runs a CP/M program; this is only for testing.
- `program/device` is any command to run as a child process providing I/O,
  or a character device assumed to be a serial port.

<kbd>F9</kbd> is the SET-UP key.  See a [VT100 User
Guide](https://vt100.net/docs/vt100-ug/chapter1.html) for instructions.

### 3D Printed Model

This simulator was inspired by Michael Gardi's 3D printed model, see his
[instructions](https://www.instructables.com/23-Scale-VT100-Terminal-Reproduction/)
and [GitHub files](https://github.com/kidmirage/2-3-Scale-VT100-Terminal-Reproduction).

This is my printing progress so far:
![VT100 3D print](https://user-images.githubusercontent.com/775050/119787447-80022a80-bed1-11eb-859a-4e9bcedda253.jpg)
