Compiler: Microsoft C/C++ 7.0, small model.

FILES
-----
LAUNCH.C    Source, including the compact BIOS keyboard macro helper
LAUNCH.MNU  Example menu configuration
LAUNCH.CFG  Appearance settings created by LAUNCH /CONFIG
SHORTCUT.ASM GNU assembler source for SHORTCUT.COM
SHORTCDB.ASM GNU assembler wrapper source for SHORTCDB.COM
MACRO.ASM    GNU assembler source for the helper embedded in LAUNCH.C
INSTALL.C    Source for the interactive installation utility
BUILD.BAT   Build command

BUILD
-----
Load the Microsoft C/C++ 7.0 environment, change to this directory, and run:

  BUILD

The build uses the small model and produces !.EXE and INSTALL.EXE. The
preassembled 8086 keyboard helper is embedded in LAUNCH.C, so no separate
assembler is required. Run Launch! by typing !; DOS does not require the
extension.

SHORTCUT.COM is already assembled and does not require Microsoft Macro
Assembler. Its optional source can be rebuilt on a modern system with GNU
binutils using the commands shown at the top of SHORTCUT.ASM.