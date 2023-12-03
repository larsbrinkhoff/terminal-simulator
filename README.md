# Simulation of the VT52 and VT100 hardware


### About

This is a fork of <a href=https://github.com/larsbrinkhoff/terminal-simulator>"Lars Brinkhoff's software simulation"</a> of the VT100 hardware, which is itself based on <a href=https://github.com/aap/vt05/>"Angelo Papenhoff's terminal emulator code"</a>

I have found that the VT100 simulator in its current state fails when I try to directly connect it to a serial port on launch, so I am deploying some patches so that you can use a switch to use the Linux "screen" command to control the serial connection instead. This renders the VT100 sim mostly aesthetic, but that suits my purposes well enough.

Currently the "s" argument sends a hard coded command, but eventually I will modify it so you can specify your serial device path and baud rate as needed.

### Usage

The command line syntax is `vt100 [-afgh2CDQ] [-c CUR] [-N DIV] [-R test] [program/device]`.

- `-a` set pixel color to amber.
- `-s` pass the command "screen /dev/serial0 9600" with a CR upon starting up the program
- `-c CUR` screen curvature (0.0 - 0.5, requires OpenGL)
- `-f` enters full screen.  Toggle with <kbd>F11</kbd>.
- `-g` set pixel color to green.
- `-h` give help message.
- `-2`magnify by 2; each additional `-2` adds 1 to multiplier.
- `-D` enters a PDP-10 style DDT for debugging the firmware.
- `-R test` runs a CP/M program; this is only for testing.
- `-C` turns capslock into control.
- `-N DIV` reduce recomputation of screen to 60/DIV Hz (may run faster).
- `-Q` disables use of OpenGL (may run faster).
- `program/device` is any command to run as a child process providing I/O,
  or a character device assumed to be a serial port.

<kbd>F9</kbd> is the SET-UP key.  See a [VT100 User
Guide](https://vt100.net/docs/vt100-ug/chapter1.html) for instructions.
<kbd>Control</kbd>+<kbd>F11</kbd> exits the simlator.

