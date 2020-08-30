@ECHO off

REM Whether to include the PBK56S in this solution.
REM Note that it's not included in the default NT mount, so you'll have to provide assets manually.
SET /A IncludePBK56 = 0

SET "VpcBin=devtools\bin\vpc.exe"
SET "BuildParams=/hl2mp +everything /mksln everything.sln"

if %IncludePBK56%==1 (
echo **NOTE**: Including PBK56S in this solution.
%VpcBin% /define:INCLUDE_WEP_PBK %BuildParams%
) else (
%VpcBin% %BuildParams%
)