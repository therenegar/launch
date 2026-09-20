@ECHO OFF
REM Launch! 3.86 8086 POC build - Microsoft C/C++ 7.0 + GNU binutils
REM /G0 selects 8086/8088 instruction generation.
CL /AS /G0 /Os /Gs /W3 /Fe!86.EXE LAUNCH86.C
IF ERRORLEVEL 1 GOTO FAIL
ECHO Built !86.EXE
ECHO SHORTC86.COM is assembled on the host with GNU as/ld (see SHORTC86.ASM).
GOTO END
:FAIL
ECHO !86 build failed.
:END
