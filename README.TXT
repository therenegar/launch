Launch! 1.1
===========

Target: DOS 3.3+, 80286, EGA or better, 80-column text mode.
Compiler: Microsoft C/C++ 7.0, small model.

FILES
-----
LAUNCH.C    Source, including the compact BIOS keyboard macro helper
LAUNCH.MNU  Example menu configuration
LAUNCH.CFG  Appearance settings created by LAUNCH /CONFIG
SHORTCUT.COM Optional tiny prompt-aware hotkey TSR
SHORTCUT.ASM GNU assembler source for SHORTCUT.COM
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

INSTALLATION
------------
After building, run INSTALL. It prompts for an installation directory, copies
!.EXE and SHORTCUT.COM there, and adds the matching PATH and LOADHIGH lines to
the boot drive's AUTOEXEC.BAT. Existing identical lines are not duplicated.
Reboot when prompted to complete installation.

Manual installation is also supported:

Keep !.EXE and LAUNCH.MNU in the same directory. Put that directory on PATH if
you want to open the menu by typing !. Launch! locates and saves LAUNCH.MNU
beside !.EXE, regardless of the current working directory.

OPTIONAL HOTKEY
---------------
Run SHORTCUT at the DOS or 4DOS prompt to install the optional hotkey. The
default is Ctrl+Alt+period. The TSR waits for DOS's safe idle callback, verifies
that the foreground PSP is the command processor that installed it, and then
types Escape, a Launch! command, and Enter. At installation, SHORTCUT compares
its own directory with the PATH components. If that directory is on PATH it
stores the one-character command !; otherwise it automatically falls back to
the full path of its sibling !.EXE. It never starts !.EXE from a hardware
interrupt and ignores the hotkey while another DOS program owns the foreground
PSP.

Install it from AUTOEXEC.BAT to make the shortcut available after boot:

  SET PATH=C:\APPS\LAUNCH;%PATH%
  LOADHIGH C:\APPS\LAUNCH\SHORTCUT.COM

Set PATH before loading SHORTCUT so it selects the one-character ! command.

The resident allocation, including its PSP, is 1,376 bytes plus the DOS memory
control block. LOADHIGH can place this block in a suitable UMB.

SHORTCUT injects commands in bursts through the BIOS type-ahead buffer. It
fills every available slot immediately and refills freed slots as the command
processor reads them, avoiding a visible character-by-character typing delay.
Hotkeys are detected from raw keyboard make/break codes through the BIOS INT 15h
keyboard-intercept service, with the original INT 09h buffer method retained as
a fallback. This supports BIOS implementations such as DOSBox-X which do not
place Ctrl+Alt combinations into the standard keyboard buffer.

Override the key with readable names or hexadecimal IBM Set-1 make codes:

  SHORTCUT /KEY=CTRL+ALT+L
  SHORTCUT /KEY=1D+38+34

Tokens are separated by plus signs. CTRL, ALT, SHIFT, PERIOD, DOT, SPACE,
letters, digits, common punctuation, and scan codes from 01 through 7F are
accepted. Modifier scan codes are 1D (Ctrl), 38 (Alt), and 2A or 36 (Shift).
Use SHORTCUT /? for its built-in help. Install it from the command prompt that
should own the hotkey. Unload it with:

  SHORTCUT /UNLOAD

For safety, unloading succeeds only while SHORTCUT remains the newest handler
on every interrupt vector it uses. If another TSR was loaded afterward and
hooked one of those vectors, SHORTCUT refuses to unload; unload that later TSR
first, or reboot. After unloading, SHORTCUT may be reinstalled with a new key.

Version 1.1 shares SHORTCUT's 384-byte command queue with Launch!. When SHORTCUT
has been loaded high, launcher commands use that existing UMB-resident service
and Launch! leaves no additional conventional-memory block behind. Without
SHORTCUT, Launch! retains its small standalone helper for compatibility. That
fallback is allocated with DOS's last-fit strategy to avoid splitting the
largest conventional-memory region, and the previous allocation strategy is
restored immediately afterward.

Both injectors fill all available BIOS-buffer slots at once and refill the
buffer while it is consumed. Reboot once after upgrading from an earlier build
so the previous resident helper and SHORTCUT are removed before testing.

MENU FILE SAFETY
----------------
Launch! validates LAUNCH.MNU before opening the menu. A valid file must contain
the [Launcher] root section and every non-comment line must be a valid section,
FOLDER, or ITEM record. Empty, truncated, malformed, or oversized records are
rejected.

Before every accepted Add, Edit, Delete, Move, or Sort operation, the existing
valid LAUNCH.MNU is copied to LAUNCH.BAK. The new menu is first written fully to
LAUNCH.$$$ and is installed only after writing succeeds. LAUNCH.BK$ is used
briefly while rotating the backup.

If LAUNCH.MNU is missing or invalid at startup and LAUNCH.BAK is valid, Launch!
restores the backup automatically and displays a recovery message. If neither
file is usable, the built-in sample menu is installed as both LAUNCH.MNU and
LAUNCH.BAK. An invalid primary file is preserved as LAUNCH.BAD when possible.

CONFIGURATION
-------------
Sections represent menu paths. Separate nesting levels with a backslash:

  [Launcher\Internet]
  FOLDER=Communications
  ITEM=Telnet|C:\MTCP\TELNET.EXE|1|0

  [Launcher\Internet\Communications]
  ITEM=Pine|C:\COMM\PINE.EXE|1|1

An ITEM record has this form:

  ITEM=title|command and parameters|press Enter|change directory

The last two values are 1 for selected and 0 for clear. Existing records that
do not contain them remain compatible and default to 1|0.

Launch! does not execute the selected program itself. It restores the screen,
returns to the existing command interpreter, types the configured command and,
when selected, supplies Enter. Shell commands, redirection, pipelines, batch
files, executable files and deliberately unfinished command lines can therefore
all be used. No secondary command processor is started.

When Change directory first is selected, Launch! extracts the directory from
the first command token. For C:\TOOLS\APP.EXE it types C:, presses Enter, types
CD C:\TOOLS, presses Enter, and then types the complete configured command.
The final Enter setting applies to that complete command; the preliminary
drive and CD commands necessarily receive Enter. Commands without a path do
not cause a directory change.

KEYS
----
Up/Down       Select an entry
Right         Open a selected folder
Enter         Open a folder or send a launcher command to the prompt
Left          Close the current folder
Esc           Close the complete menu
Ctrl+A        Add a folder or launcher to the open menu
Ctrl+D        Delete the selected item after confirmation
Ctrl+E        Edit the selected folder or launcher
Ctrl+Up/Down  Move the selected item within its menu
Ctrl+S        Sort the open menu alphabetically

MOUSE
-----
When an INT 33h mouse driver is present, Launch! shows the pointer and places
it at coordinate 1,1 initially. Left click opens a folder or runs a launcher;
left click outside all visible menu panels closes Launch!. Right click an item
opens its Edit dialog. The pointer remains visible in dialogs: left click an
input field to position the insertion point, or left click a dialog button.

APPEARANCE
----------
Run ! /CONFIG to configure menu and dialog colours, menu position, and
whether the live clock is shown. Use Left/Right to cycle a focused value;
Tab or Up/Down moves between controls. Space advances a value or toggles the
clock. Mouse clicks are also supported. OK saves LAUNCH.CFG beside !.EXE;
Cancel leaves the previous appearance unchanged.

If LAUNCH.CFG is absent, Launch! uses the original blue colour scheme, opens
at the lower-left, and displays the clock. A malformed LAUNCH.CFG is ignored
with a warning and the defaults are used.

LIVE MENU MANAGEMENT
--------------------
Accepted changes are written immediately to LAUNCH.MNU. The Add/Edit Launcher
form provides these checkboxes:

  [X] Press Enter-symbol after launcher command
  [ ] Change directory first

Space or Enter toggles the focused checkbox; a mouse click also toggles it.
The form stores the executable and parameters as one command when it writes
the file.
Removing a folder also removes every item and subfolder beneath it.
Folder names cannot contain backslash, square brackets, or the pipe character,
because those characters delimit the text configuration hierarchy.

Each menu panel can display 20 items. Adding a 21st item automatically creates
a More folder at the bottom and moves the overflow into it. Further overflow
is handled the same way, up to the four-level menu limit. More is maintained
by Launch! and is kept at the bottom when the menu is sorted.

The top-level Launcher menu can contain both folders and launcher items.

COMMAND-LINE PARAMETERS
-----------------------
! /CONFIG opens Configure Appearance. ! /? shows the concise built-in
help. The former /POS parameter has been removed; position is now persistent.

The program saves and restores the underlying text screen. It writes directly
to monochrome or colour text video memory and does not require ANSI.SYS.
