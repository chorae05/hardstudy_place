@echo off
REM ================================================================
REM  BubbleShooter - Quick MSVC build script
REM  Run from a "Developer Command Prompt for VS 2022" or later
REM ================================================================

setlocal

set OUT=BubbleShooter.exe
set SRC=main.cpp Shader.cpp Player.cpp Enemy.cpp Bubble.cpp GameManager.cpp
set LIBS=d3d11.lib dxgi.lib d3dcompiler.lib user32.lib

echo Building %OUT% ...
cl /nologo /std:c++17 /W3 /EHsc /MDd ^
   /D_WINDOWS /D_UNICODE /DUNICODE ^
   /Fe%OUT% %SRC% ^
   /link /SUBSYSTEM:WINDOWS %LIBS%

if %ERRORLEVEL%==0 (
    echo.
    echo Build successful -^> %OUT%
) else (
    echo.
    echo Build FAILED.  See errors above.
)
endlocal
