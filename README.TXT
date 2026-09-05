Launch! 0.41
===========

Target: DOS 3.3+, 80286, EGA or better, 80-column text mode.
Compiler: Microsoft C/C++ 7.0, small model.

FILES
-----
LAUNCH.C    Source
LAUNCH.MNU  Example menu configuration
BUILD.BAT   Build command

BUILD
-----
Load the Microsoft C/C++ 7.0 environment, change to this directory, and run:

  BUILD

The /AS option selects the small model and produces LAUNCH.EXE. Run it by
typing LAUNCH; DOS does not require the extension.

INSTALLATION
------------
Keep LAUNCH.EXE and LAUNCH.MNU in the same directory and put that directory on
PATH. Type LAUNCH at the DOS prompt to open the menu. Launch! locates and saves
LAUNCH.MNU beside LAUNCH.EXE, regardless of the current working directory.
This works both with a full executable path and when LAUNCH is found via PATH.

Version 0.4 removed the resident Ctrl+Alt+L hotkey because its conventional-
memory cost was excessive. Launch! now consumes no memory when it is not
running.

CONFIGURATION
-------------
Sections represent menu paths. Separate nesting levels with a backslash:

  [Launcher\Internet]
  FOLDER=Communications
  ITEM=Telnet|C:\MTCP\TELNET.EXE

  [Launcher\Internet\Communications]
  ITEM=Pine|C:\COMM\PINE.EXE

EXE and COM launchers are started directly with DOS process overlay. Launch!
therefore leaves memory completely and the original command processor regains
control when the selected program exits. This also avoids starting a secondary
4DOS and unexpectedly running 4START.BAT.

BAT and BTM launchers inherently require a command processor, so those alone
are passed to COMSPEC /C. A secondary 4DOS may run its configured 4START file
on that path. Launcher definitions should therefore name an EXE, COM, BAT, or
BTM file; shell-only internal commands and redirection are not supported as
standalone launcher commands in version 0.41.

KEYS
----
Up/Down       Select an entry
Right         Open a selected folder
Enter         Open a folder or launch an item
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

POSITION
--------
The default is the lower-left corner. Use LAUNCH /POS=TOP for the upper-left,
or LAUNCH /POS=BOTTOM explicitly for the default. LAUNCH /? shows built-in
usage and keyboard help.

LIVE MENU MANAGEMENT
--------------------
Accepted changes are written immediately to LAUNCH.MNU. The Add Launcher form
stores the executable and parameters as one command when it writes the file.
Removing a folder also removes every item and subfolder beneath it.
Folder names cannot contain backslash, square brackets, or the pipe character,
because those characters delimit the text configuration hierarchy.

Each menu panel can display 20 items. Adding a 21st item automatically creates
a More folder at the bottom and moves the overflow into it. Further overflow
is handled the same way, up to the four-level menu limit. More is maintained
by Launch! and is kept at the bottom when the menu is sorted.

The top-level Launcher menu can contain both folders and launcher items.

COLOUR SCHEME
-------------
The colour attributes are grouped as C_* constants near the top of LAUNCH.C.
Change those constants and rebuild to customise menu, dialog, selection, and
shadow colours. Each hexadecimal attribute uses the DOS convention: the high
nibble is the background colour and the low nibble is the foreground colour.

The program saves and restores the underlying text screen. It writes directly
to monochrome or colour text video memory and does not require ANSI.SYS.
