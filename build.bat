@echo off

rem Build the assets
IF NOT EXIST temp MD temp
python tools\build_assets.py tools\assets.json temp\assets.bin
IF EXIST temp\assets.bin.gz DEL /F temp\assets.bin.gz
tools\gzip\bin\gzip.exe -k temp\assets.bin

rem Generate resources.h file
python tools\bin2c.py temp\assets.bin.gz temp\resources.h
move temp\resources.h src\resources.h

rem Build the project
IF NOT EXIST build MD build
set Compiler=cl
set LinkerFlags=-incremental:no -opt:ref -nodefaultlib -entry:main kernel32.lib -stack:100000,100000
set CompilerFlags=-nologo -W4 -GS- -Gs99999
set DebugFlags=-FC -Z7
%Compiler% src\main.c -Os /Febuild\debug.exe -DGFX=0 %DebugFlags% %CompilerFlags% -link %LinkerFlags%
%Compiler% src\main.c -Os /Febuild\nogfx.exe -DGFX=0 %CompilerFlags% -link %LinkerFlags%
%Compiler% src\main.c -Os /Febuild\flappy.exe -DGFX=1 %CompilerFlags% -link %LinkerFlags%

