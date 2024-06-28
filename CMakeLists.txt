cmake_minimum_required(VERSION 3.12)
project("hyprscroller"
    LANGUAGES CXX
    DESCRIPTION "Hyprland layout similar to PaperWM and niri")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(HYPRLAND REQUIRED hyprland)
pkg_check_modules(DEPS REQUIRED hyprutils hyprlang pixman-1 libdrm)

# Compile flags
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "-fno-gnu-unique")

add_library(hyprscroller SHARED
            src/main.cpp
            src/dispatchers.h
            src/dispatchers.cpp
            src/list.h
            src/scroller.h
            src/scroller.cpp)

add_compile_definitions(WLR_USE_UNSTABLE)

target_include_directories(hyprscroller PRIVATE ${HYPRLAND_INCLUDE_DIRS} ${HYPRLAND_PREFIX}/hyprland/wlroots-hyprland ${DEPS_INCLUDE_DIRS})
set_target_properties(hyprscroller PROPERTIES PREFIX "")
