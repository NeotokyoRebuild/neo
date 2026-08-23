@echo off
rem
rem Runs the map compile tools (vbsp, vvis, vrad) over every combination of
rem
rem   * where the tool binaries are run from:
rem       build-tree      straight out of the CMake build tree. Windows has no rpath,
rem                       so PATH is pointed at the engine binaries and at the tool
rem                       libraries, which sit in their own directories there.
rem       engine-bin      from an engine's bin\x64 directory, which is how the
rem                       NEO_INSTALL_UTILS package is meant to be unpacked. A hardlink
rem                       mirror of the engine directory is used so that nothing is
rem                       written into the Steam installation, which means the work
rem                       directory has to be on the same drive as the engine.
rem       library-path    from an unrelated directory with PATH pointing at the engine
rem                       binaries
rem
rem   * what kind of game directory they are pointed at with -game:
rem       sourcemod       steamapps\sourcemods\neo, no engine next to it, so the engine
rem                       is resolved from the SteamAppId in gameinfo.txt
rem       source-tree     game\neo in this repository, same engine lookup as above
rem       ntre            the Steam "NeotokyoRebuild" installation, which has the engine
rem                       binaries sitting right next to the game directory
rem       no-engine       negative test: a gameinfo.txt naming an appid that isn't
rem                       installed, which every tool has to reject
rem
rem The games are looked for in Steam's default library only. If yours live in another
rem library folder, point at them with --sdk-dir and --ntre-dir.
rem
rem See the "Map compile tools" section of README.md for what these layouts are about.
rem
rem Usage: utils\test_tools.bat [options]
rem Run with --help for the option list.

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "SRC_DIR=%%~fI"
for %%I in ("%SRC_DIR%\..") do set "REPO_DIR=%%~fI"

set "PLATSUBDIR=x64"

rem Nothing is ever installed under this one
set "BOGUS_APPID=99999999"

set "ALL_LOCATIONS=build-tree engine-bin library-path"
set "ALL_GAME_TYPES=sourcemod source-tree ntre no-engine"
set "ALL_TOOLS=vbsp vvis vrad"

set "BUILD_DIR="
set "MAP_FILE="
set "STEAM_DIR="
set "SDK_DIR="
set "NTRE_DIR="
set "SOURCEMOD_DIR="
set "SOURCE_TREE_GAME_DIR=%REPO_DIR%\game\neo"
set "WORK_DIR="
set "KEEP_WORK_DIR=0"
set "LIST_ONLY=0"

set "LOCATIONS=,build-tree,engine-bin,library-path,"
set "GAME_TYPES=,sourcemod,source-tree,ntre,no-engine,"
set "TOOLS=,vbsp,vvis,vrad,"

rem Only devbox.vmf is known to reference an HL2 skybox, which is what proves that the
rem engine content actually got mounted. Any other map only gets the weaker checks.
set "CHECK_ENGINE_CONTENT=1"
set "ENGINE_CONTENT_MARKER=sky_day01_01"

set "PASSED=0"
set "FAILED=0"
set "SKIPPED=0"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--build-dir"     ( set "BUILD_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--map"           ( set "MAP_FILE=%~2" & set "CHECK_ENGINE_CONTENT=0" & shift & shift & goto parse_args )
if /i "%~1"=="--steam-dir"     ( set "STEAM_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--sdk-dir"       ( set "SDK_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--ntre-dir"      ( set "NTRE_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--sourcemod-dir" ( set "SOURCEMOD_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--game-dir"      ( set "SOURCE_TREE_GAME_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--work-dir"      ( set "WORK_DIR=%~2" & shift & shift & goto parse_args )
if /i "%~1"=="--locations"     ( set "LOCATIONS=,%~2," & shift & shift & goto parse_args )
if /i "%~1"=="--game-types"    ( set "GAME_TYPES=,%~2," & shift & shift & goto parse_args )
if /i "%~1"=="--tools"         ( set "TOOLS=,%~2," & shift & shift & goto parse_args )
if /i "%~1"=="--keep"          ( set "KEEP_WORK_DIR=1" & shift & goto parse_args )
if /i "%~1"=="--list"          ( set "LIST_ONLY=1" & shift & goto parse_args )
if /i "%~1"=="--help"          goto usage
if /i "%~1"=="-h"              goto usage
echo error: unknown option '%~1'
goto usage

:usage
echo Usage: %~nx0 [options]
echo.
echo   --build-dir DIR      CMake build directory holding the tools
echo                        ^(default: src\build\windows-release^)
echo   --map FILE           .vmf to compile ^(default: game\neo\mapsrc\devbox.vmf^)
echo   --steam-dir DIR      Steam installation to look the games up in ^(autodetected^)
echo   --sdk-dir DIR        "Source SDK Base 2013 Multiplayer" install ^(autodetected^)
echo   --ntre-dir DIR       "NeotokyoRebuild" installation ^(autodetected^)
echo   --sourcemod-dir DIR  steamapps\sourcemods\neo ^(autodetected, simulated if missing^)
echo   --game-dir DIR       source tree game directory
echo   --work-dir DIR       scratch directory to use ^(default: a new one under %%TEMP%%^)
echo   --locations LIST     comma separated subset of: %ALL_LOCATIONS%
echo   --game-types LIST    comma separated subset of: %ALL_GAME_TYPES%
echo   --tools LIST         comma separated subset of: %ALL_TOOLS%
echo   --keep               keep the work directory ^(logs and compiled maps^) around
echo   --list               only print what was detected, then exit
echo   -h, --help           this text
exit /b 1

:args_done

rem ---------------------------------------------------------------------------- rem
rem Locating things
rem ---------------------------------------------------------------------------- rem

if not defined BUILD_DIR set "BUILD_DIR=%SRC_DIR%\build\windows-release"
for %%I in ("%BUILD_DIR%") do set "BUILD_DIR=%%~fI"

for %%F in (
    "%BUILD_DIR%\utils\vbsp\vbsp.exe"
    "%BUILD_DIR%\utils\vvis_launcher\vvis.exe"
    "%BUILD_DIR%\utils\vrad_launcher\vrad.exe"
    "%BUILD_DIR%\utils\vvis\vvis_library.dll"
    "%BUILD_DIR%\utils\vrad\vrad_library.dll"
) do (
    if not exist %%F (
        echo error: %%~F is missing, build the utils first or pass --build-dir
        exit /b 1
    )
)

if not defined STEAM_DIR call :detect_steam_dir
if defined STEAM_DIR (
    if not defined SDK_DIR (
        if exist "%STEAM_DIR%\steamapps\common\Source SDK Base 2013 Multiplayer\bin\%PLATSUBDIR%\filesystem_stdio.dll" (
            set "SDK_DIR=%STEAM_DIR%\steamapps\common\Source SDK Base 2013 Multiplayer"
        )
    )
    if not defined NTRE_DIR (
        if exist "%STEAM_DIR%\steamapps\common\NeotokyoRebuild\bin\%PLATSUBDIR%\filesystem_stdio.dll" (
            set "NTRE_DIR=%STEAM_DIR%\steamapps\common\NeotokyoRebuild"
        )
    )
    if not defined SOURCEMOD_DIR (
        if exist "%STEAM_DIR%\steamapps\sourcemods\neo\gameinfo.txt" (
            set "SOURCEMOD_DIR=%STEAM_DIR%\steamapps\sourcemods\neo"
        )
    )
)

if not defined MAP_FILE set "MAP_FILE=%SOURCE_TREE_GAME_DIR%\mapsrc\devbox.vmf"
if not exist "%MAP_FILE%" (
    echo error: map '%MAP_FILE%' not found, pass --map
    exit /b 1
)
for %%I in ("%MAP_FILE%") do set "MAP_NAME=%%~nI"

echo source tree:      %REPO_DIR%
echo build directory:  %BUILD_DIR%
echo map:              %MAP_FILE%
if defined STEAM_DIR     (echo Steam:            %STEAM_DIR%)     else (echo Steam:            ^<not found^>)
if defined SDK_DIR       (echo SDK Base 2013 MP: %SDK_DIR%)       else (echo SDK Base 2013 MP: ^<not found, pass --sdk-dir^>)
if defined NTRE_DIR      (echo NeotokyoRebuild:  %NTRE_DIR%)      else (echo NeotokyoRebuild:  ^<not found, pass --ntre-dir^>)
if defined SOURCEMOD_DIR (echo sourcemods\neo:   %SOURCEMOD_DIR%) else (echo sourcemods\neo:   ^<not installed, will be simulated^>)
echo.

if "%LIST_ONLY%"=="1" exit /b 0

rem ---------------------------------------------------------------------------- rem
rem Work directory
rem ---------------------------------------------------------------------------- rem

if not defined WORK_DIR set "WORK_DIR=%TEMP%\neo-test-tools-%RANDOM%%RANDOM%"
if not exist "%WORK_DIR%" mkdir "%WORK_DIR%" 2>nul
if not exist "%WORK_DIR%" (
    echo error: can't create work directory '%WORK_DIR%'
    exit /b 1
)
for %%I in ("%WORK_DIR%") do set "WORK_DIR=%%~fI"

rem The tools run the whole map path through Q_DefaultExtension, so a dot anywhere in
rem it makes them skip appending .vmf
set "WORK_DIR_TAIL=%WORK_DIR:~2%"
if not "%WORK_DIR_TAIL:.=%"=="%WORK_DIR_TAIL%" (
    echo error: work directory '%WORK_DIR%' contains a '.', which the tools can't handle
    echo        in a map path. Pass --work-dir with a dot free path.
    exit /b 1
)

set "RESULTS_FILE=%WORK_DIR%\results.txt"
type nul > "%RESULTS_FILE%"

echo work directory:   %WORK_DIR%
echo.

rem ---------------------------------------------------------------------------- rem
rem Game directories
rem ---------------------------------------------------------------------------- rem

set "GAME_DIR_sourcemod="
set "GAME_DIR_source_tree="
set "GAME_DIR_ntre="
set "GAME_DIR_no_engine="
set "SKIP_sourcemod="
set "SKIP_source_tree="
set "SKIP_ntre="
set "SKIP_no_engine="
set "ENGINE_sourcemod="
set "ENGINE_source_tree="
set "ENGINE_ntre="
set "ENGINE_no_engine="

if defined SOURCEMOD_DIR (
    set "GAME_DIR_sourcemod=%SOURCEMOD_DIR%"
) else (
    if exist "%SOURCE_TREE_GAME_DIR%\gameinfo.txt" (
        rem Same layout as steamapps\sourcemods\neo: a mod directory with no engine
        rem next to it, which is all the engine lookup cares about
        mkdir "%WORK_DIR%\sourcemods" 2>nul
        mklink /J "%WORK_DIR%\sourcemods\neo" "%SOURCE_TREE_GAME_DIR%" >nul 2>&1
        if exist "%WORK_DIR%\sourcemods\neo\gameinfo.txt" (
            set "GAME_DIR_sourcemod=%WORK_DIR%\sourcemods\neo"
            echo note: simulating steamapps\sourcemods\neo at %WORK_DIR%\sourcemods\neo
        ) else (
            set "SKIP_sourcemod=could not create a junction to the source tree game directory"
        )
    ) else (
        set "SKIP_sourcemod=no sourcemods\neo and no source tree game directory"
    )
)

if exist "%SOURCE_TREE_GAME_DIR%\gameinfo.txt" (
    set "GAME_DIR_source_tree=%SOURCE_TREE_GAME_DIR%"
) else (
    set "SKIP_source_tree=%SOURCE_TREE_GAME_DIR% does not exist"
)

if defined NTRE_DIR (
    set "GAME_DIR_ntre=%NTRE_DIR%\neo"
    set "ENGINE_ntre=%NTRE_DIR%\bin\%PLATSUBDIR%"
) else (
    set "SKIP_ntre=NeotokyoRebuild was not found"
)

rem A valid mod whose engine simply isn't installed. The tools have to bail out on the
rem engine lookup, before they ever look at any content.
mkdir "%WORK_DIR%\no-engine\neo" 2>nul
> "%WORK_DIR%\no-engine\neo\gameinfo.txt" (
    echo "GameInfo"
    echo {
    echo 	game		"engine lookup test"
    echo 	FileSystem
    echo 	{
    echo 		SteamAppId	%BOGUS_APPID%
    echo 		SearchPaths
    echo 		{
    echo 			Game	.
    echo 		}
    echo 	}
    echo }
)
set "GAME_DIR_no_engine=%WORK_DIR%\no-engine\neo"

rem The engine each game type ends up running on, which is also the one the tool
rem binaries have to be able to load their libraries from
if defined SDK_DIR (
    set "ENGINE_sourcemod=%SDK_DIR%\bin\%PLATSUBDIR%"
    set "ENGINE_source_tree=%SDK_DIR%\bin\%PLATSUBDIR%"
    set "ENGINE_no_engine=%SDK_DIR%\bin\%PLATSUBDIR%"
) else (
    set "GAME_DIR_sourcemod="
    set "GAME_DIR_source_tree="
    set "GAME_DIR_no_engine="
    set "SKIP_sourcemod=Source SDK Base 2013 Multiplayer was not found"
    set "SKIP_source_tree=Source SDK Base 2013 Multiplayer was not found"
    set "SKIP_no_engine=Source SDK Base 2013 Multiplayer was not found"
)

rem ---------------------------------------------------------------------------- rem
rem Running
rem ---------------------------------------------------------------------------- rem

for %%L in (%ALL_LOCATIONS%) do (
    if not "!LOCATIONS:,%%L,=!"=="!LOCATIONS!" (
        for %%G in (%ALL_GAME_TYPES%) do (
            if not "!GAME_TYPES:,%%G,=!"=="!GAME_TYPES!" (
                call :run_case "%%L" "%%G"
            )
        )
    )
)

rem ---------------------------------------------------------------------------- rem
rem Summary
rem ---------------------------------------------------------------------------- rem

echo.
echo Summary
echo -------
sort "%RESULTS_FILE%"
echo.
echo %PASSED% passed, %FAILED% failed, %SKIPPED% skipped

if "%KEEP_WORK_DIR%"=="1" (
    echo work directory kept at %WORK_DIR%
) else (
    rd /s /q "%WORK_DIR%" 2>nul
)

if %FAILED% GTR 0 exit /b 1
exit /b 0

rem ============================================================================ rem
rem Subroutines
rem ============================================================================ rem

:detect_steam_dir
for /f "tokens=2,*" %%A in ('reg query "HKCU\Software\Valve\Steam" /v SteamPath 2^>nul ^| findstr /i "SteamPath"') do set "STEAM_DIR=%%B"
if defined STEAM_DIR set "STEAM_DIR=%STEAM_DIR:/=\%"
if defined STEAM_DIR if not exist "%STEAM_DIR%\steamapps" set "STEAM_DIR="
exit /b 0

rem %1 = destination directory
:stage_tools
if not exist "%~1" mkdir "%~1" 2>nul
copy /y "%BUILD_DIR%\utils\vbsp\vbsp.exe"          "%~1\" >nul || exit /b 1
copy /y "%BUILD_DIR%\utils\vvis_launcher\vvis.exe" "%~1\" >nul || exit /b 1
copy /y "%BUILD_DIR%\utils\vrad_launcher\vrad.exe" "%~1\" >nul || exit /b 1
copy /y "%BUILD_DIR%\utils\vvis\vvis_library.dll"  "%~1\" >nul || exit /b 1
copy /y "%BUILD_DIR%\utils\vrad\vrad_library.dll"  "%~1\" >nul || exit /b 1
exit /b 0

rem Hardlink farm standing in for "the utils package was unpacked over an engine
rem installation", without touching the installation itself
rem %1 = engine bin directory, %2 = destination directory
:mirror_engine_bin
if not exist "%~2" mkdir "%~2" 2>nul
if not exist "%~2" exit /b 1
for %%F in ("%~1\*") do (
    if not exist "%~2\%%~nxF" mklink /H "%~2\%%~nxF" "%%~fF" >nul 2>&1
    if not exist "%~2\%%~nxF" exit /b 1
)
call :stage_tools "%~2"
exit /b %ERRORLEVEL%

rem %1 = location, %2 = engine bin directory, %3 = short engine name
rem Sets _TOOL_DIR (empty means "run from the build tree") and _EXTRA_PATH
:prepare_location
set "_TOOL_DIR="
set "_EXTRA_PATH="

if /i "%~1"=="build-tree" (
    rem Windows has no rpath, so the tool libraries and the engine have to be on PATH
    set "_EXTRA_PATH=%~2;%BUILD_DIR%\utils\vvis;%BUILD_DIR%\utils\vrad"
    exit /b 0
)

if /i "%~1"=="library-path" (
    set "_TOOL_DIR=%WORK_DIR%\staged-tools"
    set "_EXTRA_PATH=%~2"
    if not exist "%WORK_DIR%\staged-tools\vbsp.exe" call :stage_tools "%WORK_DIR%\staged-tools"
    if not exist "%WORK_DIR%\staged-tools\vbsp.exe" exit /b 1
    exit /b 0
)

if /i "%~1"=="engine-bin" (
    set "_TOOL_DIR=%WORK_DIR%\engine-mirror-%~3"
    if not exist "%WORK_DIR%\engine-mirror-%~3\vbsp.exe" call :mirror_engine_bin "%~2" "%WORK_DIR%\engine-mirror-%~3"
    if not exist "%WORK_DIR%\engine-mirror-%~3\vbsp.exe" exit /b 1
    exit /b 0
)

exit /b 1

rem %1 = location, %2 = game type
:run_case
set "_LOCATION=%~1"
set "_GAME_TYPE=%~2"
set "_GAME_KEY=%_GAME_TYPE:-=_%"
set "_CASE_FAILED="

call set "_GAME_DIR=%%GAME_DIR_%_GAME_KEY%%%"
call set "_ENGINE_BIN=%%ENGINE_%_GAME_KEY%%%"
call set "_SKIP=%%SKIP_%_GAME_KEY%%%"

if /i "%_GAME_TYPE%"=="ntre" ( set "_ENGINE_TAG=ntre" ) else ( set "_ENGINE_TAG=sdk" )

if not defined _GAME_DIR (
    if not defined _SKIP set "_SKIP=not available"
    call :record "%_LOCATION%/%_GAME_TYPE%" SKIP "!_SKIP!"
    echo   %_LOCATION% %_GAME_TYPE% ... SKIP ^(!_SKIP!^)
    exit /b 0
)

if not exist "%_GAME_DIR%\gameinfo.txt" (
    call :record "%_LOCATION%/%_GAME_TYPE%" SKIP "no gameinfo.txt in %_GAME_DIR%"
    echo   %_LOCATION% %_GAME_TYPE% ... SKIP ^(no gameinfo.txt in %_GAME_DIR%^)
    exit /b 0
)

call :prepare_location "%_LOCATION%" "%_ENGINE_BIN%" "%_ENGINE_TAG%"
if errorlevel 1 (
    call :record "%_LOCATION%/%_GAME_TYPE%" SKIP "could not stage the tools for this location"
    echo   %_LOCATION% %_GAME_TYPE% ... SKIP ^(could not stage the tools for this location^)
    exit /b 0
)

set "_OUT_DIR=%WORK_DIR%\out\%_LOCATION%-%_GAME_TYPE%"
if not exist "%_OUT_DIR%" mkdir "%_OUT_DIR%" 2>nul
copy /y "%MAP_FILE%" "%_OUT_DIR%\%MAP_NAME%.vmf" >nul

for %%T in (%ALL_TOOLS%) do (
    if not "!TOOLS:,%%T,=!"=="!TOOLS!" (
        if not defined _CASE_FAILED call :run_tool "%%T"
    )
)

set "_CASE_FAILED="
exit /b 0

rem %1 = tool
:run_tool
set "_TOOL=%~1"
set "_LOG=%_OUT_DIR%\%_TOOL%.log"

if defined _TOOL_DIR (
    set "_BINARY=%_TOOL_DIR%\%_TOOL%.exe"
) else (
    if /i "%_TOOL%"=="vbsp" set "_BINARY=%BUILD_DIR%\utils\vbsp\vbsp.exe"
    if /i "%_TOOL%"=="vvis" set "_BINARY=%BUILD_DIR%\utils\vvis_launcher\vvis.exe"
    if /i "%_TOOL%"=="vrad" set "_BINARY=%BUILD_DIR%\utils\vrad_launcher\vrad.exe"
)

set "_ARGS=-game "%_GAME_DIR%""
if /i not "%_TOOL%"=="vbsp" set "_ARGS=%_ARGS% -fast"

<nul set /p "=  %_LOCATION% %_GAME_TYPE% %_TOOL% ... "

set "_SAVED_PATH=%PATH%"
if defined _EXTRA_PATH set "PATH=%_EXTRA_PATH%;%PATH%"
"!_BINARY!" %_ARGS% "%_OUT_DIR%\%MAP_NAME%" > "%_LOG%" 2>&1
set "_EXIT=!ERRORLEVEL!"
set "PATH=%_SAVED_PATH%"

if /i "%_GAME_TYPE%"=="no-engine" (
    rem Everything here has to fail, and it has to fail on the engine lookup
    rem Only vbsp is worth running, the others have no .bsp to chew on
    set "_CASE_FAILED=1"

    if "!_EXIT!"=="0" (
        call :record "%_LOCATION%/%_GAME_TYPE%/%_TOOL%" FAIL "exited 0 without a usable engine"
        echo FAIL ^(exited 0 without a usable engine^)
        exit /b 0
    )

    findstr /c:"Couldn't find appid" /c:"Can't load" "%_LOG%" >nul
    if errorlevel 1 (
        call :record "%_LOCATION%/%_GAME_TYPE%/%_TOOL%" FAIL "failed for an unexpected reason"
        echo FAIL ^(failed for an unexpected reason^)
        call :dump_log "%_LOG%"
    ) else (
        call :record "%_LOCATION%/%_GAME_TYPE%/%_TOOL%" PASS "rejected as expected"
        echo PASS ^(rejected as expected^)
    )
    exit /b 0
)

if not "!_EXIT!"=="0" (
    call :record "%_LOCATION%/%_GAME_TYPE%/%_TOOL%" FAIL "exit code !_EXIT!"
    echo FAIL ^(exit code !_EXIT!^)
    call :dump_log "%_LOG%"
    rem The following tools work on this one's output, so stop here
    set "_CASE_FAILED=1"
    exit /b 0
)

call :check_tool_output "%_TOOL%" "%_LOG%"
if defined _CHECK_REASON (
    call :record "%_LOCATION%/%_GAME_TYPE%/%_TOOL%" FAIL "!_CHECK_REASON!"
    echo FAIL ^(!_CHECK_REASON!^)
    call :dump_log "%_LOG%"
    set "_CASE_FAILED=1"
    exit /b 0
)

call :record "%_LOCATION%/%_GAME_TYPE%/%_TOOL%" PASS ""
echo PASS
exit /b 0

rem %1 = tool, %2 = log file. Sets _CHECK_REASON when something is wrong.
:check_tool_output
set "_CHECK_REASON="

findstr /c:"Can't load" /c:"Couldn't find appid" /c:"Failed to load content for steam AppID" /c:"doesn't say which engine to use" "%~2" >nul
if not errorlevel 1 (
    set "_CHECK_REASON=the engine could not be resolved"
    exit /b 0
)

if /i "%~1"=="vbsp" (
    if not exist "%_OUT_DIR%\%MAP_NAME%.bsp" (
        set "_CHECK_REASON=no %MAP_NAME%.bsp was written"
        exit /b 0
    )
    if "%CHECK_ENGINE_CONTENT%"=="1" (
        rem The default map's skybox only exists in the engine's hl2 content, so not
        rem seeing it means the wrong base directory got mounted
        findstr /c:"%ENGINE_CONTENT_MARKER%" "%~2" >nul
        if errorlevel 1 set "_CHECK_REASON=engine content was not mounted"
    )
    exit /b 0
)

if /i "%~1"=="vvis" (
    findstr /c:"visdatasize" "%~2" >nul
    if errorlevel 1 set "_CHECK_REASON=no visibility data was written"
    exit /b 0
)

if /i "%~1"=="vrad" (
    findstr /c:"BuildFacelights" "%~2" >nul
    if errorlevel 1 (
        set "_CHECK_REASON=no lighting data was written"
        exit /b 0
    )
    findstr /i /r /c:"writing .*\.bsp" "%~2" >nul
    if errorlevel 1 set "_CHECK_REASON=no lighting data was written"
    exit /b 0
)

exit /b 0

rem %1 = key, %2 = PASS/FAIL/SKIP, %3 = detail
:record
if "%~3"=="" (
    >>"%RESULTS_FILE%" echo   %~1	%~2
) else (
    >>"%RESULTS_FILE%" echo   %~1	%~2: %~3
)
if /i "%~2"=="PASS" set /a PASSED+=1
if /i "%~2"=="FAIL" set /a FAILED+=1
if /i "%~2"=="SKIP" set /a SKIPPED+=1
exit /b 0

rem %1 = log file, prints its last lines
:dump_log
set "_LINES=0"
for /f %%C in ('find /c /v "" ^< "%~1"') do set "_LINES=%%C"
set /a _SKIP_LINES=_LINES-15
rem "skip=0" is not valid, so only ask to skip when there is something to skip
if !_SKIP_LINES! GTR 0 (
    for /f "usebackq skip=!_SKIP_LINES! delims=" %%L in ("%~1") do echo       ^| %%L
) else (
    for /f "usebackq delims=" %%L in ("%~1") do echo       ^| %%L
)
exit /b 0
