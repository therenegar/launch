# Launch!

A lightweight launcher for any DOS, with huge features to improve the usability of the command prompt - and make it more convenient and friendly.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**
> You can download as either a .ZIP file or a 1.44MB floppy disk image .IMG

Requires DOS 3.3, 80286, EGA or better. <br/>
Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual machines, including DOSBox.<br/>
Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="menu" src="https://github.com/user-attachments/assets/8a82f1c1-0677-4209-adde-b0f797ecbb21" />


## Features
- Displays a hierarchical folder based menu, modally over the top of the existing console contents.
- Supports trigger by a customizable keyboard shortcut.
- Launches commands using the existing command interpreter and shell.
- Easy visual menu editing.
- Automatic menu generator with comprehensive DOS program database to automatically identify programs.
- Program parameter help screen to make it easy supplying parameters to any menu item.
- Built in executable explorer to quickly browse and run programs anywhere.
- Built in Power Off/Reboot control with retro Windows 95 power off experience.
- Awesome screensavers including a 7-segment digital clock, starry night skyline, bouncing DOS logo, warp field, 3D pipes and mystifying polygons.
- Custom VGA display fonts to change the look of your whole DOS environment.
- Maximum compatibility across DOS versions (back to DOS 3.3) on real or emulated hardware/virtual machines.
- No libraries or dependencies including ANSI.


## Install it

Extract the release zip file or mount the floppy image:

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- The necessary files will be copied and you will be prompted for all changes to `AUTOEXEC.BAT`
- The keyboard shortcut can be chosen
- Your drive can be scanned and an initial menu built from a comprehensive database of over 1000 DOS programs.
- Simply reboot after install and you're ready to go.

> DOSBox installs will be detected by the installer, and a different DOSBox compatible version of the shortcut key tool will be installed.


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

<img width="70" height="70" alt="cursor" src="https://github.com/user-attachments/assets/c0fb9398-7623-408e-bc30-d3d7a6beb940" />

The arrow shaped cursor will be visible when Launch! is run.

- Left click opens a folder or runs a launcher; left click outside all visible menu panels closes Launch!.
- Right click an item opens its Edit dialog. 


## Keyboard shortcut

You can change the keyboard shortcut any time after install by running `! /SETKEY`<br/>
Press the desired keys, your `AUTOEXEC.BAT` will be updated. Reboot to activate the new shortcut.

<img width="720" height="183" alt="setkey" src="https://github.com/user-attachments/assets/9aac80e7-d60d-4752-ac5d-c0049709ffce" />

## Configuration

Run `! /CONFIG` to configure Launch! appearance and preferences. You can also right-click on the Launch! menu title.<br/>
Options are split across 4 tabs.
Cancel will return the previous configuration.

Settings are saved to `LAUNCH.CFG`, a plain text file you can also edit yourself.<br/>
If `LAUNCH.CFG` is absent or malformed, Launch! uses the defaults. 

### Colors

You can change the colors for all elements in the user interface.

<img width="720" height="600" alt="Color Configuration" src="https://github.com/user-attachments/assets/e06c8d7a-3660-49d7-a331-88a0b3a3a9b3" />

### Menu

You can choose where the menu is positioned, what options are displayed, and the time format.

<img width="720" height="600" alt="Menu Configuration" src="https://github.com/user-attachments/assets/dc177f62-9ff0-4820-b7c6-46dbaf92ad21" />

### Screensaver

You can select which screensaver to show, choose None to disable this functionality. You can also choose the time after which the screensaver will activate (1, 5, 15 or 30 minutes).

<img width="720" height="600" alt="Screensaver Configuration" src="https://github.com/user-attachments/assets/cf391032-b969-4070-96b7-f2bef98ede56" />

###  Font

You can change the VGA font used across the entire DOS session. There's 22 different fonts to choose from. `Standard` uses the system VGA BIOS rom font. 

If you choose `Persist`, Launch! will forcefully keep your font applied, even after screen mode changes. However, that will consume 4KB of lower memory. Without `Persist`, no extra memory is consumed. Launch! will still re-apply your font each time the menu is shown anyway.

<img width="720" height="600" alt="Font Configuration" src="https://github.com/user-attachments/assets/be8565de-6f71-4575-8906-5f7f17d9bde8" />

> Note this feature is not available with an EGA display adapter.


## Live menu management
Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is kept at the bottom when the menu is sorted.

`CTRL+A` allows you to add a new folder, launcher, or separator to the currently visible menu panel.

<img width="720" height="600" alt="add" src="https://github.com/user-attachments/assets/de6261c8-c15b-4dd0-8281-ba5cff249edc" />

`CTRL+E` will show you the edit dialog to modify a launcher.

<img width="720" height="600" alt="edit" src="https://github.com/user-attachments/assets/ed7a86b8-685b-49d5-a0e9-3dd0af49c822" />

When **Change directory first** is selected, Launch! extracts the directory from the first command token when the launcher is run.<br/>
E.g. for `C:\TOOLS\APP.EXE` it types `C:`, presses Enter, types `CD C:\TOOLS`, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

### Parameter prompting and help
If a launcher has `Prompt?` activated, when launched from the menu the parameter help dialog will be displayed.<br/>
It shows useful command help information for the selected command. You can then enter the required parameters and choose `Run` to execute the command.

<img width="720" height="600" alt="params" src="https://github.com/user-attachments/assets/ecba02c8-33ce-42f9-b307-1ce94d9388d8" />

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

<img width="720" height="600" alt="explore and run" src="https://github.com/user-attachments/assets/323100a1-e415-41ca-8046-5b7f0f418a48" />

Choosing `/?` for an executable will show the command help (if available) and allow you to provide parameters.

<img width="720" height="600" alt="explore-params" src="https://github.com/user-attachments/assets/5db92f2c-9737-4492-867e-306ac422e3d0" />

You can remove the `Explore & Run` menu item in configuration (`! /CONFIG`)


## Shutdown
The `Shutdown...` menu item is also fixed at the bottom of the menu, and when selected shows a dialog for you to Power Off or Reboot.<br/>
Power Off flushes DOS and SMARTDrive buffers first and then uses power management APM/ACPI calls to power off. That may not work on older machines of course, in which case, the Power off message from Windows 9x will be shown.<br/>

<img width="720" height="600" alt="shutdown" src="https://github.com/user-attachments/assets/361ef781-445d-4aff-9773-426e7bd12a05" />

You can remove the `Shutdown...` menu item in configuration (`! /CONFIG`)

<img width="720" height="600" alt="Power off bitmap" src="https://github.com/user-attachments/assets/2eee9995-eed2-49a5-8f5f-6753d8423140" />

If desired, you can replace this image as `PWROFF.BMP` with any 320x400 256 color bitmap.


## Screensavers
If the menu is open and there has been inactivity for the configured time (1 minute by default), the screen will blank and show a screensaver (by default the Clock).
You can disable the screensaver completely (choose None) or choose from one of the other 6 screensavers by running `! /CONFIG`.<br/>
You can also start the selected screensaver at any time by running `! /NOW`.

Pressing any key or moving the mouse will return to the menu.

The screensavers have been designed to use the EGA 640x350 screen mode, using absolutely minimal resources, and look amazing on a CRT!

### Clock

<img width="720" height="600" alt="screensaver-clock" src="https://github.com/user-attachments/assets/01be0070-cb66-4cdd-96a8-d58dca7e01e5" />

The clock will display in 12 or 24-hour mode depending on your menu time setting, and you can change its color in `! /CONFIG`

### Starry Nite

<img width="720" height="600" alt="Screensaver - Starry Nite" src="https://github.com/user-attachments/assets/c8916e4b-cfe7-4d72-ae96-531c5eca1ab9" />

A random skyline will be generated each time.

### Logo

<img width="720" height="600" alt="Screensaver - DOS" src="https://github.com/user-attachments/assets/508fa7b6-02f0-4c98-b785-47268aba355e" />

The DOS logo will bounce around the screen.

### Warp

<img width="720" height="600" alt="Screensaver - Warp" src="https://github.com/user-attachments/assets/5a21e623-d5ad-4776-a455-67af11d40d62" />

Go at warp speed to absolutely nowhere.

### Pipes

<img width="720" height="600" alt="Screensaver - Pipes" src="https://github.com/user-attachments/assets/e113e7db-1bff-43d2-8a89-4eea13cf0902" />

3-dimensional pipes with a mind of their own.

### Mystify

<img width="720" height="600" alt="Screensaver - Mystify" src="https://github.com/user-attachments/assets/5bc92bfd-1aaa-4625-9a46-1cd1258d1c5c" />

Mystify yourself with the moving polygons.


## SHORTCUT - keyboard shortcut tool
The keyboard shortcut is provided by a separate utility as it is not required to use `!.EXE` on its own. 

If chosen, it will be added to `AUTOEXEC.BAT` by install so the shortcut is available after startup.

Running `! /SETKEY` provides an interactive way to set the shortcut and update your `AUTOEXEC.BAT` automatically.
Or, you can change the keyboard shortcut used by adding the `/KEY=` parameter to `SHORTCUT.COM` with readable names or hexadecimal scan codes, e.g.
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

<img width="720" height="600" alt="autogen_001" src="https://github.com/user-attachments/assets/b8964c31-f9c4-40d3-87d7-a9dfb9458905" />

The menu generator will scan your C:\ for recognized programs in its internal database (over 1000 DOS programs up to 1995).

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
