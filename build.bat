@echo off
REM Windows (MinGW) build helper. Linux/macOS equivalent: build.sh
setlocal

if not exist build mkdir build
cd build

cmake -G "MinGW Makefiles" .. || (echo cmake configure failed & exit /b 1)
cmake --build . --parallel   || (echo build failed & exit /b 1)

copy /Y client.exe "%USERPROFILE%\client.exe" >nul
echo.
echo Build complete -^> %USERPROFILE%\client.exe
endlocal
