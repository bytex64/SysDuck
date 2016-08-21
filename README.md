# SysDuck

An attempt at a general-purpose Lua-based operating system for the [USB Rubber
Ducky](http://usbrubberducky.com/).

The USB Rubber Ducky is sold as a simple keystroke injection device, but
the AVR32 CPU on-board is capable of a lot more. This (unfinished)
project aims to unlock that potential by embedding
[Lua](http://www.lua.org/), a minimal scripting language.

## Building Lua

Download [Lua 5.3.2](http://www.lua.org/ftp/lua-5.3.2.tar.gz).  Patch it with
lua-5.3.2.patch. Make sure `avr32-gcc` is in your path (e.g. by starting the
Atmel Studio Command Prompt), then build with `make generic`.  This should
produce a `liblua.a` under `src/`.

## Building SysDuck

Open `SysDuck.atsln` in Atmel Studio. Add the build `liblua.a` as a library
reference, then build as normal.

## Flashing to the Duck

I had trouble with the Atmel flash tools so I used
[dfu-programmer](https://dfu-programmer.github.io/) from Linux. There shouldn't
be anything special required, just flash `Debug/SysDuck.hex`.

## Accessing SysDuck

When booted, SysDuck presents itself as a USB ACM device. Connect to it with
your favorite serial terminal emulator, like minicom or screen.
