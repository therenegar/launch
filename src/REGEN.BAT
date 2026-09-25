@ECHO OFF
REM Maintainer-only regeneration of build intermediate sources.
REM Normal BUILD.BAT does not require this step.
CL /AS /Od /Gs /W2 /FeGENBUILD.EXE GENBUILD.C
IF ERRORLEVEL 1 GOTO FAIL
GENBUILD.EXE
IF ERRORLEVEL 1 GOTO FAIL
ECHO Release 3.71 generated sources refreshed.
GOTO END
:FAIL
ECHO Regeneration failed.
:END
