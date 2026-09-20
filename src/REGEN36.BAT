@ECHO OFF
REM Maintainer-only regeneration of Release 3.61 intermediate sources.
REM Normal BUILD.BAT does not require this step.
CL /AS /Od /Gs /W2 /FeGEN36.EXE GEN36.C
IF ERRORLEVEL 1 GOTO FAIL
GEN36.EXE
IF ERRORLEVEL 1 GOTO FAIL
ECHO Release 3.61 generated sources refreshed.
GOTO END
:FAIL
ECHO Regeneration failed.
:END
