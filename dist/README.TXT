===============================================================================
                                   LAUNCH!                                     
===============================================================================

A lightweight launcher for any DOS, with huge features to improve the 
usability of the command prompt - and make it more convenient and friendly.

Download the latest release here: https://github.com/therenegar/launch/releases
* Available as either a .ZIP file or a 1.44MB floppy disk image .IMG

System Requirements:
- DOS 3.3 or higher
- 80286 processor
- EGA graphics or better

Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual 
machines, including DOSBox. Compatible with third-party command interpreters 
such as 4DOS/NDOS.


FEATURES
-------------------------------------------------------------------------------
- Displays a hierarchical folder based menu over existing console contents.
- Supports trigger by a customizable keyboard shortcut.
- Launches commands using the existing command interpreter and shell.
- Easy visual menu editing.
- Automatic menu generator with comprehensive DOS program database.
- Program parameter help screen to make it easy supplying parameters.
- Built-in executable explorer to quickly browse and run programs anywhere.
- 8 handy and optional text-mode only accessories and a game!
  * Calculator, Calendar, Note, Pixel Draw, Card Stack, System Info, To-Dos,
    and a game Boxes.
- Built-in Power Off/Reboot control with retro Windows 95 style experience.
- 14 awesome screensavers including a 7-segment digital clock, starry night,
  bouncing DOS logo, warp field, 3D pipes, paintball, and more!
- Custom VGA display fonts to change the look of your whole DOS environment.
- Maximum compatibility across DOS versions on real or emulated hardware.
- No libraries or dependencies including ANSI.
- Extremely minimal memory footprint (resident components can be disabled).


INSTALLATION
-------------------------------------------------------------------------------
Extract the release zip file or mount the floppy image:

1. Run INSTALL.EXE
2. You'll be prompted for a directory to place Launch!
3. You can choose whether to install accessories.
4. Files will be copied and you will be prompted for changes to AUTOEXEC.BAT
5. The keyboard shortcut can be chosen.
6. Your drive can be scanned and an initial menu built.
7. Simply reboot after install and you're ready to go.

* Note: DOSBox installs will be automatically detected by the installer, 
  and a different DOSBox compatible shortcut key tool will be installed.
* Upgrading: If you already have Launch! installed, choose the same 
  directory and an upgrade will preserve your configuration and menu.


USAGE
-------------------------------------------------------------------------------
At the command prompt, display your menu by pressing the default shortcut:
  [ CTRL ] + [ ALT ] + [ . ]

* Note: The keyboard shortcut will do nothing while inside a program. You 
  must be at the command prompt. This is not a multi-tasking switcher!

If you do not have SHORTCUT.COM loaded, start Launch! by entering:
  !

How it works:
Launch! does not execute the selected program itself. Rather, it returns to 
the existing command interpreter, types the configured command and presses 
Enter. This means shell commands, redirection, pipelines, batch files, and 
unfinished command lines can all be used. No secondary command processor or 
additional shell is started.

File Structure:
- LAUNCH.CFG : Stores configuration settings.
- LAUNCH.MNU : Contains the menu data.
- LAUNCH.BAK : Automatically created backup of your menu data.
Launch! always saves files right beside !.EXE, regardless of current directory.


PARAMETERS
-------------------------------------------------------------------------------
The Launch! executable (!) accepts the following parameters:

  /?                  Show Launch! help text
  /CONFIG             Show the configuration dialog
  /EXPLORE            Open the Explore & Run dialog directly without the menu
  /NOW                Start the configured screensaver immediately
  /USE=file.mnu       Use an alternative menu file instead of launch.mnu
  /OPENTO="folder"    Open the menu directly into a specified sub-folder


KEYBOARD CONTROLS
-------------------------------------------------------------------------------
General Navigation:
  Up / Down         - Select an entry
  Right             - Open a selected folder
  Left              - Close the current folder
  Enter             - Open a folder or send a launcher command to the prompt
  Esc               - Close the complete menu

Menu Management:
  Ctrl + A          - Add a folder or launcher to the open menu
  Ctrl + D          - Delete the selected item after confirmation
  Ctrl + E          - Edit the selected folder or launcher
  Ctrl + Up/Down    - Move the selected item within its menu
  Ctrl + S          - Sort the open menu alphabetically


MOUSE USAGE
-------------------------------------------------------------------------------
If a suitable mouse driver has been loaded (MOUSE.COM, MOUSE.SYS, CTMOUSE.EXE) 
an arrow-shaped cursor will be visible when Launch! is run.

- Left click opens a folder or runs a launcher.
- Left click outside all visible menu panels closes Launch!.
- Right click on an item opens its Edit dialog.


CONFIGURATION
-------------------------------------------------------------------------------
Run "! /CONFIG" to configure appearance and preferences. You can also right-
click on the main "Launch!" menu title. Options are split across 5 tabs:

1. Shortcut:
   View utility status, set a new key combination, or unload the TSR utility 
   from memory. Changing the key combination requires a reboot to apply.

2. Menu:
   Choose where the menu is positioned, what options are displayed, time 
   format, and the style of the text cursor.

3. Colors:
   9 pre-defined color schemes are available, or customize individual items.
   If using a monochrome display, use the 'Mono' scheme for best results.

4. Screensaver:
   Select a screensaver or choose 'None' to disable. Set inactivity timers 
   to 1, 5, 15, or 30 minutes. If the shortcut utility is loaded, inactivity 
   monitoring applies to the standard command prompt as well.

5. Font:
   Change the VGA font across the entire DOS session (22 fonts available). 
   Choosing 'Persist' keeps the font active after screen mode changes but 
   consumes 4KB of lower memory. Without 'Persist', zero extra memory is used.
   * Note: Font selection is not available on original EGA displays.


LIVE MENU MANAGEMENT
-------------------------------------------------------------------------------
Use keyboard shortcuts to visually edit the menu while it is open. 
Each menu panel can display 20 items. Adding a 21st item automatically 
creates a "More" folder at the bottom and moves the overflow into it.

When "Change directory first" is selected in the Edit dialog (Ctrl+E), 
Launch! will automatically change paths. For example, "C:\TOOLS\APP.EXE" will 
automatically type "C:", "CD C:\TOOLS", and then fire the executable.

Parameter Prompting and Help:
If a launcher has "Prompt?" activated, a dialog box will appear allowing 
you to input command parameters or view command assistance with "/?" before 
the software executes.

Manual Menu Configuration:
LAUNCH.MNU is a plain text file. Nesting levels use backslashes:

  [Launcher\Internet]
  FOLDER=Communications
  ITEM=Telnet|C:\MTCP\TELNET.EXE|1|0|101

  [Launcher\Internet\Communications]
  ITEM=Pine|C:\COMM\PINE.EXE|1|1|0

ITEM format: title | command | provide Enter | change directory | prompt
Values use 1 for ON and 0 for OFF. Separators are added via SEPARATOR=


EXPLORE & RUN
-------------------------------------------------------------------------------
Located at the bottom of the menu (or via "! /EXPLORE"). Allows you to browse 
the disk file-system for executable programs and launch them instantly. 

- Drive bar shows available disks and lets you switch between them.
- Path bar shows a preview of the command line that will execute.
- Double-clicking or hitting Enter opens directories or runs files.
- Can be disabled inside the "! /CONFIG" screen.


SHUTDOWN
-------------------------------------------------------------------------------
Located at the bottom of the menu. Flushes DOS and SMARTDrive buffers, 
then uses APM/ACPI calls to shut down. If the hardware lacks modern power 
features, a retro Windows 9x style "It is now safe to turn off your computer" 
graphic message is displayed.

You can replace the power-off screen by placing a 320x400 256-color bitmap 
named PWROFF.BMP into the application folder.


SCREENSAVERS
-------------------------------------------------------------------------------
Screensavers use EGA 16-color 640x350 graphics mode. They are procedurally 
generated to ensure high performance on 286 computers. Fire them manually 
via "! /NOW". Press any key or move the mouse to return to DOS.

Available Screensavers:
- Clock        : 12/24 hour digital 7-segment clock display
- Boing        : The famous pseudo-3D bouncing checkered ball
- Logo         : Bouncing classic DOS logo
- Mosaic       : Randomly filling mosaic tile patterns
- Mystic       : Moving mystical geometric polygons
- Paintball    : Dripping paint splotches
- Particles    : Intertwining particle masses with random colors
- Pipes        : 3D pipes winding across the screen
- Scooter      : Fast scrolling through the universe
- Space Junk   : Avoid orbital space debris flying past
- Space Wars   : A simulated live space battle
- Spiro        : Spiraling lines filling up pixels
- Starry Nite  : Twinkling city skyline at night
- Warp         : Classic starfield warp drive simulation


ACCESSORIES
-------------------------------------------------------------------------------
Launch! contains 6 low-footprint desktop tools accessible via the 
"Accessories" menu folder. They inherit your active color and mouse settings.

!CAL (Calendar)
Monthly calendar view. Navigate months with Left/Right arrows. Home key 
returns to the current month. Features a full-page printing option.

!CALC (Calculator)
Simple standard text-mode mathematical calculator.

!STACK (Card Stack)
A stack of virtual index cards. Each contains a title and notes. Click a 
card title to bring it forward. Automatically saves and exports to text.

!NOTE (Note)
Simple notepad. Click and type anywhere on screen. Features clear, export 
to text file, and direct printer routing capabilities.

!DRAW (Pixel Draw)
96x96 pixel canvas with a scrollable 28x12 viewport. Left click draws, 
right click erases. Toggleable alignment grid. Drawings can be exported 
directly to a standard BMP file cropped to your drawing dimensions.

!BOXES (Boxes)
A simple Sokoban style puzzle game with over 100 different challenges.
Use the arrow keys to move the worker and push boxes over the targets.

!TODOS (To-Dos)
A to-do list with tasks organised into groups. Tasks can have a due date,
extended description, and tags.

!SYSINFO (System Information)
Displays comprehensive statistics on free hardware memory, disk spaces, 
and DOS operating versions. Reports can be printed.


SHORTCUT UTILITY DETAILS
-------------------------------------------------------------------------------
The background keyboard monitoring is handled via SHORTCUT.COM. Unloading 
it frees up 500 bytes of conventional memory (though LOADHIGH is supported).

To customize keys manually, pass the /KEY= parameter in AUTOEXEC.BAT:
  SHORTCUT /KEY=CTRL+SPACE
  SHORTCUT /KEY=CTRL+ALT+L
  SHORTCUT /KEY=1D+38+34  (Uses direct hex scan codes)

- Remove tool from memory: SHORTCUT /UNLOAD
- View syntax assistance:  SHORTCUT /?


AUTOMATIC GENERATOR (AUTOGEN)
-------------------------------------------------------------------------------
Run AUTOGEN.EXE to scan your disk partitions for recognized entries 
matching a built-in database of over 1,000 retro programs up to 1995. It also 
generates localized menus for your specific DOS version utilities. 

- Overwrites LAUNCH.MNU (renaming the prior layout to LAUNCH.BAK first).
- Parameter filtering: Use "AUTOGEN /LOOKIN=C,D" to restrict targeted search 
  drives.


DATA INTEGRITY AND SAFETY
-------------------------------------------------------------------------------
Launch! validates LAUNCH.MNU structural safety prior to opening the GUI. 
Before any Add, Edit, Delete, Move, or Sort operation, the menu copies the 
existing data to LAUNCH.BAK. The updated configuration writes fully to 
LAUNCH.$$$ first, transferring to live operations only upon total success.

If LAUNCH.MNU corrupts or goes missing, Launch! restores the backup 
automatically. If both are unreadable, a basic sample menu populates. 
Corrupted source configurations are preserved safely under LAUNCH.BAD.
===============================================================================
