@echo off
call _paths.bat

PATH=%QtDir%;%MinGW%;%GitDir%;%PATH%
SET SEVENZIP=C:\Program Files\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=C:\Program Files (x86)\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles%\7-Zip

qmake MiniPhysics.pro CONFIG+=release CONFIG-=debug
IF ERRORLEVEL 1 goto error

mingw32-make
IF ERRORLEVEL 1 goto error

md _packed
cd bin
windeployqt MiniPhysics.exe
IF ERRORLEVEL 1 goto error
cd ..

"%SEVENZIP%\7z" a -tzip "_packed\mini-physics-demo-win32.zip" bin\

goto quit
:error
echo ==============BUILD ERRORED!===============
:quit

