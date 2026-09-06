
FILES
-----
LAUNCH.C    Source, including the compact BIOS keyboard macro helper
BUILD.BAT   Build command


BUILD
-----
Load the Microsoft C/C++ 7.0 environment, change to this directory, and run:

  BUILD

The build uses the small model and produces LAUNCH.EXE. The preassembled 8086
keyboard helper is embedded in LAUNCH.C, so no separate assembler is required.
Run it by typing LAUNCH; DOS does not require the extension.