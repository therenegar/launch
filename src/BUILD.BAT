@ECHO OFF
REM Microsoft C/C++ 7.0: run from an environment prepared by SETENV.BAT
CL /AS /Os /Gs /W3 LAUNCH.C
IF ERRORLEVEL 1 GOTO FAIL
ECHO Built LAUNCH.EXE
GOTO END
:FAIL
ECHO Build failed.
:END
