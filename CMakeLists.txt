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
pkg_check_modules(DEPS REQUIRED hyprutils hyprlang pixman-1 libdrm pangocairo)

# Compile flags
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-fno-gnu-unique")
endif()

add_library(hyprscroller SHARED
            src/main.cpp
            src/dispatchers.h
            src/dispatchers.cpp
            src/list.h
            src/overview.h
            src/overview.cpp
            src/scroller.h
            src/scroller.cpp
            src/enums.h
            src/enums.cpp
            src/common.h
            src/sizes.h
            src/sizes.cpp
            src/window.h
            src/window.cpp
            src/decorations.h
            src/decorations.cpp
            src/column.h
            src/column.cpp
            src/row.h
            src/row.cpp
            src/functions.h
            src/functions.cpp
        )

add_compile_definitions(WLR_USE_UNSTABLE)

target_include_directories(hyprscroller PRIVATE ${HYPRLAND_INCLUDE_DIRS} ${DEPS_INCLUDE_DIRS})
set_target_properties(hyprscroller PROPERTIES PREFIX "")
