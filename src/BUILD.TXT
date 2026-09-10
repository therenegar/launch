Launch!
=======

Build instructions

Target: DOS 3.3+, 80286, EGA or better, 80-column text mode.
Compiler: Microsoft C/C++ 7.0, small model.


FILES
-----

AUTOGEN.DAT  Autogen program ID database
AUTOGEN.C    Source for the automatic program scanner
BUILD.BAT    Build command
CLOCKDAT.H   The segmented digits used in the Clock screensaver
INSTALL.C    Source for the interactive installation utility
LAUNCH.C     Source, including the compact BIOS keyboard macro helper
LAUNCH.MNU   Example menu configuration
LOGODAT.H    The DOS logo used in the Logo screensaver
MACRO.ASM    GNU assembler source for the helper embedded in LAUNCH.C
SHORTCDB.ASM GNU assembler wrapper source for SHORTCDB.COM
SHORTCUT.ASM GNU assembler source for SHORTCUT.COM


BUILD
-----
Load the Microsoft C/C++ 7.0 environment, change to this directory, and run:

BUILD.BAT

The build uses the small model and produces !.EXE, INSTALL.EXE and AUTOGEN.EXE. The
preassembled 8086 keyboard helper is embedded in LAUNCH.C, so no separate
assembler is required. Run Launch! by typing !; DOS does not require the
extension.

SHORTCUT.COM is already assembled and does not require Microsoft Macro
Assembler. Its optional source can be rebuilt on a modern system with GNU
binutils using the commands shown at the top of SHORTCUT.ASM.

