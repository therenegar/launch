# Launch!

A light-weight customizable and flexible command launcher for all DOS systems.

Download the latest release [here](https://github.com/therenegar/launch/releases/download/v0.6/LAUNCH-0.6.zip)

Requires DOS 3.3+, 80286, EGA or better, 80-column text mode.
Compatible with all command processors including 4DOS/NDOS.

<img width="720" height="576" alt="LAUNCH03" src="https://github.com/user-attachments/assets/c7492f8c-6990-4460-95a3-709f20a3ae0f" />

## Usage

Place `LAUNCH.EXE` anywhere, ideally inside its own directory which is on `PATH`.
To display your menu, just type
```
    LAUNCH
```
> Tip: if you use 4DOS, create an alias `ALIAS .=LAUNCH` and you can have the menu appear by typing `.` at the command prompt. 

Launch! does not execute the selected program itself. It restores the screen, returns to the existing command interpreter, types the configured command and, when selected, supplies Enter. Shell commands, redirection, pipelines, batch files, executable files and deliberately unfinished command lines can therefore all be used. No secondary command processor is started.

The program will automatically create `LAUNCH.CFG` and `LAUNCH.MNU` on first run.
`LAUNCH.CFG` stores configuration settings and `LAUNCH.MNU` contains the menu data. Whenever a change is made to the menu, a `LAUNCH.BAK` file will also be created containing a backup of the menu.

Launch! locates and saves `LAUNCH.MNU` beside `LAUNCH.EXE,` regardless of the current working directory. This works both with a full executable path and when `LAUNCH` is found via `PATH`.

A keyboard shortcut for showing the menu was purposely excluded, otherwise a memory resident TSR would be required, additional command shelling, and unreliability with the menu potentially being executed during other programs (not at the command prompt).

## Keyboard shortcuts

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

<img width="720" height="600" alt="LAUNCH05" src="https://github.com/user-attachments/assets/7eb0edcd-62f4-4c28-b754-88fb7985aad1" />

Run `LAUNCH /CONFIG` to configure menu and dialog colours, menu position, and whether the live clock is shown. Use Left/Right to cycle a focused value; Tab or Up/Down moves between controls. Space advances a value or toggles the clock. Mouse clicks are also supported. 
Settings are saved to `LAUNCH.CFG`.
Cancel leaves the previous appearance unchanged.

If `LAUNCH.CFG` is absent, Launch! uses the original blue colour scheme, opens at the lower-left, and displays the clock. A malformed LAUNCH.CFG is ignored with a warning and the defaults are used.


## Live menu management

<img width="720" height="576" alt="LAUNCH04" src="https://github.com/user-attachments/assets/18e60e33-dd4e-4187-aebb-98ac2ba15053" />

Use the keyboard shortcuts to visually edit the menu while it is open. Changes are written immediately to `LAUNCH.MNU`. 

Each menu panel can display 20 items. Adding a 21st item automatically creates a **More** folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. **More** is maintained by Launch! and is kept at the bottom when the menu is sorted.

When **Change directory first** is selected, Launch! extracts the directory from the first command token. For C:\TOOLS\APP.EXE it types C:, presses Enter, types CD C:\TOOLS, presses Enter, and then types the complete configured command. The final Enter setting applies to that complete command; the preliminary drive and CD commands necessarily receive Enter. Commands without a path do not cause a directory change.

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


## File safety

Launch! validates `LAUNCH.MNU` before opening the menu. A valid file must contain the [Launcher] root section and every non-comment line must be a valid section, FOLDER, or ITEM record. Empty, truncated, malformed, or oversized records are rejected.

Before every accepted Add, Edit, Delete, Move, or Sort operation, the existing valid `LAUNCH.MNU` is copied to `LAUNCH.BAK`. The new menu is first written fully to `LAUNCH.$$$` and is installed only after writing succeeds. `LAUNCH.BK$` is used briefly while rotating the backup.

If LAUNCH.MNU is missing or invalid at startup and `LAUNCH.BAK` is valid, Launch! restores the backup automatically and displays a recovery message. If neither file is usable, the built-in sample menu is installed as both `LAUNCH.MNU` and `LAUNCH.BAK`. An invalid primary file is preserved as `LAUNCH.BAD` when possible.
