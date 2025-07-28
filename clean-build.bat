@echo off
echo LMWallpaper Clean Build
echo =======================

REM Clean previous build
if exist "build" (
    echo Onceki build temizleniyor...
    rmdir /s /q build
)

if exist "CMakeCache.txt" (
    del CMakeCache.txt
)

if exist "CMakeFiles" (
    rmdir /s /q CMakeFiles
)

REM Create fresh build directory
mkdir build
cd build

REM Configure without vcpkg to avoid recursion
echo CMake yapilandiriliyor (vcpkg olmadan)...
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_GENERATOR_PLATFORM=x64 ^
      ..

if %ERRORLEVEL% NEQ 0 (
    echo CMake yapilandirma hatasi!
    echo.
    echo Cozum onerileri:
    echo 1. Visual Studio 2022 yuklu oldugunu kontrol edin
    echo 2. vcpkg-fix.bat dosyasini calistirin
    echo 3. Developer Command Prompt kullanin
    cd ..
    pause
    exit /b 1
)

REM Build
echo Proje derleniyor...
cmake --build . --config Release --verbose

if %ERRORLEVEL% NEQ 0 (
    echo Derleme hatasi!
    echo Log dosyalarini kontrol edin.
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ================================
echo DERLEME BASARIYLA TAMAMLANDI!
echo ================================
echo.
echo Cikti dosyalari:
echo - build\Release\LMWallpaper.exe
echo - build\bin\Release\LMWallpaper.exe
echo.

REM Test the executable
if exist "build\Release\LMWallpaper.exe" (
    echo LMWallpaper.exe dosyasi olusturuldu: build\Release\
) else if exist "build\bin\Release\LMWallpaper.exe" (
    echo LMWallpaper.exe dosyasi olusturuldu: build\bin\Release\
) else (
    echo UYARI: LMWallpaper.exe dosyasi bulunamadi!
)

pause