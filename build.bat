@echo off
echo LMWallpaper Build Script
echo ========================

REM Check if Visual Studio is available
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Visual Studio derleyicisi bulunamadi!
    echo Visual Studio Developer Command Prompt'tan calistirin.
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo CMake yapilandiriliyor...
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_INSTALL_PREFIX="%CD%\install" ^
      ..

if %ERRORLEVEL% NEQ 0 (
    echo CMake yapilandirma hatasi!
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo Proje derleniyor...
cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo Derleme hatasi!
    cd ..
    pause
    exit /b 1
)

REM Install (optional)
echo Kurulum dosyalari hazirlaniyor...
cmake --install . --config Release

cd ..

echo.
echo Derleme basariyla tamamlandi!
echo Cikti dosyalari: build\bin\Release\
echo.
pause