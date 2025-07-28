@echo off
echo vcpkg Recursion Hatasi Cozumu
echo ==============================

REM vcpkg integration'i temizle
echo vcpkg integration temizleniyor...
vcpkg integrate remove

REM Environment variable'lari temizle
set VCPKG_ROOT=
set VCPKG_DEFAULT_TRIPLET=
set CMAKE_TOOLCHAIN_FILE=

REM Yeni integration kurulumu
echo Yeni vcpkg integration kuruluyor...
vcpkg integrate install

echo.
echo vcpkg integration yenilendi!
echo Simdi projeyi yeniden derleyebilirsiniz.
echo.
pause