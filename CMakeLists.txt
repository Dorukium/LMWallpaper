# CMake Listfile for LMWallpaper
#
# Copyright (c) 2024 Dorukium
#
# This project is licensed under the MIT License.
# See the LICENSE file for details.

# Minimum CMake version
cmake_minimum_required(VERSION 3.20)

# Project definition
project(LMWallpaper VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vcpkg toolchain setup
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

# Include directories
include_directories(.)
include_directories(./Headers)

# Function to check for header existence and add to list
function(check_and_add_header header_file header_list)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${header_file}")
        set(${header_list} ${${header_list}} "${header_file}" PARENT_SCOPE)
        message(STATUS "Header eklendi: ${header_file}")
    else()
        message(WARNING "Header bulunamadi: ${header_file}")
    endif()
endfunction()

# Function to check for source existence and add to list
function(check_and_add_source source_file source_list)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
        set(${source_list} ${${source_list}} "${source_file}" PARENT_SCOPE)
        message(STATUS "Source eklendi: ${source_file}")
    else()
        message(WARNING "Source bulunamadi: ${source_file}")
    endif()
endfunction()

# Gather header files
set(header_files "")
check_and_add_header("framework.h" header_files)
check_and_add_header("targetvar.h" header_files)
check_and_add_header("Logger.h" header_files)
check_and_add_header("Headers/resource.h" header_files)
check_and_add_header("Headers/ErrorHandler.h" header_files)
check_and_add_header("Headers/TrayManager.h" header_files)
check_and_add_header("Headers/VideoPlayer.h" header_files)
check_and_add_header("Headers/VideoPreview.h" header_files)
check_and_add_header("Headers/MemoryOptimizer.h" header_files)
check_and_add_header("Headers/ImageProcessor.h" header_files)
check_and_add_header("Headers/SettingsWindow.h" header_files)
check_and_add_header("Headers/ThemeManager.h" header_files)
# NOTE: Removed non-existent MonitorManager.h
# check_and_add_header("Headers/MonitorManager.h" header_files) 

# Gather source files
set(source_files "main.cpp" "Logger.cpp")
check_and_add_source("Source/TrayManager.cpp" source_files)
check_and_add_source("Source/VideoPlayer.cpp" source_files)
check_and_add_source("Source/VideoPreview.cpp" source_files)
check_and_add_source("Source/MemoryOptimizer.cpp" source_files)
check_and_add_source("Source/ImageProcessor.cpp" source_files)
check_and_add_source("Source/SettingsWindow.cpp" source_files)
# NOTE: Removed non-existent source files
# check_and_add_source("Source/ThemeManager.cpp" source_files)
# check_and_add_source("Source/MonitorManager.cpp" source_files)
check_and_add_source("Source/ErrorHandler.cpp" source_files)

# Add executable
add_executable(LMWallpaper WIN32 ${source_files} ${header_files})

# Link libraries
set(libraries
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
foreach(lib ${libraries})
    find_library(LIB_${lib} NAMES ${lib})
    if(LIB_${lib})
        target_link_libraries(LMWallpaper PRIVATE ${LIB_${lib}})
        message(STATUS "Kütüphane bulundu: ${lib}")
    else()
        message(WARNING "Kütüphane bulunamadi: ${lib}")
    endif()
endforeach()

# Add resource file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.rc")
    target_sources(LMWallpaper PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.rc")
    message(STATUS "Resource dosyası eklendi")
endif()

# Add manifest file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.manifest")
    set_target_properties(LMWallpaper PROPERTIES LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:\"${CMAKE_CURRENT_SOURCE_DIR}/LMWallpaper.manifest\"")
    message(STATUS "Manifest dosyası eklendi")
endif()

# Output build configuration
message(STATUS "=== LMWallpaper Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source Files: ${source_files}")
message(STATUS "Header Files: ${header_files}")
message(STATUS "=======================================")
