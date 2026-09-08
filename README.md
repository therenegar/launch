# Launch!

Version 1.5

A lightweight customizable and flexible command launcher for all DOS systems.
Includes automatic menu generator, and keyboard shortcut launching.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**
> You can download either a .ZIP file or in a 1.44 floppy disk image .IMG

Requires DOS 3.3, 80286, EGA or better. <br/>
Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual machines, including DOSBox.<br/>
Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="LAUNCH!" src="https://github.com/user-attachments/assets/e8dd3dd3-8540-40a8-a500-656aae59b6e6" />

## Easy Install

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- The necessary files will be copied and your `AUTOEXEC.BAT` will be updated.
- You can scan your drive and automatically build an initial menu from a comprehensive database of over 1000 DOS programs.
- Simply reboot after install and you're good to go.

<img width="720" height="600" alt="install_001" src="https://github.com/user-attachments/assets/23ddd438-a744-4ec5-9bea-ddb1f42e30bc" />

### DOSBox
DOSBox installs will be detected by the installer, and a different DOSBox compatible version of the shortcut key tool will be installed.


## Usage

At the **command prompt**, display your menu by pressing the keyboard shortcut 
```
CTRL + ALT + .
```
> The keyboard shortcut will do nothing while in a program, you must be at the command prompt for the menu to display. This is not a multi-tasking application switcher!

If you don't have `SHORTCUT.COM` loaded, the keyboard shortcut will not be available.<br/>
To start Launch! without the keyboard shortcut, at the command prompt, simply type 
```
! <ENTER>
```

### How it works
Launch! does not execute the selected program itself. Rather, it returns to the existing command interpreter, types the configured command and, if selected, supplies Enter. This means shell commands, redirection, pipelines, batch files, executable files and deliberately unfinished command lines can all be used. No secondary command processor is started or additional shells. Launch! does not interfere with program execution or return.
This approach provides maximum flexibility, and compatibility. If it can be run from the command prompt, it will work with Launch!.

`LAUNCH.CFG` stores configuration settings and `LAUNCH.MNU` contains the menu data. Whenever a change is made to the menu, a `LAUNCH.BAK` file will also be created containing a backup of the menu.
Launch! locates and saves `LAUNCH.MNU` beside `!.EXE,` regardless of the current working directory. This works both with a full executable path and when `!.EXE` is found via `PATH`.

## Keyboard usage

General navigation
- Up/Down       - Select an entry
- Right         - Open a selected folder
- Left          - Close the current folder
- Enter         - Open a folder or send a launcher command to the prompt
- Esc           - Close the complete menu

Menu management
- Ctrl+A        - Add a folder or launcher to the open menu
- Ctrl+D        - Delete the selected item after confirmation
- Ctrl+E        - Edit the selected folder or launcher
- Ctrl+Up/Down  - Move the selected item within its menu
- Ctrl+S        - Sort the open menu alphabetically

## Mouse usage

- Left click opens a folder or runs a launcher; left click outside all visible menu panels closes Launch!.
- Right click an item opens its Edit dialog. 

## Customizing appearance
<img width="720" height="600" alt="!_002" src="https://github.com/user-attachments/assets/7994e3e9-4cc5-48e0-899c-9ed1c6ce794a" />

Run `! /CONFIG` to configure menu and dialog colours, menu position, and whether the Shutdown/Reboot option and live clock are shown. 
Use Left/Right to cycle a focused value; Tab or Up/Down moves between controls. Space advances a value or toggles the clock. Mouse clicks are also supported. 
Settings are saved to `LAUNCH.CFG`.
Cancel leaves the previous appearance unchanged.

If `LAUNCH.CFG` is absent, Launch! uses the original blue colour scheme, opens at the lower-left, and displays the clock. A malformed `LAUNCH.CFG` is ignored with a warning and the defaults are used.

## Live menu management
Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

`CTRL+A` allows you to add a new folder, launcher, or separator to the currently visible menu panel.

<img width="720" height="600" alt="!_003" src="https://github.com/user-attachments/assets/92e5ede8-7d6a-4f53-90ee-9b58ac665814" />

`CTRL+E` will show you the edit dialog.

<img width="720" height="600" alt="!_004" src="https://github.com/user-attachments/assets/b0b525ca-0fad-43c1-ad39-195cbb7eb3c8" />

When **Change directory first** is selected, Launch! extracts the directory from the first command token. E.g. for C:\TOOLS\APP.EXE it types C:, presses Enter, types CD C:\TOOLS, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is maintained by Launch! and is kept at the bottom when the menu is sorted.

### Manual menu configuration
`LAUNCH.MNU` is a plain text file that you can edit yourself with any text editor.<br/>
Within the `LAUNCH.MNU` file, sections represent menu paths. Separate nesting levels with a backslash:
```
  [Launcher\Internet]
  FOLDER=Communications
  ITEM=Telnet|C:\MTCP\TELNET.EXE|1|0

  [Launcher\Internet\Communications]
  ITEM=Pine|C:\COMM\PINE.EXE|1|1
```
An ITEM record has this form:
```
  ITEM=title|command and parameters|press Enter|change directory
```
The last two values are 1 for selected and 0 for clear. Existing records that do not contain them remain compatible and default to 1|0.

## Shutdown/Reboot
<img width="720" height="600" alt="!_001" src="https://github.com/user-attachments/assets/c7ed73a2-1578-4c13-9c44-a6bb9af35ee4" />

The `Shutdown/Reboot` menu item is fixed, always at the bottom, and when selected shows a dialog for you to Shutdown or Reboot.
Shutdown uses standard ACPI power off calls, which may not be supported on older machines.
You can remove the Shutdown/Reboot menu item in configuration.

## SHORTCUT - keyboard shortcut tool
The keyboard shortcut is provided by a separate utility to make it completely optional. <br/>
Change the keyboard shortcut used by adding the `/KEY=` parameter with readable names or hexadecimal IBM Set-1 scan codes, e.g.
```
  SHORTCUT /KEY=CTRL+ALT+L
  SHORTCUT /KEY=1D+38+34
```
Tokens are separated by `+`. CTRL, ALT, SHIFT, PERIOD, DOT, SPACE, F1-F12, the Windows key, letters, digits, common punctuation are accepted. You can also use scan codes.<br/>

The shortcut utility can be removed from memory with `SHORTCUT /UNLOAD`.
Use `SHORTCUT /?` for more information.

## AUTOGEN - an automatic menu generator
<img width="720" height="600" alt="autogen_001" src="https://github.com/user-attachments/assets/a03df183-75bf-48bd-b84e-772d1a1d4d1d" />

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

