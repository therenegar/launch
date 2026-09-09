# Launch!

A lightweight launcher for any DOS, with huge features to improve the usability of the command prompt - and make it more convenient and friendly.

**Download the latest release [here](https://github.com/therenegar/launch/releases/latest)**
> You can download as either a .ZIP file or a 1.44MB floppy disk image .IMG

Requires DOS 3.3, 80286, EGA or better. <br/>
Tested with MS-DOS, PC DOS, DR-DOS and FreeDOS on real hardware and virtual machines, including DOSBox.<br/>
Compatible with third-party command interpreters such as 4DOS/NDOS.

<img width="720" height="600" alt="menu" src="https://github.com/user-attachments/assets/433ec062-e5e8-4d0b-a764-8263ce5c25ca" />


## Key features
- Displays a hierarchical folder based menu, modally over the top of the existing console contents.
- Supports trigger by keyboard shortcut which can be customized.
- Launches commands at the existing command prompt. It doesn't interfere with program execution or return meaning maximum compatibility and flexibility.
- Easy visual menu editing.
- Automatic menu generator with comprehensive DOS program database to automatically identify programs and create a menu
- Program parameter help screen to help with supplying parameters to any menu item.
- Built in executable explorer to quickly browse and run programs anywhere.
- Built in Shutdown/Reboot control with retro Windows 95 power off experience.
- Maximum compatibility across DOS versions on real or emulated hardware.
- No libraries or dependencies including ANSI.


## Install it

Extract the release zip file or mount the floppy image:

- Run `INSTALL.EXE`
- You'll be prompted for a directory to place Launch!
- The necessary files will be copied and your `AUTOEXEC.BAT` will be updated.
- You can scan your drive and automatically build an initial menu from a comprehensive database of over 1000 DOS programs.
- Simply reboot after install and you're good to go.

<img width="720" height="600" alt="install" src="https://github.com/user-attachments/assets/0ef73b1f-b2f2-4a3c-929c-986aba861126" />


> DOSBox installs will be detected by the installer, and a different DOSBox compatible version of the shortcut key tool will be installed.


## Usage

At the **command prompt**, display your menu by pressing the keyboard shortcut which by default is set to:
```
CTRL + ALT + .
```
> The keyboard shortcut will do nothing while in a program, you must be at the command prompt for the menu to display. This is not a multi-tasking application switcher!

If you don't have `SHORTCUT.COM` loaded, the keyboard shortcut will not be available.<br/>
To start Launch! without the keyboard shortcut, at the command prompt, simply type:
```
! <ENTER>
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

- Left click opens a folder or runs a launcher; left click outside all visible menu panels closes Launch!.
- Right click an item opens its Edit dialog. 


## Customizing appearance
<img width="720" height="600" alt="config" src="https://github.com/user-attachments/assets/fa6635ea-0740-48a6-8735-3f47a0f7d539" />

Run `! /CONFIG` to configure menu and dialog colors, menu position, and whether options such as Explore & Run, Shutdown, and the live clock are displayed.
Use Left/Right to cycle a focused value; Tab or Up/Down moves between controls. Space advances a value or toggles checkbox items. Mouse clicks are also supported. 
Settings are saved to `LAUNCH.CFG`.
Cancel leaves the previous appearance unchanged.

If `LAUNCH.CFG` is absent, Launch! uses the original blue color scheme, opens at the lower-left, and displays the clock. A malformed `LAUNCH.CFG` is ignored with a warning and the defaults are used.


## Live menu management
Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 
Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is maintained by Launch! and is kept at the bottom when the menu is sorted.

`CTRL+A` allows you to add a new folder, launcher, or separator to the currently visible menu panel.

<img width="720" height="600" alt="add" src="https://github.com/user-attachments/assets/be2f5567-086a-4337-909e-42f22fa8853a" />

`CTRL+E` will show you the edit dialog to modify a launcher.

<img width="720" height="600" alt="edit" src="https://github.com/user-attachments/assets/dda87955-ad54-4583-b073-996eba095d9d" />


When **Change directory first** is selected, Launch! extracts the directory from the first command token. <br/>
E.g. for `C:\TOOLS\APP.EXE` it types `C:`, presses Enter, types `CD C:\TOOLS`, presses Enter, and then types the complete configured command. The Enter setting applies to that final complete command; the preliminary drive and CD commands must receive Enter. <br/>Commands without a path do not cause a directory change.

### Parameter prompting and help
If a launcher has `Prompt?` activated, when launched from the menu the parameter help dialog will be displayed.
It shows useful command help information for the selected command. You can then enter the required parameters and choose `Run` to execute the command.

<img width="720" height="600" alt="param" src="https://github.com/user-attachments/assets/9a8f4694-814e-400a-a6d1-037c8c1a4265" />

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


## Explore & Run
The `Explore & Run` menu it is a fixed item at the bottom of the menu.<br/>
When selected you will be able to browse the file-system for executable programs and quickly run them, exactly as launchers are run from the menu.<br/>
Enter will open the selected directory, or run the selected executable. You can also double-click entries with the mouse for the same effect.

<img width="720" height="600" alt="explore" src="https://github.com/user-attachments/assets/591ce9e5-f5d7-4d6f-bb0b-b98772545766" />

Choosing `/?` for an executable will show the command help (if available) and allow you to provide parameters.

<img width="720" height="600" alt="exechelp" src="https://github.com/user-attachments/assets/7fd4f354-3022-4e90-9283-b6c5a8bd225b" />

You can remove the `Explore & Run` menu item in configuration (`! /CONFIG`)


## Shutdown/Reboot
The `Shutdown...` menu item is also fixed at the bottom of the menu, and when selected shows a dialog for you to Shutdown or Reboot.<br/>
Shutdown flushes DOS and SMARTDrive buffers first and then uses standard ACPI calls to power off. That may not work on older machines of course, in which case, the Power off message from Windows 9x will be shown.<br/>

<img width="720" height="600" alt="power" src="https://github.com/user-attachments/assets/b4743d26-5bbe-4d44-b95d-dfeef85dcafe" />

You can remove the `Shutdown...` menu item in configuration (`! /CONFIG`)

<img width="720" height="600" alt="poweroff" src="https://github.com/user-attachments/assets/2eee9995-eed2-49a5-8f5f-6753d8423140" />


## SHORTCUT - keyboard shortcut tool
The keyboard shortcut is provided by a separate utility as it is not required to use `!.EXE` on its own. 
It will be added to `AUTOEXEC.BAT` by install so the shortcut is available after startup.
You can change the keyboard shortcut used by adding the `/KEY=` parameter with readable names or hexadecimal scan codes, examples:
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
<img width="720" height="600" alt="autogen" src="https://github.com/user-attachments/assets/0430d091-5e71-4613-82a5-545172a2a74a" />

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

