cmake_minimum_required(VERSION 3.10)
project(LMWallpaper)

# Windows API ayarları
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# vcpkg toolchain ayarı
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

# Derleme ayarları
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Kaynak dosyaları
file(GLOB SOURCES "Source/*.cpp")
file(GLOB HEADERS "Headers/*.h")

# OpenCV kurulumu
find_package(OpenCV REQUIRED)

# Windows API kütüphaneleri
link_directories(${CMAKE_SYSTEM_ROOT}/System32)
link_libraries(user32 shell32 gdi32 ole32 comctl32)

# Uygulama oluşturma
add_executable(LMWallpaper ${SOURCES} ${HEADERS})

# Bağlı kütüphaneler
target_link_libraries(LMWallpaper ${OpenCV_LIBS})

# Optimizasyon ayarları
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ot /Oi /MT")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/STACK:256000000")