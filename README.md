<img width="1633" height="250" alt="Launch! for DOS" src="https://github.com/user-attachments/assets/1aa6eb8a-98f3-4e74-a1ef-479e4f7a04ef" />

> A lightweight command launcher for any DOS, with huge features to improve the usability of the command prompt -- and loads of goodies to uplift an old DOS machine.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**

**Requires:** DOS 3.3, 80286, EGA or better. 1.2MB free disk space for install.

Works with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual machines, including DOSBox, 86Box and DOSEMU. Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="The ! Menu" src="https://github.com/user-attachments/assets/6d1e7461-0498-4f7e-aa40-a8514c435247" />

## Contents
- [Features](#features)
- [Installation](#install)
- [Menu](#the-menu)
- [Config](#config)
- [Screen Savers](#screen-savers)
- [Accessories](#accessories)
- [Games](#games)
- [Tooling](#tooling)

## Features
- Displays a hierarchical folder based menu, modally over the top of the existing console contents.
- Supports trigger by a customizable keyboard shortcut.
- Launches commands using the existing command interpreter and shell.
- Easy visual menu editing.
- Automatic menu generator with comprehensive DOS program database to automatically identify programs.
- Built in executable explorer to quickly browse and run programs anywhere.
- Built in file opener to create associations between files and launchers for easy open.
- Built in Power Off/Reboot control with retro Windows 95 power off experience.
- 10 handy and optional text-mode only accessories and 7 games to go with Launch!
    - [Calendar](#!cal), [Calculator](#!calc), [Card Stack](#!stack), [Journal](#!journal), [Markdown](#!mkdown), [Note](#!note), [Pixel Draw](#!draw) , [System Info](#!sysinfo), [To-dos](#!todos), [Typo](#!typo)
    - [Boxes](#!boxes), [Pop](#!pop), [Snake](#!snake), [Solitaire](#!sol), [FreeCell](#!fcell), [Plumb](#!plumb) and [Wordz](#!wordz)
- 14 awesome screensavers including a 7-segment digital clock, starry night skyline, bouncing DOS logo, warp field, 3D pipes, bouncing 3D ball, paintball, and more!
- Various Command Prompt styles to choose from to uplift your C:\
- Custom VGA display fonts to change the look of your whole DOS environment.
- Maximum compatibility across DOS versions (back to DOS 3.3) on real or emulated hardware/virtual machines.
- No libraries or dependencies including ANSI. Custom UI toolkit written in C. Fast and simple.
- Extremely minimal memory footprint. All resident components can be disabled to have zero memory impact if desired. 

----
<a id="install"></a>
# <img width="1633" height="250" alt="Install" src="https://github.com/user-attachments/assets/b7dee4b9-154f-47b6-b407-8b24ef9b03fd" />

Extract the release zip file or mount the floppy image:

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- You can choose whether to install accessories
- The necessary files will be copied and you will be prompted for all changes to `AUTOEXEC.BAT`
    - FreeDOS will be detected automatically and `FDAUTO.BAT` used instead of `AUTOEXEC.BAT` throughout Launch!
- The keyboard shortcut can be chosen
- Your drive can be scanned and an initial menu built.
- Simply reboot after install and you're ready to go.

<img width="360" height="300" alt="Install" src="https://github.com/user-attachments/assets/dfaa9257-91cb-4433-8509-30c65ba2e7fd" />

> The compatible shortcut key utility for your environment will automatically be selected.

> **If you already have Launch! installed**, choose the same directory and an upgrade will be performed keeping your existing configuration and menu in-tact.

----
<a id="the-menu"></a>
# <img width="1633" height="250" alt="The Menu" src="https://github.com/user-attachments/assets/833bd071-f0c6-436f-8cfa-fd933ac6685f" />

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

- `file.mnu` - use an alternative menu file from the default `launch.mnu`. It will be assumed to be beside `!.EXE` unless a full path is provided. This allows you to make use of different menu configurations, e.g.`! TEST.MNU`
- `/?` - show Launch! help
- `/CONFIG` - show the configuration dialog
- `/EXPLORE` - show the Explore & Run dialog directly without the menu
- `/OPEN` - show the Open File dialog directly without the menu
- `/BYE` - show the Shutdown... dialog directly without the menu
- `/NOW` - start the configured screensaver immediately
- `/OPENTO=folder` - open the Launch! menu to the specified folder, e.g. `/OPENTO="System Tools"` would show the menu with the System Tools sub-menu already open. 


### Keyboard usage

Menu management
- `Ctrl+A`        - Add a folder or launcher to the open menu
- `Ctrl+D`        - Delete the selected item after confirmation
- `Ctrl+E`        - Edit the selected folder or launcher
- `Ctrl+Up/Down`  - Move the selected item within its menu
- `Ctrl+S`        - Sort the open menu alphabetically

Menu navigation
- `Up/Down`       - Select an entry
- `Right`         - Open a selected folder
- `Left`          - Close the current folder
- `Enter`         - Open a folder or send a launcher command to the prompt
- `Esc`           - Close the complete menu

UI navigation
- `Tab`/`Shift+Tab` - Cycle between UI elements
- `Up/Down/Left/Right` - Navigate content area/playfield
- `Space`           - Selects
- `Enter`           - Commits
- `Esc`             - Cancels
- `F11`             - Toggle full screen mode


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


### Icons
The following icons are used across Launch!

| <img width="47" height="47" alt="OK" src="https://github.com/user-attachments/assets/3d44f57e-0bb0-4e0a-8790-d15145878684" /> | <img width="47" height="47" alt="Cancel" src="https://github.com/user-attachments/assets/cdee5d3e-b9c5-4595-9da1-15a2ded7f117" /> | <img width="47" height="47" alt="help" src="https://github.com/user-attachments/assets/fb9af3a9-cb9f-4f5c-8bd1-ac795a625c39" /> | <img width="47" height="47" alt="Retry" src="https://github.com/user-attachments/assets/64b6787b-aa38-48e7-b54c-ecbea32129da" /> | <img width="47" height="47" alt="Add" src="https://github.com/user-attachments/assets/24a71932-e7b8-4d8b-b4b2-29f38392b170" /> | <img width="47" height="47" alt="Edit" src="https://github.com/user-attachments/assets/aeadd957-d056-4d8e-b744-6a2a3a999416" /> | <img width="47" height="47" alt="Delete" src="https://github.com/user-attachments/assets/14c7f094-f3f4-4aa7-b04a-9dbe0a483d08" /> | <img width="47" height="47" alt="Export/Save" src="https://github.com/user-attachments/assets/92787df1-24a7-48e0-9155-f7a20053d02f" /> | <img width="47" height="47" alt="Print" src="https://github.com/user-attachments/assets/b6f46c46-61d2-47b8-95e1-e3f0eca69a17" />
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| OK | Cancel | Help | Retry | Add | Edit | Delete | Export | Print |


## Open File
`Open File` is a fixed special item at the bottom of the menu (removable in Config).
It allows you to browse for files, based on configured associations, and open the file with the configured launcher.

As an example, this can allow you to browse and open images with a launcher for an image viewing program - without having to type a massive command line and file path out yourself. Or browse zip files, and easily extract with your ZIP program, adding further parameters if required (like extraction directory).

There's a lot of flexibility to enable you to set up file-based workflows in tandem with your Launch! menu.

<img width="360" height="300" alt="Open File" src="https://github.com/user-attachments/assets/9e3acdbb-8f0c-445d-a39a-66eaf8fead6b" />

An association consists of one or more file extensions, and a selected launcher from your menu.

<img width="360" height="300" alt="Create association" src="https://github.com/user-attachments/assets/3bc0d834-9157-4497-b0f5-dc678666e6b0" />

You can also open files with further parameters if required.

<img width="360" height="300" alt="Open file with parameters" src="https://github.com/user-attachments/assets/718202b0-bf7b-4725-a002-df009b827725" />

The `Open File` dialog can also be shown on its own, without the menu, by running `! /OPEN`.

## Explore & Run
`Explore & Run` is a fixed item at the bottom of the menu.<br/>
When selected you will be able to browse the file-system for executable programs and quickly run them, exactly as launchers are run from the menu.<br/>
Enter will open the selected directory, or run the selected executable. You can also double-click entries with the mouse for the same effect.

The drive bar shows the available disk drives, and allows you to switch between them.

The path bar shows a preview of the command line that will be executed when `Run` is chosen.

<img width="360" height="300" alt="Explore" src="https://github.com/user-attachments/assets/558f8ff2-dbae-435a-a7f9-139f8f740f79" />

Choosing `Params` for an executable will show the parameter entry dialog, so you can provide the desired parameters before launching the command.

<img width="360" height="300" alt="Run with parameters" src="https://github.com/user-attachments/assets/60b7f23a-5b96-40cc-8ae9-72864a925c10" />

You can remove the `Explore & Run` menu item in configuration (`! /CONFIG`)<br/>

The dialog can also be shown on its own, without the menu, by running `! /EXPLORE`.


## Shutdown
The `Shutdown...` menu item is also fixed at the bottom of the menu, and when selected shows a dialog for you to Power Off or Reboot.<br/>

Power Off flushes DOS and SMARTDrive buffers first and then uses power management APM/ACPI calls to power off. That may not work on older machines of course, in which case, the Power off message from Windows 9x will be shown.<br/>

<img width="360" height="300" alt="Shutdown..." src="https://github.com/user-attachments/assets/55a8a298-83e7-4010-afdf-4f2f5d96ae4c" />

You can remove the `Shutdown...` menu item in configuration (`! /CONFIG`). <BR/>
The dialog can also be shown on its own, without the menu, by running `! /BYE`.

<img width="360" height="300" alt="Power off bitmap" src="https://github.com/user-attachments/assets/2eee9995-eed2-49a5-8f5f-6753d8423140" />

If desired, you can replace this image as `PWROFF.BMP` with any 320x400 256 color bitmap - it is stretch to full screen.


----
<a id="config"></a>
# <img width="1633" height="250" alt="Config" src="https://github.com/user-attachments/assets/281d5d70-bcd8-4730-baf8-df572e7006bf" />

Run `! /CONFIG` to configure Launch! appearance and preferences. You can also right-click on the main "Launch!" menu title.<br/>
Options are split across 6 tabs.
Cancel will return the previous configuration.

Settings are saved to `LAUNCH.CFG`, a plain text file you can also edit yourself.<br/>
If `LAUNCH.CFG` is absent or malformed, Launch! uses the defaults. 

### Menu

You can choose where the menu is positioned, what options are displayed, the time format, and the style of cursor to use.

<img width="360" height="300" alt="Config: Menu" src="https://github.com/user-attachments/assets/908ca5e2-248e-4d4c-b4e4-e6dbe80fa203" />

Contextual help (tooltips) can be turned on (off by default). They will appear throughout Launch! with keyboard focus or mouse-over.

The SysBar (off by default) shows an information bar, top-right of screen every time the menu is opened with some handy system status information.

<img width="360" height="300" alt="SysBar" src="https://github.com/user-attachments/assets/bd4bb85c-87a3-4da7-910e-8f776f99d824" />

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

<img width="360" height="300" alt="Config: Colors" src="https://github.com/user-attachments/assets/1b53c115-8717-49a7-9d26-6cdb2c566d2e" />

<img width="360" height="300" alt="Color Schemes" src="https://github.com/user-attachments/assets/189807a7-5fd1-4b88-98e5-b120dc750bcc" />


### Screen Saver

You can select which screensaver to show, choose `None` to disable this functionality.<br/>
You can also choose the time after which the screensaver will activate (1, 5, 15 or 30 minutes).<br/>
Click `Preview` for an instant preview of the currently selected screensaver.

<img width="360" height="300" alt="Config: ScreenSavers" src="https://github.com/user-attachments/assets/60c45c31-fc64-4f8a-8357-f2d15d9816a8" />

If the shortcut utility is loaded, the inactivity monitoring will apply to the command prompt and the menu - so your screensaver will also start if there's inactivity at the command prompt.<br/>
If the shortcut utility is not loaded, the screensaver will only start when the menu is open. 

### Prompt

You can easily change the appearance of the DOS command prompt, choosing from 10 different prompt styles.

You can also add your own prompt definitions in `PROMPTS.CFG` and they will appear in the selector here.

Choosing `SET` applies the selected prompt style immediately, and also updates any existing `PROMPT` statement in `AUTOEXEC.BAT/FDAUTO.BAT` so this style is set at startup.

Styles using ANSI escape sequences will not show in the selector if an ANSI driver is not detected.

<img width="360" height="300" alt="Config: Prompt" src="https://github.com/user-attachments/assets/0808a681-5930-4af9-a405-37919ae95bd7" />

###  Font

You can change the VGA font used across the entire DOS session. There's 22 different fonts to choose from. `Standard` uses the system VGA BIOS rom font. 

<img width="360" height="300" alt="Config: Font" src="https://github.com/user-attachments/assets/b9968688-1705-4977-97cf-c64eb6241b8f" />

If you choose `Persist`, Launch! will forcefully keep your font applied, even after screen mode changes. However, that will consume 4KB of lower memory (tiny, but that could be all the difference in some circumstances). Without `Persist`, no extra memory is consumed, Launch! will re-apply your font each time the menu is shown. Unchecking `Persist` if already active at any time, will release the 4KB of memory back.

> Note this feature is not available with an EGA display adapter.

**Font Choices**

<img width="456" height="50" alt="LAUNCH" src="https://github.com/user-attachments/assets/bf7c725c-c644-4c8e-9e5d-c3a8348ef29f" />
<img width="456" height="50" alt="BIG" src="https://github.com/user-attachments/assets/541640d9-7bff-424b-8ebd-cf4254d645dc" />
<img width="456" height="50" alt="BOLD" src="https://github.com/user-attachments/assets/dd5714d2-3cf3-4a1d-8b73-c97df3402d26" />
<img width="456" height="50" alt="BOLDALT" src="https://github.com/user-attachments/assets/ef418c8a-506a-4a34-b409-902c176b9188" />
<img width="456" height="50" alt="CHUNKY" src="https://github.com/user-attachments/assets/ceeb7cb3-1dc5-4c8a-b6f7-dee2be662ee6" />
<img width="456" height="50" alt="CLEAN" src="https://github.com/user-attachments/assets/188a18f4-67f4-43b3-989b-f40d91a6d716" />
<img width="456" height="50" alt="ELERGON" src="https://github.com/user-attachments/assets/fbe407be-5569-4161-a569-22f482bc6524" />
<img width="456" height="50" alt="ELITE" src="https://github.com/user-attachments/assets/a466dd82-b881-40f6-b99d-abc11ac82374" />
<img width="456" height="50" alt="EXTRA" src="https://github.com/user-attachments/assets/9822d908-b454-4c0b-bc3b-dee61f2ad51e" />
<img width="456" height="50" alt="HUMANIST" src="https://github.com/user-attachments/assets/568a3425-4808-44eb-b4f9-d468c776e0e4" />
<img width="456" height="50" alt="ISO" src="https://github.com/user-attachments/assets/4429ca61-000c-45d2-a7b2-719d36c3edb2" />
<img width="456" height="50" alt="ITALIC" src="https://github.com/user-attachments/assets/52072ead-4657-41b6-b271-d797249fd8fe" />
<img width="456" height="50" alt="MAX" src="https://github.com/user-attachments/assets/b5df456a-45ef-4b90-bd18-6ebe74856b9c" />
<img width="456" height="50" alt="MAXELITE" src="https://github.com/user-attachments/assets/92846791-3fb2-43cd-b556-2bdcff84269c" />
<img width="456" height="50" alt="NEAT" src="https://github.com/user-attachments/assets/29d732a8-adaf-434b-a6ad-49c08e5b31be" />
<img width="456" height="50" alt="NEATALT" src="https://github.com/user-attachments/assets/b4ad7047-188a-41fa-9696-21745cd269f4" />
<img width="456" height="50" alt="OLDENG" src="https://github.com/user-attachments/assets/0d2e90a2-9c04-40eb-961c-964604d9b39b" />
<img width="456" height="50" alt="PIXEL" src="https://github.com/user-attachments/assets/23159908-d319-4be8-80d9-881c6fd60847" />
<img width="456" height="50" alt="PROFONT" src="https://github.com/user-attachments/assets/0d1dda69-25f5-408c-b43f-ab7479a19e20" />
<img width="456" height="50" alt="PROFONT BOLD" src="https://github.com/user-attachments/assets/af4ac8de-0655-4c89-9760-c8687f730142" />
<img width="456" height="50" alt="ROMAN" src="https://github.com/user-attachments/assets/47e7b176-8e0b-478a-8c8f-d4fa4922f6f2" />
<img width="456" height="50" alt="SCRIBBLE" src="https://github.com/user-attachments/assets/1cc68f2e-d1b4-4a9f-acb7-44e9f2867c80" />
<img width="456" height="50" alt="SCRIPT" src="https://github.com/user-attachments/assets/9015e22c-1d93-467a-88bb-7b913e6dd2f6" />
<img width="456" height="50" alt="TALL" src="https://github.com/user-attachments/assets/7a7a6567-b933-468a-bc48-1cef33b64945" />

### Shortcut

You can view the current status of the shortcut utility, current key combination, and set a new combination.<br/>
If the shortcut is active, you can unload it - removing all traces of the TSR from memory.<br/>
If the shortcut is inactive, you can activate the shortcut - which will install the required line in your `AUTOEXEC.BAT/FDAUTO.BAT` and reboot.<br/>
Note that changing the key combination also requires a reboot to apply.

<img width="360" height="300" alt="Config: Shortcut" src="https://github.com/user-attachments/assets/93d6ccf2-262a-46cc-be95-2b2fe05af8a9" />


### Reset

If you mess up your Launch! installation you can reset the configuration, the menu, or both by choosing Reset.

This will return things to the defaults, and rebuild a default menu as well.

<img width="360" height="300" alt="Config: Reset" src="https://github.com/user-attachments/assets/0b432c76-2841-4ca3-a326-4078efba75fc" />


## Live menu management
Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is kept at the bottom when the menu is sorted.

`CTRL+A` allows you to add a new folder, launcher, or separator to the currently visible menu panel.

<img width="360" height="300" alt="Add item" src="https://github.com/user-attachments/assets/feaf075d-39a6-4a57-bf7d-5a388474904a" />

`CTRL+D` will remove the selected item from the menu (with confirmation first).

<img width="360" height="300" alt="Remote item" src="https://github.com/user-attachments/assets/1daef798-e7be-4cc8-b41e-2de77e4aa70e" />

You can move selected items up and down with `CTRL+↑/↓`.

`CTRL+E` will show you the edit dialog to modify a launcher.

<img width="360" height="300" alt="Edit item" src="https://github.com/user-attachments/assets/d609be98-e703-4f89-98ee-f1694bac0176" />

If a launcher has `Prompt?` activated, when launched from the menu, the parameter entry dialog will be displayed.<br/>
You will be able to enter parameters before running the program.

**Provide enter after launcher command** if toggled off will just type the command, not supplying Enter - allowing you to then add more to the command line (such as further parameters) or review it, before execution.

When **Change directory first** is selected, Launch! extracts the directory from the first command token when the launcher is run.<br/>
E.g. for `C:\TOOLS\APP.EXE` it types `C:`, presses Enter, types `CD C:\TOOLS`, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

If **Add to PATH for execution** is selected, the command's directory will be added to the environment %PATH% variable. Many programs require this, and will update AUTOEXEC.BAT/FDAUTO.BAT - which can get unwieldy. This allows you do away with that, and just add a program to PATH when launched from the menu.

<img width="360" height="300" alt="Run with parameters" src="https://github.com/user-attachments/assets/9f596c4b-8f47-4ea0-9f93-da1ee0c199e9" />


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
<a id="screen-savers"></a>
# <img width="1633" height="250" alt="Screen Savers" src="https://github.com/user-attachments/assets/60492c02-09ae-4846-8761-b318f9ddb504" />

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

<a id="accessories"></a>
# <img width="1633" height="250" alt="Accessories" src="https://github.com/user-attachments/assets/98418a23-1b67-43ee-b6f4-77de6fecee98" />

Launch! comes with 9 handy accessories that can be useful in a basic DOS environment.

If selected during the Install program, there will be an **Accessories** menu created for you in Launch! providing quick access.

All programs depend on Launch! (`!.EXE`) - for shared UI toolkit, and will take on Launch! configuration settings such as color scheme and mouse cursor - so the `.EXE`s can't be distributed without `!.EXE`.<br/>
They've all been designed with a tiny memory footprint, and maximum compatibility in mind.

Usable with either keyboard or mouse interaction.

Any data files for each program are stored in the `data` sub-directory of Launch!. Exports will be placed in the `exports` sub-directory.

<a id="!cal"></a>
### !CAL.EXE - Calendar
A basic monthly calendar view. Use `Left` and `Right` arrow keys to navigate months, `Home` returns to the current month.

You can print the calendar out, it will create a full page calendar with big enough space to write. 

The calendar will show events from a `CAL.ICS` file (in RFC5545 iCalendar Specification) that is located beside `!CAL.EXE`

**Open files**: You can also use add one or more `.ICS` files as a parameter, and the Calendar will show events from those files.<br/>
e.g. `!CAL C:\DOCS\HOLIDAYS.ICS C:\DOCS\BDAYS.ICS`

<img width="360" height="300" alt="Calendar" src="https://github.com/user-attachments/assets/9db4cf53-9938-4e35-8381-a7e1633a2cfa" />
<img width="360" height="300" alt="Calendar - Day View" src="https://github.com/user-attachments/assets/d1dbe2c3-6d0a-48fd-9887-b3d2ed2f7f28" />

<a id="!calc"></a>
### !CALC.EXE - Calculator
A simple calculator, with nice large digits, pretty self explanatory!

<img width="360" height="300" alt="Calculator" src="https://github.com/user-attachments/assets/3bb84bf7-383d-4f8e-b92b-0a7ada006c62" />

<a id="!draw"></a>
### !DRAW.EXE - Pixel Draw
You can create simple pixel images, they can be exported as a bitmap.<br/>
The canvas is 96x96 pixels, the viewable area is 30x12 and can be scrolled.<br/>
Exported bitmaps will be cropped to the visible contents. So if you draw a 32x32 icon, you'll get a 32x32 export.

Click a palette color and then draw in the drawing area. You can hold down the mouse button to draw, or click individual pixels. <br/>
The right mouse button erases.<br/>
The grid can be toggled on/off.

Your drawing is persisted and will be there when the program is re-opened.

<img width="360" height="300" alt="Draw" src="https://github.com/user-attachments/assets/af350e87-e4fa-4629-9c8e-316bb29e34ce" />
<img width="360" height="300" alt="Show" src="https://github.com/user-attachments/assets/fd5e02c8-e5cd-4c01-9ad7-d34cb8b8a742" />
<img width="360" height="300" alt="Full-screen Draw" src="https://github.com/user-attachments/assets/96a259e4-1f04-4af3-9805-eb71fe373c43" />

`Show` will render your drawing in large scale graphics mode.
You can toggle full-screen editing with F11, or the Maximize icon in the titlebar.

**Open files**: `!DRAW.EXE` accepts a filename as a parameter for any 16-color BMP image. It will load this image into the canvas for viewing/editing. e.g. `!DRAW C:\ICON.BMP`

<a id="!journal"></a>
### !JOURNAL.EXE - Journal
A simple and flexible daily journal. Easily navigate between days or go to a specific date.

Export and Print your journal too.

Text can be marked holding down the `Shift` key - and then Cut (`CTRL+X`), Copied (`CTRL+C`) or Pasted (`CTRL+V`) anywhere on the page.

<img width="360" height="300" alt="Journal" src="https://github.com/user-attachments/assets/2d4f1a47-4cd7-43e9-b4e0-567b5c2768f2" />

<a id="!mkdown"></a>
### !MKDOWN.EXE - Markdown
A simple markdown editor with split-screen live preview, full-screen graphical preview (font formatting and styles), and focus mode for a distraction free retro word processing experience.
All markdown syntax [in this document](https://github.com/therenegar/launch/blob/main/res/SAMPLE.MD) is supported, and will be 'rendered' in the live preview or graphical preview (including tables!).

<img width="360" height="300" alt="Markdown" src="https://github.com/user-attachments/assets/90dfdb60-1e91-4338-b138-4c67b08c17a8" />

The `Split` button toggles between markdown entry only, split-screen preview, and preview only. 

`Show` displays a rendered full-screen graphical preview of the markdown - up/down/PgUp/PgDn scrolls, and Esc returns to the editor. 

There's also a Focus mode for distraction free writing.

<img width="360" height="300" alt="Live Preview" src="https://github.com/user-attachments/assets/17c52b20-da1e-4969-8442-bdd75c1c9920" />
<img width="360" height="300" alt="Graphical Preview" src="https://github.com/user-attachments/assets/f038ef00-ae08-4b02-90af-f3cbdbf8eef9" />
<img width="360" height="300" alt="Focus Mode" src="https://github.com/user-attachments/assets/cb198856-6499-4c8c-9fd8-05c5266dc6e8" />

> Note the graphic preview requires a VGA display adapter.

**Open files**: You can open one or more markdown (`.MD`) files by adding as a parameter to `!MKDOWN.EXE`, e.g. `!MKDOWN C:\DOCS\REPORT.MD`

Save and print the current page with the respective buttons.

And you can press `F11` or the Maximize titlebar button to go full-screen.

<img width="360" height="300" alt="Full-screen Editing" src="https://github.com/user-attachments/assets/051c91cb-c9ee-456b-8fc0-f0f31dbbf441" />

<a id="!note"></a>
### !NOTE.EXE - Note
Simple notepad. You can click and type anywhere!

Create tabs, and up to 10 pages per tab. Export the contents to a text file with `Export`. You can also send the contents to the printer with `Print`.
The text contents are persisted when you close the Note, and will be there when the program is re-opened.

Choose `Clr` to erase everything on the current page. `Chars` shows a character palette to insert extended/special characters.

Text can be marked holding down the `Shift` key - and then Cut (`CTRL+X`), Copied (`CTRL+C`) or Pasted (`CTRL+V`) anywhere on the page, other pages, or pages in other tabs.

Words will wrap at the end of each line intelligently to the next, for natural typing.

<img width="360" height="300" alt="Note" src="https://github.com/user-attachments/assets/a447c391-17c7-4a6b-9e52-1fe82083b958" />

**Open Files**: You can add the filename/s (up to 10) of a text file after `!NOTE.EXE` and Note will open your external text file (in a tab labelled 'External' - one file per page). Any changes be saved (overwrite) with the Export button - which will save all opened files. Externally opened files are not persisted.
e.g. `!NOTE C:\DOCS\CONTACTS.TXT C:\DOCS\BDAYS.TXT`

Press `F11` or the Maximize titlebar button to go full-screen editing.

<img width="360" height="300" alt="Full screen editing" src="https://github.com/user-attachments/assets/9d2da201-f4c5-476b-9909-0c7287922385" />


<a id="!stack"></a>
### !STACK.EXE - Card Stack
A stack of index cards. Each card has a title and text, and you can navigate between them. Clicking a card title brings that card to the front of the stack for viewing/editing.

Add/Delete cards with the respective buttons.
You can also export all the cards (everything concatenated) into a text file.

Entries are automatically saved, everything is persisted, and all cards will be visible when the program is re-opened.

<img width="360" height="300" alt="Card Stack" src="https://github.com/user-attachments/assets/4f73850e-c67d-4a56-9b0b-b01001673bdd" />

<a id="!sysinfo"></a>
### !SYSINFO.EXE - System Information
View useful information about your system, including free memory and disk space.

The system information report can be printed for reference.

<img width="360" height="300" alt="System Information" src="https://github.com/user-attachments/assets/258f9b7c-bc72-4bbb-8743-ad280804bb4a" />

<a id="!todos"></a>
### !TODOS.EXE - To-dos
A simple and flexible To-Do list supporting grouping, due dates (with natural language like "tomorrow", "next Friday", extended description, and tags.

Tasks can be sorted and tags can be used to filter the list.

Short date entry and display (`DD-MM-YYYY` / `MM-DD-YYYY`) will depend on your locale settings as/if set by `COUNTRY.SYS` in your `CONFIG.SYS`.

Some secret hotkeys<br/>
`A` Adds a task, `E` Edits selected, `D` Deletes selected `S` sorts the active group.

<img width="360" height="300" alt="To-Dos" src="https://github.com/user-attachments/assets/4ec01403-7f3a-4391-9a39-98283af626d1" />

<a id="!typo"></a>
### !TYPO.EXE - Typo
Level up on your words-per-minute and put that retro mechanical keyboard to good use, practicing your typing.

100 different lessons with different skills and types of texts, and a focus on technical writing and coding.

Records are maintained so you can keep track of your achievements.

<img width="360" height="300" alt="Typo" src="https://github.com/user-attachments/assets/15ca7b6f-9332-4796-add8-0ad5ce87b576" />


----
<a id="games"></a>
# <img width="1633" height="250" alt="Games" src="https://github.com/user-attachments/assets/c4d31625-d74d-4534-a256-eb436e3ece78" />

Launch! comes with 7 addictive games to kill some time. My attempt at creating a **DOS Entertainment Pack** if you will. And also a VGA/EGA font plane glyph manipulation game engine for graphics in text mode DOS.

If selected during the Install program, there will be a **Games** menu created for you in Launch! providing quick access.

<a id="!boxes"></a>
### !BOXES.EXE - Boxes
A Sokoban style puzzle game with 250 different challenges.

Use the arrow keys to move the worker and push boxes to cover all the targets - in as few moves as possible.

Puzzles 1-200 can be played out for you by choosing `Solve`. Levels 200-250, you must solve yourself!

<img width="360" height="300" alt="Boxes" src="https://github.com/user-attachments/assets/c92312ce-49d5-4eb4-b966-f5ce4c08d4db" />

Puzzles are from Microban collections designed by [David W. Skinner](http://www.abelmartin.com/rj/sokobanJS/Skinner/David%20W.%20Skinner%20-%20Sokoban.htm)

<a id="!fcell"></a>
### !FCELL.EXE - FreeCell
Play the FreeCell card game. Same interaction and interface as Solitaire.

<img width="360" height="300" alt="FreeCell" src="https://github.com/user-attachments/assets/f2f4f0c3-b7c0-42c8-8dd5-a2e8efc701b1" />

<a id="!plumb"></a>
### !PLUMB.EXE - Plumb
Lay some pipe and stop the leaking slime before it's too late!

<img width="360" height="300" alt="Plumb" src="https://github.com/user-attachments/assets/e32affd0-d03f-42fc-9801-483495a66dc1" />

<a id="!pop"></a>
### !POP.EXE - Pop
An implementation of the classic SameGame/CHAIN SHOT tile-matching game.

10 levels with different difficulty levels. Click groups of bubbles to select, and then click again (or Enter) to pop them away!

<img width="360" height="300" alt="Pop" src="https://github.com/user-attachments/assets/03730b5e-1da7-476a-9f76-e80f5dc50de2" />

<a id="!snake"></a>
### !SNAKE.EXE - Snake
Eat the fruit before the timer runs out!
Watch out for walls and don't run into yourself

<img width="360" height="300" alt="Snake" src="https://github.com/user-attachments/assets/0df26043-9abd-480c-8e88-f0b344ff3b63" />

Levels 1-10 come from the Microsoft QBasic NIBBLES.BAS sample program. Beyond level 10 playfields are randomly generated (and might be crazy).

<a id="!sol"></a>
### !SOL.EXE - Solitaire
Play draw three Solitaire completely in text mode!

Click a card (it will show as selected), then click the destination (valid destinations are indicated). With keyboard, `Space` selects a card, navigate to destination with arrow keys and press `Enter` to commit.

<img width="360" height="300" alt="Solitaire" src="https://github.com/user-attachments/assets/68daf4d3-1f61-41a8-876b-31d71b65d558" />

<a id="!wordz"></a>
### !WORDZ.EXE - Wordz

Over 200 word-find style challenges.
Use the hints to find all the hidden words in the letter grid.

Click on letters, or click and drag to select a word. `Enter` commits your selection. `Backspace` un-selects the last selected letter.

Words can be horizontal, diagonal down and up. Words will not be on the grid backwards.

<img width="360" height="300" alt="Wordz" src="https://github.com/user-attachments/assets/368eae31-ce50-4475-bed6-93e2863b520f" />


----
<a id="tooling"></a>
# <img width="1633" height="250" alt="Tooling" src="https://github.com/user-attachments/assets/dd935b02-2baa-4626-8c37-9cae052432f4" />

## !KEY.COM - keyboard shortcut tool
The keyboard shortcut is provided by a separate utility as it is not required to use `!.EXE` on its own. 

If you don't load the keyboard shortcut tool, you'll regain 500 bytes of memory - although `LOADHIGH` is used with `SHORTCUT.COM` to move this to upper memory anyway.

If chosen, it will be added to `AUTOEXEC.BAT/FDAUTO.BAT` by install so the shortcut is available after startup.
The combination can be changed any time after install in Configuration.

You can change the keyboard shortcut used by manually adding the `/KEY=` parameter to `!KEY.COM` with readable names or hexadecimal scan codes, e.g.
```
  !KEY /KEY=LWIN
  !KEY /KEY=CTRL+SPACE
  !KEY /KEY=CTRL+ALT+L
  !KEY /KEY=1D+38+34
```
Tokens are separated by `+`. CTRL, ALT, SHIFT, PERIOD, DOT, SPACE, F1-F12, the Windows key, letters, digits, common punctuation are accepted. You can also use scan codes.<br/>

The shortcut utility can be removed from memory with `!KEY /UNLOAD`.<br/>
Use `!KEY /?` for more information.

To maintain system integrity and security, `!KEY.COM` validates an authentication key in `!.EXE` to ensure the shortcut key will only ever execute (a genuine) Launch! menu.

Separate executables with the same functionality are provided to extend capability to specific architectures; `!KEYDB.COM` is for DOSBox users, `!KEY286.COM` is for 286 machines.

The installer automatically installs the correct version.

## !MNUGEN.EXE - an automatic menu generator
The menu generator will scan your C:\ for recognized programs in its internal database (over 1000 DOS programs up to 1995).

This database has been extracted from [DirectAccess 5.19](https://winworldpc.com/product/direct-access/5x). 

You will be prompted to resolve any ambiguous items found.
It will also detect your installed DOS version and build a DOS command menu.
This menu will replace any existing `LAUNCH.MNU` file, saving the existing menu as `LAUNCH.BAK` first.

`!MNUGEN` is run as part of the install process, but can be run at any time. 

You can use the `/LOOKIN=C,D,E` parameter to change the drives the tool will search for programs. By default, only C. One or more drives can be specified, separated by a comma.
Use `!MNUGEN /?` for more information.


## File safety
Launch! validates `LAUNCH.MNU` before opening the menu. A valid file must contain the [Launcher] root section and every non-comment line must be a valid section, FOLDER, or ITEM record. Empty, truncated, malformed, or oversized records are rejected.

Before every accepted Add, Edit, Delete, Move, or Sort operation, the existing valid `LAUNCH.MNU` is copied to `LAUNCH.BAK`. The new menu is first written fully to `LAUNCH.$$$` and is installed only after writing succeeds. `LAUNCH.BK$` is used briefly while rotating the backup.

If `LAUNCH.MNU` is missing or invalid at startup and `LAUNCH.BAK` is valid, Launch! restores the backup automatically and displays a recovery message. If neither file is usable, the built-in sample menu is installed as both `LAUNCH.MNU` and `LAUNCH.BAK`. An invalid primary file is preserved as `LAUNCH.BAD` when possible.

----

Launch is created on a 486DX4/100 machine, 32mb RAM, running IBM PC DOS 7.0. Coded using [Microsoft QuickC](https://en.wikipedia.org/wiki/QuickC) IDE and [FTE](https://fte.sourceforge.net/) (Folding Text Editor). Graphical glyphs created using [Fontraption](https://int10h.org/blog/2019/05/fontraption-vga-text-mode-font-editor/). Screen layout and composition done with [TheDraw](https://www.abandonwaredos.com/abandonware-game.php?abandonware=TheDraw+4&gid=3563).
Compiled with Microsoft C/C++ Optimizing Compiler 7.00 from 1992. 
Screenshots taken on this machine using [Screen Thief](http://www.win3x.org/win3board/viewtopic.php?t=2710&view=min).

<img width="360" height="300" alt="About" src="https://github.com/user-attachments/assets/ea323dd1-dcff-45e3-b721-b02e66b0cbea" />
