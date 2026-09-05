# Launch!
A simple light-weight launcher menu for all DOS systems.

Download the latest release [here](https://github.com/therenegar/launch/releases/download/v0.41/LAUNCH-0.41.zip).

<img width="720" height="600" alt="LAUNCH06" src="https://github.com/user-attachments/assets/ff39dd14-5a45-499f-9acc-7d2b74b4e558" />
<img width="720" height="600" alt="LAUNCH08" src="https://github.com/user-attachments/assets/2fa021bd-05cc-4bfb-952b-f3ac0f77d458" />
<img width="720" height="600" alt="LAUNCH05" src="https://github.com/user-attachments/assets/0cd89198-1ad4-47d5-9bdc-2bf4415284f9" />

SYS REQUIREMENTS
----------------
Target: DOS 3.3+, 80286, EGA or better, 80-column text mode
Compiler: Microsoft C 5.1

INSTALLATION
------------
Keep `LAUNCH.EXE` and `LAUNCH.MNU` in the same directory and put that directory on PATH. 
Type 
```
LAUNCH
```
at the DOS prompt to open the menu.

Launch! locates and saves `LAUNCH.MNU` beside `LAUNCH.EXE`, regardless of the current working directory.

CONFIGURATION
-------------
You can edit the `LAUNCH.MNU` file in a text editor to manage the menu and items, or use shortcut keys (described below) to edit the menu within the program.
Sections represent menu paths. Separate nesting levels with a backslash:
```
  [Launcher\Internet]
  FOLDER=Communications
  ITEM=Telnet|C:\MTCP\TELNET.EXE

  [Launcher\Internet\Communications]
  ITEM=Pine|C:\COMM\PINE.EXE
```

KEYS
----
- Global
  - **Up/Down**       - Select an entry
  - **Right**         - Open a selected folder
  - **Enter**         - Open a folder or launch an item
  - **Left**          - Close the current folder
  - **Esc**           - Close the menu and exit the program
  
- On the currently displayed menu
  - **Ctrl+A**        - Add a folder or launcher
  - **Ctrl+D**        - Delete the selected item
  - **Ctrl+E**        - Edit the selected item
  - **Ctrl+Up/Down**  - Move the selected item up or down
  - **Ctrl+S**        - Sort the menu alphabetically

MOUSE
-----
When an INT 33h mouse driver is present, Launch! shows a pointer for mouse interaction. 
- Left click opens a folder or runs a launcher; or outside the menu closes Launch!. 
- Right clicking an item opens its Edit dialog. 

POSITION
--------
The default position for the menu is the lower-left corner. Use `LAUNCH /POS=TOP` for the upper-left corner.

MENU MANAGEMENT
---------------
- Accepted changes are written immediately to `LAUNCH.MNU`. 
- Removing a folder also removes every item and subfolder beneath it.
- Folder names cannot contain backslash, square brackets, or the pipe character, because those characters delimit the text configuration hierarchy.
- Each menu panel can display 20 items. Adding a 21st item automatically creates a More folder at the bottom and moves the overflow into it. Further overflow is handled the same way, up to the four-level menu limit. More is maintained by Launch! and is kept at the bottom when the menu is sorted.

TECH DETAILS
------------
EXE and COM launchers are started directly with DOS process overlay. Launch! therefore leaves memory completely and the original command processor regains control when the selected program exits. This also avoids starting a secondary command interpreter.
