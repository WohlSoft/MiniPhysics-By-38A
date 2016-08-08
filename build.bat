@echo off
call _paths.bat

PATH=%QtDir%;%MinGW%;%GitDir%;%PATH%
SET SEVENZIP=C:\Program Files\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=C:\Program Files (x86)\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles%\7-Zip

qmake MiniPhysics.pro CONFIG+=release CONFIG-=debug
make

md _packed
cd bin
windeployqt MiniPhysics.exe
cd ..

"%SEVENZIP%\7z" a -tzip "_packed\mini-physics-demo-win32.zip" bin\

