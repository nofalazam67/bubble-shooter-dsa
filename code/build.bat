@echo off
rem Builds the game with MSYS2 UCRT64 g++ and SFML 3
rem (SFML was installed with:  pacman -S mingw-w64-ucrt-x86_64-sfml).
rem It is linked statically, so bubbleshooter.exe runs on its own - no DLLs needed,
rem from any terminal or on another Windows PC.
cd /d "%~dp0"

echo Building bubbleshooter.exe ...
g++ -std=c++17 -O2 -DSFML_STATIC bubbleshooter.cpp -o bubbleshooter.exe -static ^
  -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32 -lwinmm -lgdi32 ^
  -lfreetype -lbz2 -lpng16 -lz -lharfbuzz -lusp10 -lgdi32 -lrpcrt4 -luser32 -ldwrite ^
  -lglib-2.0 -lintl -liconv -lws2_32 -lole32 -lshlwapi -luuid -latomic -lpcre2-8 ^
  -lgraphite2 -lbrotlidec -lbrotlicommon
if errorlevel 1 goto failed

echo.
echo Done: bubbleshooter.exe
goto end

:failed
echo.
echo BUILD FAILED - see the errors above.
:end
pause
