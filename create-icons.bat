@echo off
echo Icon Dosyalari Olusturucu
echo =========================

REM Check if magick (ImageMagick) is available
where magick >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo ImageMagick bulundu, icon dosyalari olusturuluyor...
    
    REM Create main icon (32x32, blue background with "LM" text)
    magick -size 32x32 xc:blue -fill white -gravity center -pointsize 12 -annotate +0+0 "LM" LMWallpaper.ico
    
    REM Create small icon (16x16)
    magick -size 16x16 xc:blue -fill white -gravity center -pointsize 8 -annotate +0+0 "LM" small.ico
    
    echo Icon dosyalari olusturuldu!
) else (
    echo ImageMagick bulunamadi, varsayilan Windows iconlari kullanilacak.
    echo.
    echo Manuel icon olusturmak icin:
    echo 1. 32x32 pixel LMWallpaper.ico olusturun
    echo 2. 16x16 pixel small.ico olusturun
    echo 3. Bu dosyalari proje klasorune koyun
)

REM Create a simple batch file to copy system icons as fallback
echo @echo off > copy-system-icons.bat
echo echo Sistem iconlari kopyalaniyor... >> copy-system-icons.bat
echo copy "%SystemRoot%\System32\shell32.dll,0" LMWallpaper.ico >> copy-system-icons.bat
echo copy "%SystemRoot%\System32\shell32.dll,0" small.ico >> copy-system-icons.bat
echo echo Sistem iconlari kopyalandi. >> copy-system-icons.bat

echo.
echo Icon dosyalari icin:
echo - ImageMagick varsa otomatik olusturuldu
echo - Yoksa copy-system-icons.bat dosyasini calistirin
echo.
pause