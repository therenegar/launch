<img width="1633" height="250" alt="Launch! for DOS" src="https://github.com/user-attachments/assets/1aa6eb8a-98f3-4e74-a1ef-479e4f7a04ef" />

A lightweight command launcher for any DOS, with huge features to improve the usability of the command prompt.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**
> You can download as either a .ZIP file or a 1.44MB floppy disk image .IMG

Requires DOS 3.3, 80286, EGA or better. 

Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual machines, including DOSBox.Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="The Launch! Menu" src="https://github.com/user-attachments/assets/ccc981b2-439f-4722-b7c8-5eca4bc274e3" />

VGA resolution

<img width="640" height="350" alt="The Launch! Menu - EGA Resolution" src="https://github.com/user-attachments/assets/7208281b-b205-4c7d-af9c-9cf8ff8a047a" />

EGA resolution

## Features
- Displays a hierarchical folder based menu, modally over the top of the existing console contents.
- Supports trigger by a customizable keyboard shortcut.
- Launches commands using the existing command interpreter and shell.
- Easy visual menu editing.
- Automatic menu generator with comprehensive DOS program database to automatically identify programs.
- Built in executable explorer to quickly browse and run programs anywhere.
- Built in file opener to create associations between files and launchers for easy open.
- Built in Power Off/Reboot control with retro Windows 95 power off experience.
- 9 handy and optional text-mode only accessories and 7 games to go with Launch!
    - Calculator, Calendar, Journal, Note, Pixel Draw, Card Stack, To-Dos, System Info, Typo
    - Boxes, Pop, Snake, Solitaire, FreeCell, Plumb and Wordz
- 14 awesome screensavers including a 7-segment digital clock, starry night skyline, bouncing DOS logo, warp field, 3D pipes, bouncing 3D ball, paintball, and more!
- Various Command Prompt styles to choose from to uplift your C:\
- Custom VGA display fonts to change the look of your whole DOS environment.
- Maximum compatibility across DOS versions (back to DOS 3.3) on real or emulated hardware/virtual machines.
- No libraries or dependencies including ANSI. Custom UI toolkit written in C. Fast and simple.
- Extremely minimal memory footprint. All resident components can be disabled to have zero memory impact if desired. 
- Design goal was to push the absolute limits of what a text-mode only DOS program can achieve in terms of user interface design and usability.

----
<img width="1633" height="250" alt="Install" src="https://github.com/user-attachments/assets/b7dee4b9-154f-47b6-b407-8b24ef9b03fd" />

Extract the release zip file or mount the floppy image:

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- You can choose whether to install accessories
- The necessary files will be copied and you will be prompted for all changes to `AUTOEXEC.BAT`
    - FreeDOS will be detected automatically and `FDAUTO.BAT` used instead of `AUTOEXEC.BAT` throughout Launch!
- The keyboard shortcut can be chosen
- Your drive can be scanned and an initial menu built.
- Simply reboot after install and you're ready to go.

> DOSBox installs will be detected by the installer, and a different DOSBox compatible version of the shortcut key tool will be installed.

> **If you already have Launch! installed**, choose the same directory and an upgrade will be performed keeping your existing configuration and menu in-tact.

----
<img width="1633" height="250" alt="The Menu" src="https://github.com/user-attachments/assets/833bd071-f0c6-436f-8cfa-fd933ac6685f" />

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
The Launch! executable `!.EXE` has some useful parameters:

- `/?` - show Launch! help
- `/CONFIG` - show the configuration dialog
- `/EXPLORE` - show the Explore & Run dialog directly without the menu
- `/OPEN` - show the Open File dialog directly without the menu
- `/BYE` - show the Shutdown... dialog directly without the menu
- `/NOW` - start the configured screensaver immediately
- `/USE=file.mnu` - use an alternative menu file from the default `launch.mnu`. It will be assumed to be beside `!.EXE` unless a full path is provided. This allows you to make use of different menu configurations. 
- `/OPENTO=folder` - open the Launch! menu to the specified folder, e.g. `/OPENTO="System Tools"` would show the menu with the System Tools sub-menu already open. 


### Keyboard usage

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


### Mouse usage
Have you ever seen a mouse cursor at the DOS prompt? Now you can!<br/>
If a suitable mouse driver has been loaded (`MOUSE.COM`, `MOUSE.SYS`, `CTMOUSE.EXE`, etc.) you will be able to use a mouse with Launch!

<img width="70" height="70" alt="Cursor - Pointer" src="https://github.com/user-attachments/assets/c0fb9398-7623-408e-bc30-d3d7a6beb940" />
<img width="70" height="70" alt="Cursor - Block" src="https://github.com/user-attachments/assets/d622aaf7-3642-4a00-b58b-678baaa6f1ae" />
<img width="70" height="70" alt="Cursor - Arrow" src="https://github.com/user-attachments/assets/fb4a7df3-fe9a-4131-8bf7-6441e65d6ee3" />

The cursor (pointer style by default) will be visible when Launch! is run.<br/>

- Left click opens a folder or runs a launcher; left click outside all visible menu panels closes Launch!.
- Right click an item opens its Edit dialog.
- Double clicking opens files and directories in Explore & Run.


## Open File
`Open File` is a fixed special item at the bottom of the menu (removable in Config).
It allows you to browse for files, based on configured associations, and open the file with the configured launcher.

As an example, this can allow you to browse and open images with a launcher for an image viewing program - without having to typing a massive command line yourself. Or browse zip files, and easily extract with your ZIP program, adding further parameters if required.

There's a lot of flexibility to enable you to set up file-based workflows in tandem with your Launch! menu.

<img width="720" height="600" alt="Open File" src="https://github.com/user-attachments/assets/9d15bb77-744a-46f2-84be-efed62b84413" />

An association consists of one or more file extensions, and a selected launcher from your menu.

<img width="720" height="600" alt="Create Association" src="https://github.com/user-attachments/assets/1914df90-1acf-4420-bf77-33c526864b98" />

You can also open files with further parameters if required.

<img width="720" height="600" alt="Open file with parameters" src="https://github.com/user-attachments/assets/ae87fce6-1ce3-4c2d-8e77-d2f4c6b8d090" />

The `Open File` dialog can also be shown on its own, without the menu, by running `! /OPEN`.

## Explore & Run
`Explore & Run` is a fixed item at the bottom of the menu.<br/>
When selected you will be able to browse the file-system for executable programs and quickly run them, exactly as launchers are run from the menu.<br/>
Enter will open the selected directory, or run the selected executable. You can also double-click entries with the mouse for the same effect.

The drive bar shows the available disk drives, and allows you to switch between them.

The path bar shows a preview of the command line that will be executed when `Run` is chosen.

<img width="720" height="600" alt="Explore and Run" src="https://github.com/user-attachments/assets/c7822dad-0392-4cb1-b70d-8dfbb6f6179c" />

Choosing `Params` for an executable will show the parameter entry dialog, so you can provide the desired parameters before launching the command.

<img width="720" height="600" alt="Run with parameters" src="https://github.com/user-attachments/assets/f230b18c-757c-43e7-b90b-8d605fff35a6" />

You can remove the `Explore & Run` menu item in configuration (`! /CONFIG`)<br/>

The dialog can also be shown on its own, without the menu, by running `! /EXPLORE`.


## Shutdown
The `Shutdown...` menu item is also fixed at the bottom of the menu, and when selected shows a dialog for you to Power Off or Reboot.<br/>

Power Off flushes DOS and SMARTDrive buffers first and then uses power management APM/ACPI calls to power off. That may not work on older machines of course, in which case, the Power off message from Windows 9x will be shown.<br/>

<img width="720" height="600" alt="Shutdown..." src="https://github.com/user-attachments/assets/05b1c734-8ada-4b28-a1aa-9113cf5bc435" />

You can remove the `Shutdown...` menu item in configuration (`! /CONFIG`). <BR/>
The dialog can also be shown on its own, without the menu, by running `! /BYE`.

<img width="360" height="300" alt="Power off bitmap" src="https://github.com/user-attachments/assets/2eee9995-eed2-49a5-8f5f-6753d8423140" />

If desired, you can replace this image as `PWROFF.BMP` with any 320x400 256 color bitmap.


----
<img width="1633" height="250" alt="Config: Menu" src="https://github.com/user-attachments/assets/281d5d70-bcd8-4730-baf8-df572e7006bf" />

Run `! /CONFIG` to configure Launch! appearance and preferences. You can also right-click on the main "Launch!" menu title.<br/>
Options are split across 6 tabs.
Cancel will return the previous configuration.

Settings are saved to `LAUNCH.CFG`, a plain text file you can also edit yourself.<br/>
If `LAUNCH.CFG` is absent or malformed, Launch! uses the defaults. 

### Menu

You can choose where the menu is positioned, what options are displayed, the time format, and the style of cursor to use.

<img width="720" height="600" alt="Configuration - Menu" src="https://github.com/user-attachments/assets/470f9598-7d75-40b9-b838-20f4a23fc91c" />

Contextual help (tooltips) can be turned off - they will appear throughout Launch! with keyboard focus or mouse-over.

The SysBar (off by default) shows an information bar, top-right of screen every time the menu is opened with some handy system status information.

<img width="720" height="600" alt="SysBar" src="https://github.com/user-attachments/assets/c5d096b3-0b7c-40aa-9136-73618ac4c5e5" />

The SysBar shows (left to right): 
- Largest executable program size (free lower memory)
    - Click to execute `MEM /C`
- The free Environment size (ever get an error 'Not enough space for environment' when running a program?)
- The free space on the system drive (if you boot from floppy it will be A:)
    - Click to execute `FREE C:`
- Status indicators for Caps Lock (CL), Num Lock (NL) and Scroll Lock (SL) keys
    - Click to toggle each keyboard indicator (i.e. turn Caps Lock on/off) 
- Status indicators for mouse presence, display type and network status (based on detection of a loaded network packet driver).
- The country code for the current DOS locale settings (as/if set by `COUNTRY.SYS`).
    - Click to change the country code for the current DOS session

### Colors

There are 9 pre-defined color schemes to choose from, or you can change the colors for any interface element to your liking.

If using a monochrome display, use the 'Mono' scheme for best results.

<img width="720" height="600" alt="Config: Colors" src="https://github.com/user-attachments/assets/2b3e7494-8486-418c-a691-247c5618093c" />

<img width="720" height="591" alt="Color Schemes" src="https://github.com/user-attachments/assets/189807a7-5fd1-4b88-98e5-b120dc750bcc" />


### Screen Saver

You can select which screensaver to show, choose `None` to disable this functionality.<br/>
You can also choose the time after which the screensaver will activate (1, 5, 15 or 30 minutes).<br/>
Click `Preview` for an instant preview of the currently selected screensaver.

<img width="720" height="600" alt="Config: Screen Savers" src="https://github.com/user-attachments/assets/5167f0e1-7752-4051-8569-e73e1dcecceb" />

If the shortcut utility is loaded, the inactivity monitoring will apply to the command prompt and the menu - so your screensaver will also start if there's inactivity at the command prompt.<br/>
If the shortcut utility is not loaded, the screensaver will only start when the menu is open. 


### Prompt

You can easily change the appearance of the DOS command prompt, choosing from 10 different prompt styles.

You can also add your own prompt definitions in `PROMPTS.CFG` and they will appear in the selector here.

Choosing `SET` applies the selected prompt style immediately, and also updates any existing `PROMPT` statement in `AUTOEXEC.BAT/FDAUTO.BAT` so this style is set at startup.

Styles using ANSI escape sequences will not show in the selector if an ANSI driver is not detected.

<img width="720" height="600" alt="Config: Prompt" src="https://github.com/user-attachments/assets/683fbaae-c2f8-4fb7-9c20-aeb239d7a2cb" />


###  Font

You can change the VGA font used across the entire DOS session. There's 22 different fonts to choose from. `Standard` uses the system VGA BIOS rom font. 

<img width="720" height="600" alt="Config: Font" src="https://github.com/user-attachments/assets/4f438e38-00f3-43c9-b826-840dda556332" />

If you choose `Persist`, Launch! will forcefully keep your font applied, even after screen mode changes. However, that will consume 4KB of lower memory (tiny, but that could be all the difference in some circumstances). Without `Persist`, no extra memory is consumed, Launch! will re-apply your font each time the menu is shown. Unchecking `Persist` if already active at any time, will release the 4KB of memory back.

> Note this feature is not available with an EGA display adapter.

**Font Sources**
- ISO is extracted from IBM PC-DOS 5.02 ISO.CPI
- DOS-J is extracted from IBM PC-DOS for DOS/V
- DOS-V is extracted from Microsoft MS-DOS/V
- ELITE.F16, OAK8.F16, OAK9.F16 and SANSERIF.F16 were extracted directly from the Video BIOS ROM image for the IBM PS/2 model 30-286 Rev 0.
- HOWARD.F16, OAKLEY.F16, OAKLYB.F16,. NEIL.F16, ITALIC.F16, OLDENG.F16 and CGA.F16 come from IBM's internally distributed HOWARD the FONT 3.61 archive by Alan E. Beelitz and contributors. 
- Remaining come from https://github.com/viler-int10h/vga-text-mode-fonts

All font files have been edited to improve specific glyphs and appearance over originals.


### Shortcut

You can view the current status of the shortcut utility, current key combination, and set a new combination.<br/>
If the shortcut is active, you can unload it - removing all traces of the TSR from memory.<br/>
If the shortcut is inactive, you can activate the shortcut - which will install the required line in your `AUTOEXEC.BAT/FDAUTO.BAT` and reboot.<br/>
Note that changing the key combination also requires a reboot to apply.

<img width="720" height="600" alt="Config: Shortcut" src="https://github.com/user-attachments/assets/17a6ad2b-b110-4ec2-b130-697503a98142" />


### Reset

If you mess up your Launch! installation you can reset the configuration, the menu, or both by choosing Reset.

This will return things to the defaults, and rebuild a default menu as well.

<img width="720" height="600" alt="Config: Reset" src="https://github.com/user-attachments/assets/b935db57-be19-45e3-a181-c8f71cf4b357" />


## Live menu management
Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is kept at the bottom when the menu is sorted.

`CTRL+A` allows you to add a new folder, launcher, or separator to the currently visible menu panel.

<img width="720" height="600" alt="Menu - Add" src="https://github.com/user-attachments/assets/cddb761b-d607-49f2-8b9f-086d0019315c" />

`CTRL+D` will remove the selected item from the menu (with confirmation first).

<img width="720" height="600" alt="Menu - Remove" src="https://github.com/user-attachments/assets/533cfb89-fe2e-4171-9af8-8f9c6d8d9ac6" />

You can move selected items up and down with `CTRL+↑/↓`.

`CTRL+E` will show you the edit dialog to modify a launcher.

<img width="720" height="600" alt="Menu - Edit" src="https://github.com/user-attachments/assets/10d524cf-9024-4eac-a3c8-d38a0a585255" />

If a launcher has `Prompt?` activated, when launched from the menu, the parameter entry dialog will be displayed.<br/>
You will be able to enter parameters before running the program.

**Provide enter after launcher command** if toggled off will just type the command, not supplying Enter - allowing you to then add more to the command line (such as further parameters) or review it, before execution.

When **Change directory first** is selected, Launch! extracts the directory from the first command token when the launcher is run.<br/>
E.g. for `C:\TOOLS\APP.EXE` it types `C:`, presses Enter, types `CD C:\TOOLS`, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

If **Add to PATH for execution** is selected, the command's directory will be added to the environment %PATH% variable. Many programs require this, and will update AUTOEXEC.BAT/FDAUTO.BAT - which can get unwieldy. This allows you do away with that, and just add a program to PATH when launched from the menu.

<img width="720" height="600" alt="Run with parameters" src="https://github.com/user-attachments/assets/626513d1-7b62-47d8-a989-d65c3ac0817e" />

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

----
<img width="1633" height="250" alt="Screen Savers" src="https://github.com/user-attachments/assets/60492c02-09ae-4846-8761-b318f9ddb504" />

If there has been inactivity for the configured time (1 minute by default), the screen will blank and show a screensaver (by default the Clock).

You can disable the screensaver completely (choose None) or choose from one of the other screensavers by running `! /CONFIG`.
You can also start the selected screensaver at any time by running `! /NOW`.

Pressing any key or moving the mouse will return the screen.

The screensavers have been designed to use the EGA 16 color 640x350 screen mode, using absolutely minimal resources (procedurally generated graphics, no bitmaps), run smoothly on a 286, and look great on a CRT! 

They are all re-engineered and coded from scratch, but based loosely on well known screensavers from AfterDark, Microsoft Windows, Amiga and XScreenSaver.
The clock is based on a 1979 Vacuum Fluorescent Display alarm clock on my bedside.

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

----

<img width="1633" height="250" alt="Accessories" src="https://github.com/user-attachments/assets/98418a23-1b67-43ee-b6f4-77de6fecee98" />

Launch! comes with 8 handy accessories that can be useful in a basic DOS environment.

If selected during the Install program, there will be an **Accessories** menu created for you in Launch! providing quick access.

All programs depend on Launch! (`!.EXE`) - for shared UI toolkit, and will take on Launch! configuration settings such as color scheme and mouse cursor - so the `.EXE`s can't be distributed without `!.EXE`.<br/>
They've all been designed with a tiny memory footprint, and maximum compatibility in mind.

Usable with either keyboard or mouse interaction.

Any data files for each program are stored in the `data` sub-directory of Launch!. Exports will be placed in the `exports` sub-directory.

### !CAL.EXE - Calendar
A basic monthly calendar view. Use `Left` and `Right` arrow keys to navigate months, `Home` returns to the current month.

You can print the calendar out, it will create a full page calendar with big enough space to write. 

The calendar will show events from a `CAL.ICS` file (in RFC5545 iCalendar Specification) that is located beside `!CAL.EXE`

You can also use the `/FILE=` parameter to point to any `.ICS` file, and the Calendar will show events from that file.

<img width="720" height="600" alt="Calendar" src="https://github.com/user-attachments/assets/1ad0116f-b7ea-4aeb-b040-029da2b62a02" />

<img width="720" height="600" alt="Calendar - Day" src="https://github.com/user-attachments/assets/f5eebd56-4d9a-4e57-b22d-6d956226db99" />


### !CALC.EXE - Calculator
A simple calculator, with nice large digits, pretty self explanatory!

<img width="720" height="600" alt="Calculator" src="https://github.com/user-attachments/assets/789a27b8-fec4-412d-9428-3e43b1b42fc3" />


### !DRAW.EXE - Pixel Draw
You can create simple pixel images, they can be exported as a bitmap.<br/>
The canvas is 96x96 pixels, the viewable area is 30x12 and can be scrolled.<br/>
Exported bitmaps will be cropped to the visible contents. So if you draw a 32x32 icon, you'll get a 32x32 export.

Click a palette color and then draw in the drawing area. You can hold down the mouse button to draw, or click individual pixels. <br/>
The right mouse button erases.<br/>
The grid can be toggled on/off.

`Show` will render your drawing in large scale graphics mode fullscreen.

Your drawing is persisted and will be there when the program is re-opened.

<img width="720" height="600" alt="Pixel Draw" src="https://github.com/user-attachments/assets/bcf393ef-1f95-45b5-9f07-8eba7894b2af" />

### !JOURNAL.EXE - Journal
A simple and flexible daily journal. Easily navigate between days or go to a specific date.

Export and Print your journal too.

<img width="720" height="600" alt="Journal" src="https://github.com/user-attachments/assets/7ba5f603-9c57-41f8-9cf8-ea11773d6a59" />


### !NOTE.EXE - Note
Simple notepad. You can click and type anywhere.

Create tabs, and up to 10 pages per tab.

Export the contents to a text file with `Export`. You can also send the contents to the printer with `Print`.

The text contents are persisted when you close the Note, and will be there when the program is re-opened.

Choose `Clr` to erase the notepad. `Chars` shows a character palette to insert extended/special characters.

<img width="720" height="600" alt="Note" src="https://github.com/user-attachments/assets/c396f27e-8b76-4b04-b360-6eb6d6f61be9" />

### !STACK.EXE - Card Stack
A stack of index cards. Each card has a title and text, and you can navigate between them. Clicking a card title brings that card to the front of the stack for viewing/editing.

Add/Delete cards with the respective buttons.
You can also export all the cards (everything concatenated) into a text file.

Entries are automatically saved, everything is persisted, and all cards will be visible when the program is re-opened.

<img width="720" height="600" alt="!STACK" src="https://github.com/user-attachments/assets/b6a64b72-6a5c-4463-aa7a-7ea408abc868" />

### !SYSINFO.EXE - System Information
View useful information about your system, including free memory and disk space.

The system information report can be printed for reference.

<img width="720" height="600" alt="!SYSINFO" src="https://github.com/user-attachments/assets/c18489c2-9383-4881-8d5f-e909189b57b6" />

### !TODOS.EXE - To-dos
A simple and flexible To-Do list supporting grouping, due dates (with natural language like "tomorrow", "next Friday", extended description, and tags.

Tasks can be sorted and tags can be used to filter the list.

Short date entry and display (`DD-MM-YYYY` / `MM-DD-YYYY`) will depend on your locale settings as/if set by `COUNTRY.SYS` in your `CONFIG.SYS`.

<img width="720" height="600" alt="!TODOS" src="https://github.com/user-attachments/assets/cd920ca9-6dcb-4300-903a-b5634fde614d" />

### !TYPO.EXE - Typo
Level up on your words-per-minute and put that retro mechanical keyboard to good use, practicing your typing.

50 different lessons focusing on different skills and types of texts, including coding.

Records are maintained so you can keep track of your high scores.

<img width="720" height="600" alt="!TYPO" src="https://github.com/user-attachments/assets/aade6dc0-c109-488e-b86a-ac85069d7cfa" />


----
<img width="1633" height="250" alt="launch-games" src="https://github.com/user-attachments/assets/c4d31625-d74d-4534-a256-eb436e3ece78" />

Launch! comes with 5 addictive games to kill some time.

If selected during the Install program, there will be an **Games** menu created for you in Launch! providing quick access.


### !BOXES.EXE - Boxes
A Sokoban style puzzle game with 250 different challenges.

Use the arrow keys to move the worker and push boxes to cover all the targets - in as few moves as possible.

Puzzles 1-200 can be played out for you by choosing `Solve`. Levels 200-250, you must solve yourself!

<img width="720" height="600" alt="Boxes" src="https://github.com/user-attachments/assets/f925d3b6-fef8-4caf-a08a-416172b5619d" />

Puzzles are from Microban collections designed by [David W. Skinner](http://www.abelmartin.com/rj/sokobanJS/Skinner/David%20W.%20Skinner%20-%20Sokoban.htm)



### !FCELL.EXE - FreeCell
Play the FreeCell card game. Same interaction and interface as Solitaire.

<img width="720" height="600" alt="FreeCell Game" src="https://github.com/user-attachments/assets/6bd5c3e6-d075-4037-a273-1bac90b22d4a" />

### !PLUMB.EXE - Plumb

<img width="720" height="600" alt="Plumb Game" src="https://github.com/user-attachments/assets/9d2a6de9-735f-4966-8be0-55c1d32e1a23" />


### !POP.EXE - Pop
An implementation of the classic SameGame/CHAIN SHOT tile-matching game.

10 levels with different difficulty levels. Click groups of bubbles to select, and then click again (or Enter) to pop them away!

<img width="720" height="600" alt="Pop Game" src="https://github.com/user-attachments/assets/2f3fd389-ebce-4cfd-baa5-e3b6ea64baac" />


### !SNAKE.EXE - Snake
Eat the fruit before the timer runs out!
Watch out for walls and don't run into yourself

<img width="720" height="600" alt="Snake Game" src="https://github.com/user-attachments/assets/aa959862-5c53-424b-a8e1-1a09ea97e105" />

Playfields come from the Microsoft QBasic NIBBLES.BAS sample program.

### !SOL.EXE - Solitaire
Play draw three Solitaire completely in text mode!

Click a card (it will show as selected), then click the destination (valid destinations are indicated). You can double click a card to move it to the correct foundation pile (if a valid move).

<img width="720" height="600" alt="Solitaire Game" src="https://github.com/user-attachments/assets/68ce19d6-3416-46ba-9684-09d0573a2144" />


### !WORDZ.EXE - Wordz

<img width="720" height="600" alt="Wordz Game" src="https://github.com/user-attachments/assets/0a9fc9fb-a9f7-4fa2-9c44-74983466820c" />


----
<img width="1633" height="250" alt="Tooling" src="https://github.com/user-attachments/assets/dd935b02-2baa-4626-8c37-9cae052432f4" />

## SHORTCUT.COM - keyboard shortcut tool
The keyboard shortcut is provided by a separate utility as it is not required to use `!.EXE` on its own. 

If you don't load the keyboard shortcut tool, you'll regain 500 bytes of memory - although `LOADHIGH` is used with `SHORTCUT.COM` to move this to upper memory anyway.

If chosen, it will be added to `AUTOEXEC.BAT/FDAUTO.BAT` by install so the shortcut is available after startup.
The combination can be changed any time after install in Configuration.

You can change the keyboard shortcut used by manually adding the `/KEY=` parameter to `SHORTCUT.COM` with readable names or hexadecimal scan codes, e.g.Update README.md
```
  SHORTCUT /KEY=LWIN
  SHORTCUT /KEY=CTRL+SPACE
  SHORTCUT /KEY=CTRL+ALT+L
  SHORTCUT /KEY=1D+38+34
```
Tokens are separated by `+`. CTRL, ALT, SHIFT, PERIOD, DOT, SPACE, F1-F12, the Windows key, letters, digits, common punctuation are accepted. You can also use scan codes.<br/>

The shortcut utility can be removed from memory with `SHORTCUT /UNLOAD`.<br/>
Use `SHORTCUT /?` for more information.

To maintain system integrity and security, `SHORTCUT.COM` validates an authentication key in `!.EXE` to ensure the shortcut key will only ever execute (a genuine) Launch! menu.

## AUTOGEN.EXE - an automatic menu generator
The menu generator will scan your C:\ for recognized programs in its internal database (over 1000 DOS programs up to 1995).

This database has been extractss from [DirectAccess 5.19](https://winworldpc.com/product/direct-access/5x). 

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

----

<img width="720" height="600" alt="About" src="https://github.com/user-attachments/assets/28b12e02-a814-4f4d-9e41-43cfeafebb66" />

