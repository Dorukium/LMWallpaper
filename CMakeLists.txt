cmake_minimum_required(VERSION 3.16)
project(LMWallpaper)

# Windows API ayarları
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Derleme ayarları
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug ve Release ayarları
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MDd /Od /Zi")
    add_definitions(-D_DEBUG)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD /O2 /Ot /Oi")
    add_definitions(-DNDEBUG)
endif()

# Kaynak dosyaları
set(SOURCES
    main.cpp
    Source/TrayManager.cpp
    Source/VideoPlayer.cpp
    Source/VideoPreview.cpp
    Source/MemoryOptimizer.cpp
    Source/ImageProcessor.cpp
    Source/SettingsWindow.cpp
)

set(HEADERS
    Headers/resource.h
    Headers/framework.h
    Headers/targetver.h
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

# Windows kütüphaneleri
set(WIN_LIBS
    user32
    shell32
    gdi32
    ole32
    comctl32
    d2d1
    dwrite
    dwmapi
    psapi
    strmiids
    oleaut32
    uuid
    windowscodecs
    mf
    mfplat
    mfreadwrite
    mfuuid
    gdiplus
)

# Executable oluşturma
add_executable(LMWallpaper WIN32 ${SOURCES} ${HEADERS})

# Kütüphaneleri bağlama
target_link_libraries(LMWallpaper ${WIN_LIBS})

# Include directories
target_include_directories(LMWallpaper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Headers
)

# Preprocessor definitions
target_compile_definitions(LMWallpaper PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

# Subsystem ayarı
set_target_properties(LMWallpaper PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

# Debug için console göster
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(LMWallpaper PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

# Resource dosyası (eğer varsa)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.rc")
    target_sources(LMWallpaper PRIVATE LMWallpaper.rc)
endif()

# Post-build commands
add_custom_command(TARGET LMWallpaper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    $<TARGET_FILE_DIR:LMWallpaper>
)
