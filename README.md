# Simulation of the VT100 hardware

### About

This is a software simulation of the VT100 hardware.  The original
firmware ROM is built in and executed by an 8080 emulator.  Other
components include video display with character generator ROM,
settings NVRAM, Intel 8251 USART, and a keyboard matrix scanner.  The
Advanced Video Option is not included.

### Usage

The command line syntax is `vt100 [-f] [-D] [-R test] program`.

- `-f` enters full screen.  Toggle with <kbd>F11</kbd>.
- `-D` enters a PDP-10 style DDT for debugging the firmware.
- `-R test` runs a CP/M program; this is only for testing.
- `program` is any command to run as a child process providing I/O.

<kbd>F9</kbd> is the SET-UP key.  See a [VT100 User
Guide](https://vt100.net/docs/vt100-ug/chapter1.html) for instructions.
