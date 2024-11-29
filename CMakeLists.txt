cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(BetterInputs VERSION 1.0.0)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

file(GLOB_RECURSE SRC_FILES
    "${SRC_DIR}/hooks/*.cpp"
)
set(SRC_FILES ${SRC_FILES}
    ${SRC_DIR}/BetterTextInputNode.cpp
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(SRC_FILES ${SRC_FILES} ${SRC_DIR}/macos.mm)
    set_source_files_properties(${SRC_DIR}/macos.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(SRC_FILES ${SRC_FILES} ${SRC_DIR}/windows.cpp)
else()
    message(FATAL_ERROR "Unsupported mod platform!")
endif()

add_library(${PROJECT_NAME} MODULE "${SRC_FILES}")

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
