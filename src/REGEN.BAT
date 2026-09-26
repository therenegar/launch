REM     __                           __    __
REM    / /   ____ ___  ______  _____/ /_  / /
REM   / /   / __ `/ / / / __ \/ ___/ __ \/ / 
REM  / /___/ /_/ / /_/ / / / /__/ / / /_/  
REM /_____/\__,_/\__,_/_/ /_/\___/_/ /_(_)   
REM Launch! for DOS ---------------------
REM MAINTAINER NOTES - Launch! 3.73
REM File: REGEN.BAT
REM Role: Maintainer regeneration script
REM Build/ownership: Builds/runs GENBUILD.EXE to refresh generated *BLD.C sources.
REM Maintainer contract: Not required for ordinary builds; use only when canonical/build transformations need regeneration.
REM Documentation note: comments describe intent and invariants; behavior remains defined by the code and Release requirements.
REM DOS constraints: code targets 16-bit DOS/MS C 7-era models. Watch DGROUP (<64K in small model), stack use, far/near pointers, BIOS/DOS reentrancy and text-mode screen restoration.
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
