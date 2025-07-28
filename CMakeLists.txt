cmake_minimum_required(VERSION 3.20)
project(LMWallpaper VERSION 1.0.0 LANGUAGES CXX)

# C++ standart ayarları
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows platformu kontrolü
if(NOT WIN32)
    message(FATAL_ERROR "Bu proje sadece Windows platformunda çalışır")
endif()

# Build türü ayarları
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Derleyici bayrakları
if(MSVC)
    # Visual Studio bayrakları
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX- /permissive-")
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Od /Zi /RTC1")
        add_compile_definitions(_DEBUG DEBUG)
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
        add_compile_definitions(NDEBUG)
    endif()
else()
    message(FATAL_ERROR "Bu proje MSVC derleyicisi gerektirir")
endif()

# Kaynak dosyaları - sadece mevcut olanlar
set(SOURCES
    main.cpp
    Logger.cpp
)

set(HEADERS
    framework.h
    targetvar.h
    Logger.h
)

# Header dosyalarını kontrol et
set(OPTIONAL_HEADERS
    Headers/resource.h
    Headers/ErrorHandler.h
    Headers/TrayManager.h
    Headers/VideoPlayer.h
    Headers/VideoPreview.h
    Headers/MemoryOptimizer.h
    Headers/ImageProcessor.h
    Headers/SettingsWindow.h
    Headers/ThemeManager.h
    Headers/MonitorManager.h
)

foreach(header ${OPTIONAL_HEADERS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
        list(APPEND HEADERS ${header})
    endif()
endforeach()

# Source dosyalarını kontrol et ve ekle
set(OPTIONAL_SOURCES
    Source/TrayManager.cpp
    Source/VideoPlayer.cpp
    Source/VideoPreview.cpp
    Source/MemoryOptimizer.cpp
    Source/ImageProcessor.cpp
    Source/SettingsWindow.cpp
    Source/ThemeManager.cpp
    Source/MonitorManager.cpp
    Source/ErrorHandler.cpp
)

foreach(source ${OPTIONAL_SOURCES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        list(APPEND SOURCES ${source})
    endif()
endforeach()

# Windows sistem kütüphaneleri
set(WIN_LIBS
    user32
    shell32
    gdi32
    ole32
    oleaut32
    uuid
    comctl32
    advapi32
    kernel32
    comdlg32
    psapi
)

# DirectX/Media Foundation kütüphaneleri - opsiyonel
set(OPTIONAL_LIBS
    d2d1
    dwrite
    dwmapi
    windowscodecs
    mf
    mfplat
    mfreadwrite
    mfuuid
    strmiids
    gdiplus
)

# Executable oluştur
add_executable(LMWallpaper WIN32 ${SOURCES} ${HEADERS})

# Temel Windows kütüphanelerini bağla
target_link_libraries(LMWallpaper ${WIN_LIBS})

# Opsiyonel kütüphaneleri kontrol et ve bağla
foreach(lib ${OPTIONAL_LIBS})
    find_library(${lib}_LIB ${lib})
    if(${lib}_LIB)
        target_link_libraries(LMWallpaper ${lib})
        message(STATUS "Kütüphane bulundu: ${lib}")
    else()
        message(WARNING "Kütüphane bulunamadı: ${lib}")
    endif()
endforeach()

# Include dizinleri
target_include_directories(LMWallpaper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Headers
)

# Preprocessor tanımlamaları
target_compile_definitions(LMWallpaper PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
)

# Windows subsystem ayarları
set_target_properties(LMWallpaper PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(LMWallpaper PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
else()
    set_target_properties(LMWallpaper PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# Resource dosyası kontrolü
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.rc")
    target_sources(LMWallpaper PRIVATE LMWallpaper.rc)
    message(STATUS "Resource dosyası eklendi")
endif()

# Manifest dosyası kontrolü
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.manifest")
    set_target_properties(LMWallpaper PROPERTIES
        LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.manifest"
    )
endif()

# Output dizini ayarları
set_target_properties(LMWallpaper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# Post-build komutları
add_custom_command(TARGET LMWallpaper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:LMWallpaper>
    COMMENT "Output dizini oluşturuluyor"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    add_custom_command(TARGET LMWallpaper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        $<TARGET_FILE_DIR:LMWallpaper>
        COMMENT "README dosyası kopyalanıyor"
    )
endif()

# CPack ayarları (installer için)
set(CPACK_PACKAGE_NAME "LMWallpaper")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dynamic Wallpaper Application")
set(CPACK_PACKAGE_VENDOR "LMWallpaper Team")

# Bilgi mesajları
message(STATUS "=== LMWallpaper Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source Files: ${SOURCES}")
message(STATUS "=======================================")