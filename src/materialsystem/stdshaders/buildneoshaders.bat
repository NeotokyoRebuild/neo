@echo off
setlocal

rem This script builds all the neo shaders

set BUILD_SHADER=call buildshaders.bat

set SOURCE_DIR="..\..\"
set GAME_DIR="..\..\..\game\neo"

%BUILD_SHADER% neoshader_dx9_20b       -game %GAME_DIR% -source %SOURCE_DIR%
rem %BUILD_SHADER% neoshader_dx9_20b_new   -game %GAME_DIR% -source %SOURCE_DIR%
%BUILD_SHADER% neoshader_dx9_30        -game %GAME_DIR% -source %SOURCE_DIR% -force30
