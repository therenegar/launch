# Launch!

Version 1.1

A lightweight customizable and flexible command launcher for all DOS systems.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**
> You can download either a .ZIP file or in a 1.44 floppy disk image .IMG

Requires DOS 3.3, 80286, EGA or better. <br/>
Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware. <br/>
Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="menu" src="https://github.com/user-attachments/assets/4e7d0907-a5d6-44a6-9edb-3f3e95f53fa6" />

## Easy Install

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- The necessary files will be copied and your `AUTOEXEC.BAT` will be updated.
- Simply reboot after install and you're good to go.

### Manual Install
If you want to install yourself manually, just copy `!.EXE` and optionally, `SHORTCUT.COM` wherever you like. <br/>
If you want to use the keyboard shortcut, add the following to your `AUTOEXEC.BAT` (with the correct path). You should add the Launch! directory to `PATH` in either case.
```
SET PATH=C:\LAUNCH;%PATH%
C:\LAUNCH\SHORTCUT.COM
```

### Sample menu
On first run `LAUNCH.MNU` will be created based on a sample (unless you're upgrading, and the file already exists). <br/>You'll likely want to delete everything and create your own structure.


## Usage

At the **command prompt**, display your menu by pressing the keyboard shortcut 
```
CTRL + ALT + .
```
> The keyboard shortcut will do nothing while in a program, you must be at the command prompt for the menu to display. This is not a multi-tasking application switcher!

> At this time, the keyboard shortcut tool (SHORTCUT.COM) does **not** work on DOSBox, but works on real hardware and other virtual machines (VMWare, VirtualBox, bochs). 

If you don't have `SHORTCUT.COM` loaded, the keyboard shortcut will not be available.<br/>
To start Launch! without the keyboard shortcut, at the command prompt, simply type 
```
! <ENTER>
```

### How it works
Launch! does not execute the selected program itself. Rather, it returns to the existing command interpreter, types the configured command and, if selected, supplies Enter. This means shell commands, redirection, pipelines, batch files, executable files and deliberately unfinished command lines can all be used. No secondary command processor is started or additional shells. Launch! does not interfere with program execution or return.
This approach provides maximum flexibility, and compatibility. If it can be run from the command prompt, it will work with Launch!.

The program will automatically create `LAUNCH.CFG` and `LAUNCH.MNU` on first run.<br/>
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
<img width="720" height="600" alt="config" src="https://github.com/user-attachments/assets/32a026cc-d16f-4e50-9bfc-941d3af68614" />

Run `! /CONFIG` to configure menu and dialog colours, menu position, and whether the live clock is shown. Use Left/Right to cycle a focused value; Tab or Up/Down moves between controls. Space advances a value or toggles the clock. Mouse clicks are also supported. 
Settings are saved to `LAUNCH.CFG`.
Cancel leaves the previous appearance unchanged.

If `LAUNCH.CFG` is absent, Launch! uses the original blue colour scheme, opens at the lower-left, and displays the clock. A malformed `LAUNCH.CFG` is ignored with a warning and the defaults are used.


## Live menu management
<img width="720" height="600" alt="edit" src="https://github.com/user-attachments/assets/1dbe3bea-ca7f-449f-8581-fd472a03b213" />

Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is maintained by Launch! and is kept at the bottom when the menu is sorted.

When **Change directory first** is selected, Launch! extracts the directory from the first command token. E.g. for C:\TOOLS\APP.EXE it types C:, presses Enter, types CD C:\TOOLS, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

## Manual menu configuration
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


## Keyboard shortcut
The keyboard shortcut is provided by a separate utility to make it completely optional. 
Change the keyboard shortcut used by adding the `/KEY=` parameter with readable names or hexadecimal IBM Set-1 scan codes, e.g.
```
  SHORTCUT /KEY=CTRL+ALT+L
  SHORTCUT /KEY=1D+38+34
```
Tokens are separated by plus signs. CTRL, ALT, SHIFT, PERIOD, DOT, SPACE, letters, digits, common punctuation, and scan codes from 01 through 7F are accepted. <br/>
Common modifier scan codes are:
- 1D (Ctrl)
- 38 (Alt)
- 2A or 36 (Shift)

Use `SHORTCUT /?` for more information.
The shortcut utility can also be removed from memory with `SHORTCUT /UNLOAD`.

<img width="720" height="600" alt="shortcut-help" src="https://github.com/user-attachments/assets/6a6fc0fd-8155-4457-b452-bee9b2de2269" />


## File safety

Launch! validates `LAUNCH.MNU` before opening the menu. A valid file must contain the [Launcher] root section and every non-comment line must be a valid section, FOLDER, or ITEM record. Empty, truncated, malformed, or oversized records are rejected.

Before every accepted Add, Edit, Delete, Move, or Sort operation, the existing valid `LAUNCH.MNU` is copied to `LAUNCH.BAK`. The new menu is first written fully to `LAUNCH.$$$` and is installed only after writing succeeds. `LAUNCH.BK$` is used briefly while rotating the backup.

If `LAUNCH.MNU` is missing or invalid at startup and `LAUNCH.BAK` is valid, Launch! restores the backup automatically and displays a recovery message. If neither file is usable, the built-in sample menu is installed as both `LAUNCH.MNU` and `LAUNCH.BAK`. An invalid primary file is preserved as `LAUNCH.BAD` when possible.

