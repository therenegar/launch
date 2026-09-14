# Launch!

A lightweight command launcher for any DOS, with huge features to improve the usability of the command prompt - and make it more convenient and friendly.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**
> You can download as either a .ZIP file or a 1.44MB floppy disk image .IMG

Requires DOS 3.3, 80286, EGA or better. 

Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual machines, including DOSBox.<br/>
Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="The Launch! Menu" src="https://github.com/user-attachments/assets/a4f92c9e-6643-45c4-b822-6b1c0ddd9c59" />

## Features
- Displays a hierarchical folder based menu, modally over the top of the existing console contents.
- Supports trigger by a customizable keyboard shortcut.
- Launches commands using the existing command interpreter and shell.
- Easy visual menu editing.
- Automatic menu generator with comprehensive DOS program database to automatically identify programs.
- Program parameter help screen to make it easy supplying parameters to any menu item.
- Built in executable explorer to quickly browse and run programs anywhere.
- 7 handy and optional text-mode only accessories and a game to go with Launch!
    - Calculator, Calendar, Note, Draw, Cardfile, System Info, and Boxes game.
- Built in Power Off/Reboot control with retro Windows 95 power off experience.
- 14 awesome screensavers including a 7-segment digital clock, starry night skyline, bouncing DOS logo, warp field, 3D pipes, bouncing 3D ball, paintball, and more!
- Custom VGA display fonts to change the look of your whole DOS environment.
- Maximum compatibility across DOS versions (back to DOS 3.3) on real or emulated hardware/virtual machines.
- No libraries or dependencies including ANSI.
- Extremely minimal memory footprint. All resident components can be disabled to have zero memory impact if desired. 


## Install it

Extract the release zip file or mount the floppy image:

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- You can choose whether to install accessories
- The necessary files will be copied and you will be prompted for all changes to `AUTOEXEC.BAT`
- The keyboard shortcut can be chosen
- Your drive can be scanned and an initial menu built.
- Simply reboot after install and you're ready to go.

> DOSBox installs will be detected by the installer, and a different DOSBox compatible version of the shortcut key tool will be installed.

> **If you already have Launch! installed**, choose the same directory and an upgrade will be performed keeping your existing configuration and menu in-tact.

<img width="360" height="300" alt="Install" src="https://github.com/user-attachments/assets/0ec5eb6a-e1af-491a-a18c-08fcb0c2db5b" />


## Usage

At the **command prompt**, display your menu by pressing the keyboard shortcut which by default is set to:
```
CTRL + ALT + .
```
> The keyboard shortcut will do nothing while in a program, you must be at the command prompt for the menu to display. This is not a multi-tasking application switcher!

If you don't have `SHORTCUT.COM` loaded, the keyboard shortcut will not be available.<br/>
To start Launch! without the keyboard shortcut, at the command prompt, simply enter:
```
!
```

### How it works
Launch! does not execute the selected program itself. Rather, it returns to the existing command interpreter, types the configured command and, if selected, supplies Enter. This means shell commands, redirection, pipelines, batch files, executable files and deliberately unfinished command lines can all be used. No secondary command processor is started or additional shells. Launch! does not interfere with program execution or return.
This approach provides maximum flexibility, and compatibility. *If it can be run from the command prompt, it will work with Launch!*.

`LAUNCH.CFG` stores configuration settings and `LAUNCH.MNU` contains the menu data. Whenever a change is made to the menu, a `LAUNCH.BAK` file will also be created containing a backup of the menu.
Launch! locates and saves `LAUNCH.MNU` beside `!.EXE,` regardless of the current working directory. This works both with a full executable path and when `!.EXE` is found via `PATH`.


### Parameters
The Launch! executable `!` has some useful parameters:

- `/?` - show Launch! help
- `/CONFIG` - show the configuration dialog
- `/EXPLORE` - open the Explore & Run dialog directly without the menu
- `/NOW` - start the configured screensaver immediately
- `/USE=file.mnu` - use an alternative menu file from the default `launch.mnu`. It will be assumed to be beside `!.EXE` unless a full path is provided. This allows you to make use of different menu configurations. 
- `/OPENTO=folder` - open the Launch! menu to the specified folder, e.g. `/OPENTO="System Tools"` would show the menu with the System Tools sub-menu already open. 


## Keyboard usage

General navigation
- `Up/Down`       - Select an entry
- `Right`         - Open a selected folder
- `Left`          - Close the current folder
- `Enter`         - Open a folder or send a launcher command to the prompt
- `Esc`           - Close the complete menu

Menu management
- `Ctrl+A`        - Add a folder or launcher to the open menu
- `Ctrl+D`        - Delete the selected item after confirmation
- `Ctrl+E`        - Edit the selected folder or launcher
- `Ctrl+Up/Down`  - Move the selected item within its menu
- `Ctrl+S`        - Sort the open menu alphabetically


## Mouse usage

If a suitable mouse driver has been loaded (`MOUSE.COM`, `MOUSE.SYS`, `CTMOUSE.EXE`, etc.) you will be able to use a mouse with Launch!

<img width="70" height="70" alt="Screenshot: Cursor" src="https://github.com/user-attachments/assets/c0fb9398-7623-408e-bc30-d3d7a6beb940" />

The arrow shaped cursor will be visible when Launch! is run.<br/>

- Left click opens a folder or runs a launcher; left click outside all visible menu panels closes Launch!.
- Right click an item opens its Edit dialog. 


## Configuration

Run `! /CONFIG` to configure Launch! appearance and preferences. You can also right-click on the main "Launch!" menu title.<br/>
Options are split across 5 tabs.
Cancel will return the previous configuration.

Settings are saved to `LAUNCH.CFG`, a plain text file you can also edit yourself.<br/>
If `LAUNCH.CFG` is absent or malformed, Launch! uses the defaults. 

### Shortcut

You can view the current status of the shortcut utility, current key combination, and set a new combination.<br/>
If the shortcut is active, you can unload it - removing all traces of the TSR from memory.<br/>
If the shortcut is inactive, you can activate the shortcut - which will install the required line in your `AUTOEXEC.BAT` and reboot.<br/>
Note that changing the key combination also requires a reboot to apply.

<img width="360" height="300" alt="Config - Shortcut" src="https://github.com/user-attachments/assets/da3e7b7a-501a-4c86-b1b4-60d86b37baee" />

### Menu

You can choose where the menu is positioned, what options are displayed, the time format, and the style of cursor to use.

<img width="360" height="300" alt="Config - Menu" src="https://github.com/user-attachments/assets/c42abaf4-e6e5-49f0-ae91-9f4298c0f104" />

### Colors

There are 9 pre-defined color schemes to choose from, or you can change the colors for any interface element to your liking.

If using a monochrome display, use the 'Mono' scheme for best results.

<img width="360" height="300" alt="Config - Colors" src="https://github.com/user-attachments/assets/5cf0cbe7-6800-4bd7-ab12-3c9f109e5718" />

<img width="360" height="296" alt="Color Schemes" src="https://github.com/user-attachments/assets/e72eedf8-7e60-4f50-90b6-3936b837bcd6" />


### Screensaver

You can select which screensaver to show, choose `None` to disable this functionality.<br/>
You can also choose the time after which the screensaver will activate (1, 5, 15 or 30 minutes).<br/>
Click `Preview` for an instant preview of the currently selected screensaver.

<img width="360" height="300" alt="Config - Screensaver" src="https://github.com/user-attachments/assets/ee0f61cb-2cdd-497d-85d6-56032bf486b9" />

If the shortcut utility is loaded, the inactivity monitoring will apply to the command prompt and the menu - so your screensaver will also start if there's inactivity at the command prompt.<br/>
If the shortcut utility is not loaded, the screensaver will only start when the menu is open. 

###  Font

You can change the VGA font used across the entire DOS session. There's 22 different fonts to choose from. `Standard` uses the system VGA BIOS rom font. 

<img width="360" height="300" alt="Config - Font" src="https://github.com/user-attachments/assets/c3cae0f7-1682-4c56-9fae-3987e45a35a3" />

If you choose `Persist`, Launch! will forcefully keep your font applied, even after screen mode changes. However, that will consume 4KB of lower memory (tiny, but that could be all the difference in some circumstances). Without `Persist`, no extra memory is consumed, Launch! will re-apply your font each time the menu is shown. Unchecking `Persist` if already active at any time, will release the 4KB of memory back.

> Note this feature is not available with an EGA display adapter.


## Live menu management
Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is kept at the bottom when the menu is sorted.

`CTRL+A` allows you to add a new folder, launcher, or separator to the currently visible menu panel.

<img width="360" height="300" alt="Menu - Add" src="https://github.com/user-attachments/assets/0159647e-1926-44a7-9dfc-29dc0abb56cd" />

`CTRL+E` will show you the edit dialog to modify a launcher.

<img width="360" height="300" alt="Menu - Edit" src="https://github.com/user-attachments/assets/30af8134-0e53-4da9-8f14-35d904805786" />

`CTRL+D` will delete the selected item.

<img width="360" height="300" alt="Menu - Delete" src="https://github.com/user-attachments/assets/289bf266-d264-4d30-a68a-7c8b9a78bed6" />

Move selected items up and down with `CTRL+↑/↓`.

When **Change directory first** is selected, Launch! extracts the directory from the first command token when the launcher is run.<br/>
E.g. for `C:\TOOLS\APP.EXE` it types `C:`, presses Enter, types `CD C:\TOOLS`, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

### Parameter prompting and help
If a launcher has `Prompt?` activated, when launched from the menu, the parameter entry dialog will be displayed.<br/>
You will be able to enter parameters before running the program.

<img width="360" height="300" alt="Run with parameters" src="https://github.com/user-attachments/assets/d8a86e31-9bc2-4e93-bee3-0e6f0d825011" />

You can also view command help for assistance in entering parameters by choosing `/?`.

<img width="360" height="300" alt="Parameter Help" src="https://github.com/user-attachments/assets/175b346f-0cbc-41f2-9fb1-23a8aebddf17" />

### Manual menu configuration
`LAUNCH.MNU` is a plain text file that you can edit yourself with any text editor.<br/>
Within the `LAUNCH.MNU` file, sections represent menu paths. Separate nesting levels with a backslash:
```
  [Launcher\Internet]
  FOLDER=Communications
  ITEM=Telnet|C:\MTCP\TELNET.EXE|1|0|101

  [Launcher\Internet\Communications]
  ITEM=Pine|C:\COMM\PINE.EXE|1|1|0
```
An ITEM record has this form:
```
ITEM=title|command and parameters|provide Enter|change directory|prompt
```
The last three values are 1 for on and 0 for off. Records that do not contain them remain compatible and default to 1|0|0.

A separator is added with
```
SEPARATOR=
```

## Explore & Run
`Explore & Run` is a fixed item at the bottom of the menu.<br/>
When selected you will be able to browse the file-system for executable programs and quickly run them, exactly as launchers are run from the menu.<br/>
Enter will open the selected directory, or run the selected executable. You can also double-click entries with the mouse for the same effect.

The drive bar shows the available disk drives, and allows you to switch between them.

The path bar shows a preview of the command line that will be executed when `Run` is chosen.

<img width="360" height="300" alt="Explore & Run" src="https://github.com/user-attachments/assets/2be35c39-b4d9-4dba-9750-14716afb7ab1" />

Choosing `Params` for an executable will show the parameter entry dialog, so you can provide the desired parameters before launching the command.

<img width="360" height="300" alt="Run with parameters" src="https://github.com/user-attachments/assets/ad77c024-122d-4399-9882-6bd24b33e314" />

Choosing `/?` will show the command's help information (if available) and allow you to provide parameters.

You can remove the `Explore & Run` menu item in configuration (`! /CONFIG`)<br/>

The dialog can also be shown on its own, without the menu, by running `! /EXPLORE`.


## Shutdown
The `Shutdown...` menu item is also fixed at the bottom of the menu, and when selected shows a dialog for you to Power Off or Reboot.<br/>

Power Off flushes DOS and SMARTDrive buffers first and then uses power management APM/ACPI calls to power off. That may not work on older machines of course, in which case, the Power off message from Windows 9x will be shown.<br/>

<img width="360" height="300" alt="Shutdown..." src="https://github.com/user-attachments/assets/7fe6adf9-1b45-4a4a-bbaa-39b3486b7732" />

You can remove the `Shutdown...` menu item in configuration (`! /CONFIG`)

<img width="360" height="300" alt="Power off bitmap" src="https://github.com/user-attachments/assets/2eee9995-eed2-49a5-8f5f-6753d8423140" />

If desired, you can replace this image as `PWROFF.BMP` with any 320x400 256 color bitmap.


## Screensavers
If there has been inactivity for the configured time (1 minute by default), the screen will blank and show a screensaver (by default the Clock).

You can disable the screensaver completely (choose None) or choose from one of the other screensavers by running `! /CONFIG`.
You can also start the selected screensaver at any time by running `! /NOW`.

Pressing any key or moving the mouse will return the screen.

The screensavers have been designed to use the EGA 16 color 640x350 screen mode, using absolutely minimal resources (procedurally generated graphics, no bitmaps), run smoothly on a 286, and look great on a CRT!

- **Clock** The clock will display in 12 or 24-hour mode depending on your menu time setting, and you can change its color in ! /CONFIG
- **Boing** The famous pseudo-3D bouncing ball
- **Logo** The DOS logo bouncing around
- **Mosaic** A random mosaic tile pattern fills the screen
- **Mystic** Mystical polygons make moves
- **Paintball** Dripping paint splotches
- **Particles** Particle masses intertwine, random colors each time
- **Pipes** The 3-dimensional pipes with a mind of their own
- **Scooter** Scoot through the universe
- **Space Junk** Damn junk flying around in space
- **Space Wars** A space battle before your eyes
- **Spiro** Spiraling snakes fill the screen with pixels
- **Starry Nite** A city skyline twinkles at night
- **Warp** Zoom through the universe to nowhere

<img width="360" height="300" alt="Clock" src="https://github.com/user-attachments/assets/fa3521ee-3c3f-4db6-9be6-c7f471258980" />
<img width="360" height="300" alt="Boing" src="https://github.com/user-attachments/assets/fb46506d-d19e-48ca-b390-6e2ade1ed28d" />
<img width="360" height="300" alt="Logo" src="https://github.com/user-attachments/assets/5cf5181f-1bf4-46f4-82c9-3fae201a393c" />
<img width="360" height="300" alt="Mosaic" src="https://github.com/user-attachments/assets/00af5d79-19b1-4ba2-b4a7-7c2915e5292d" />
<img width="360" height="300" alt="Mystic" src="https://github.com/user-attachments/assets/48c0f0ac-5f6e-4bf8-921b-ce0344062a47" />
<img width="360" height="300" alt="Paintball" src="https://github.com/user-attachments/assets/dc05581c-704b-49db-8e11-b1d4c25cf7a4" />
<img width="360" height="300" alt="Particles" src="https://github.com/user-attachments/assets/b3ea5e20-e25d-433c-acda-bc1047161974" />
<img width="360" height="300" alt="Pipes" src="https://github.com/user-attachments/assets/18b5e9b6-a217-4d91-9aba-c24617648fc1" />
<img width="360" height="300" alt="Scooter" src="https://github.com/user-attachments/assets/81b2a363-84df-4e87-b3d9-d3da62fc721e" />
<img width="360" height="300" alt="SpaceJunk" src="https://github.com/user-attachments/assets/ed5fdc8b-7936-4c5b-a2b0-b2eb610d7555" />
<img width="360" height="300" alt="SpaceWars" src="https://github.com/user-attachments/assets/98b53a86-547d-4f1d-bfb9-e76c0f42ac8c" />
<img width="360" height="300" alt="Spiro" src="https://github.com/user-attachments/assets/61cc9193-ae96-4eed-a858-0241cdfad46a" />
<img width="360" height="300" alt="StarryNite" src="https://github.com/user-attachments/assets/f8db1076-934a-44ec-828d-24a633c52f6a" />
<img width="360" height="300" alt="Warp" src="https://github.com/user-attachments/assets/79a98013-a37a-4690-9eef-cbe7f30f10a5" />


## Accessories
Launch! comes with 7 handy accessories (including a game) that can be useful in a basic DOS environment.

If selected during the Install program, there will be an **Accessories** menu created for you in Launch! providing quick access.

All accessories depend on Launch! (`!.EXE`), and will take on Launch! configuration settings such as color scheme and mouse cursor.<br/>
They've all been designed with a tiny memory footprint, and maximum compatibility in mind.

Usable with either keyboard or mouse interaction.

### !BOXES - Boxes
A Sokoban style puzzle game with 100 different challenges.

Use the arrow keys to move the worker and push boxes to cover all the targets - in as few moves as possible.

<img width="360" height="300" alt="Launch! Boxes" src="https://github.com/user-attachments/assets/9e26095e-9967-4d0d-8fa2-faafe94a32dd" />

### !CAL - Calendar
A basic monthly calendar view. Use `Left` and `Right` arrow keys to navigate months, `Home` returns to the current month.

You can print the calendar out, it will create a full page calendar with big enough space to write. 

<img width="360" height="300" alt="Launch! Calendar" src="https://github.com/user-attachments/assets/3d63cff5-b046-4a91-bb23-0798ee9c3732" />

### !CALC - Calculator
A simple calculator, pretty self explanatory!

<img width="360" height="300" alt="Launch! Calculator" src="https://github.com/user-attachments/assets/1c579811-89e4-4a1f-ba5a-5f8ef1fea2dc" />

### !STACK - Card Stack
A stack of index cards. Each card has a title and text, and you can navigate between them. Clicking a card title brings that card to the front of the stack for editing.

Add/Delete cards with the respective buttons.
You can also export all the cards (everything concatenated) into a text file.

Entries are automatically saved, everything is persisted, and all cards will be visible when the program is re-opened.

<img width="360" height="300" alt="Launch! Stack" src="https://github.com/user-attachments/assets/9b2515c7-1453-4daf-870a-263d2642e1bf" />

### !NOTE - Note
Simple notepad. You can click and type anywhere.

Export the contents to a text file with `Export`. You can also send the contents to the printer with `Print`.

The text contents are persisted when you close the Note, and will be there when the program is re-opened.

Choose `Clear` to erase the notepad.

<img width="360" height="300" alt="Launch! Note" src="https://github.com/user-attachments/assets/c7b98fb6-8be4-411f-90b6-03e94444cad5" />

### !DRAW - Pixel Draw
You can create simple pixel images, they can be exported as a bitmap.<br/>
The canvas is 96x96 pixels, the viewable area is 28x12 and can be scrolled.<br/>
Exported bitmaps will be cropped to the visible contents. So if you draw a 32x32 icon, you'll get a 32x32 export.

Click a palette color and then draw in the drawing area. You can hold down the mouse button to draw, or click individual pixels. <br/>
The right mouse button erases.<br/>
The grid can be toggled on/off.

`Show` will render your drawing in large scale graphics mode fullscreen.

Your drawing is persisted and will be there when the program is re-opened.

<img width="360" height="300" alt="Launch! Pixel Draw" src="https://github.com/user-attachments/assets/13badfca-725d-4471-8bb4-be041675808f" />

Here's an actual export

<img width="280" height="100" alt="Export" src="https://github.com/user-attachments/assets/4ef4d7cb-d7fd-42f0-b549-cfb2d95db58c" />


### !SYSINFO - System Information
View useful information about your system, including free memory and disk space.

The system information report can be printed for reference.

<img width="360" height="300" alt="Launch! System Information" src="https://github.com/user-attachments/assets/cfde40c0-d6ba-4875-a5de-fab5ae6b2882" />


## SHORTCUT - keyboard shortcut tool
The keyboard shortcut is provided by a separate utility as it is not required to use `!.EXE` on its own. 

If you don't load the keyboard shortcut tool, you'll regain 500 bytes of memory - although `LOADHIGH` is used with `SHORTCUT.COM` to move this to upper memory anyway.

If chosen, it will be added to `AUTOEXEC.BAT` by install so the shortcut is available after startup.
The combination can be changed any time after install in Configuration.

You can change the keyboard shortcut used by manually adding the `/KEY=` parameter to `SHORTCUT.COM` with readable names or hexadecimal scan codes, e.g.
```
  SHORTCUT /KEY=LWIN
  SHORTCUT /KEY=CTRL+SPACE
  SHORTCUT /KEY=CTRL+ALT+L
  SHORTCUT /KEY=1D+38+34
```
Tokens are separated by `+`. CTRL, ALT, SHIFT, PERIOD, DOT, SPACE, F1-F12, the Windows key, letters, digits, common punctuation are accepted. You can also use scan codes.<br/>

The shortcut utility can be removed from memory with `SHORTCUT /UNLOAD`.<br/>
Use `SHORTCUT /?` for more information.


## AUTOGEN - an automatic menu generator
The menu generator will scan your C:\ for recognized programs in its internal database (over 1000 DOS programs up to 1995).

<img width="360" height="300" alt="AUTOGEN" src="https://github.com/user-attachments/assets/8edbc6cb-a31b-4149-9c6f-6e4d39809a0c" />


You will be prompted to resolve any ambiguous items found.
It will also detect your installed DOS version and build a DOS command menu.
This menu will replace any existing `LAUNCH.MNU` file, saving the existing menu as `LAUNCH.BAK` first.

`AUTOGEN` is run as part of the install process, but can be run at any time. 

You can use the `/LOOKIN=C,D,E` parameter to change the drives the tool will search for programs. By default, only C. One or more drives can be specified, separated by a comma.
Use `AUTOGEN /?` for more information.


## File safety
Launch! validates `LAUNCH.MNU` before opening the menu. A valid file must contain the [Launcher] root section and every non-comment line must be a valid section, FOLDER, or ITEM record. Empty, truncated, malformed, or oversized records are rejected.

Before every accepted Add, Edit, Delete, Move, or Sort operation, the existing valid `LAUNCH.MNU` is copied to `LAUNCH.BAK`. The new menu is first written fully to `LAUNCH.$$$` and is installed only after writing succeeds. `LAUNCH.BK$` is used briefly while rotating the backup.

If `LAUNCH.MNU` is missing or invalid at startup and `LAUNCH.BAK` is valid, Launch! restores the backup automatically and displays a recovery message. If neither file is usable, the built-in sample menu is installed as both `LAUNCH.MNU` and `LAUNCH.BAK`. An invalid primary file is preserved as `LAUNCH.BAD` when possible.
