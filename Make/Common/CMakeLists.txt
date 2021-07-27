cmake_minimum_required(VERSION 3.16)
if(CMAKE_HOST_UNIX)
  set(CMAKE_CXX_COMPILER clang++)
endif()
if(APPLE)
  project(SimRobot-temp LANGUAGES CXX)
else()
  project(SimRobot LANGUAGES CXX)
endif()

set(CMAKE_CONFIGURATION_TYPES Debug Develop Release CACHE STRING "" FORCE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SIMROBOT_PREFIX "${PROJECT_SOURCE_DIR}/../..")
set(OUTPUT_PREFIX "${PROJECT_SOURCE_DIR}/../..")

if(APPLE)
  set(CMAKE_SUPPRESS_REGENERATION ON)
  add_definitions(
      -DMACOS
      -DFIX_MACOS_DOCKED_WIDGETS_DRAG_BUG
      -DFIX_MACOS_PINCH_SCALE_RELATIVE_BUG
      -DFIX_MACOS_NO_CENTER_IN_PINCH_GESTURE_BUG
      -DFIX_MACOS_UNDOCKED_WIDGETS_DURING_CLOSE_BUG
      -DFIX_MACOS_UNDOCKED_WIDGETS_DISAPPEAR_WHEN_DOCKED_BUG)
  if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL arm64)
    set(PLATFORM "macOSarm64")
  else()
    set(PLATFORM "macOS")
  endif()
  set(OS "macOS")
  set(CMAKE_XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
elseif(WIN32)
  add_definitions(-DWINDOWS)
  set(PLATFORM "Windows")
  set(OS "${PLATFORM}")
elseif(UNIX)
  add_definitions(-DLINUX)
  set(PLATFORM "Linux")
  set(OS "${PLATFORM}")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER CMake)
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated Files")
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT SimRobot)

set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEVELOP "")
set(CMAKE_CXX_FLAGS_RELEASE "")

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "")
set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "")
set(CMAKE_EXE_LINKER_FLAGS_DEVELOP "")
set(CMAKE_MODULE_LINKER_FLAGS_DEVELOP "")
set(CMAKE_SHARED_LINKER_FLAGS_DEVELOP "")
set(CMAKE_STATIC_LINKER_FLAGS_DEVELOP "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "")

if(MSVC)
  add_compile_options(/W3 /GF /GS- /MP /Zc:wchar_t- /EHsc /permissive- /Zc:preprocessor /arch:AVX)
  add_definitions(-DNOMINMAX -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
else()
  add_compile_options(-pipe
      -Wall
      -Wextra
      -Wconversion
      -Wno-unknown-warning-option
      -Wno-sign-conversion
      -Wno-implicit-int-float-conversion
      -Wunreachable-code
      -Wsign-compare)
  if(NOT ${CMAKE_SYSTEM_PROCESSOR} STREQUAL arm64)
    add_compile_options(-march=native)
  endif()
  if(APPLE)
    add_compile_options(-Wno-deprecated)
  else()
    add_link_options(-fuse-ld=lld)
  endif()
endif()

set(_is_msvc "$<CXX_COMPILER_ID:MSVC>")
set(_is_clang "$<NOT:$<CXX_COMPILER_ID:MSVC>>")
set(_not_mac "$<NOT:$<PLATFORM_ID:Darwin>>")

add_library(Flags::ForDevelop INTERFACE IMPORTED)
target_compile_definitions(Flags::ForDevelop INTERFACE
    $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(Flags::ForDevelop INTERFACE
    $<${_is_msvc}:$<$<CONFIG:Debug>:/ZI>>
    $<${_is_msvc}:$<$<CONFIG:Develop>:/Zi>>
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Release>>:/Od>>
    $<${_is_msvc}:$<$<CONFIG:Develop>:/Ob2>>
    $<${_is_msvc}:$<$<CONFIG:Develop>:/Oi>>
    $<${_is_msvc}:$<$<CONFIG:Develop>:/Ot>>
    $<${_is_msvc}:$<$<CONFIG:Develop>:/Oy>>
    $<${_is_msvc}:$<$<CONFIG:Release>:/Ox>>
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Debug>>:/GT>>
    $<${_is_clang}:$<$<NOT:$<CONFIG:Release>>:-g>>
    $<${_is_clang}:$<$<CONFIG:Release>:-O3>>
    $<${_is_clang}:$<$<CONFIG:Release>:-fomit-frame-pointer>>
    $<${_is_clang}:$<$<CONFIG:Release>:-Wno-unused-parameter>>
    $<${_is_clang}:$<$<CONFIG:Release>:-Wno-unused-variable>>
    $<${_is_clang}:$<$<CONFIG:Release>:-Wno-unused-lambda-capture>>)
target_link_options(Flags::ForDevelop INTERFACE
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Release>>:/DEBUG>>
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>>
    $<${_is_msvc}:$<$<CONFIG:Develop>:/SAFESEH:NO>>
    $<$<AND:${_not_mac},${_is_clang}>:$<$<CONFIG:Release>:-s>>)

add_library(Flags::Default INTERFACE IMPORTED)
target_compile_definitions(Flags::Default INTERFACE
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)
target_compile_options(Flags::Default INTERFACE
    $<${_is_msvc}:$<$<CONFIG:Debug>:/ZI>>
    $<${_is_msvc}:$<$<CONFIG:Debug>:/Od>>
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Debug>>:/Ox>>
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Debug>>:/GT>>
    $<${_is_clang}:$<$<CONFIG:Debug>:-g>>
    $<${_is_clang}:$<$<NOT:$<CONFIG:Debug>>:-O3>>
    $<${_is_clang}:$<$<NOT:$<CONFIG:Debug>>:-fomit-frame-pointer>>
    $<${_is_clang}:$<$<NOT:$<CONFIG:Debug>>:-Wno-unused-parameter>>
    $<${_is_clang}:$<$<NOT:$<CONFIG:Debug>>:-Wno-unused-variable>>
    $<${_is_clang}:$<$<NOT:$<CONFIG:Debug>>:-Wno-unused-lambda-capture>>)
target_link_options(Flags::Default INTERFACE
    $<${_is_msvc}:$<$<CONFIG:Debug>:/DEBUG>>
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>>
    $<$<AND:${_not_mac},${_is_clang}>:$<$<CONFIG:Release>:-s>>)

set(SIMROBOT_CONTROLLERS Factory SimpleVehicle Soccer)
if(APPLE)
  set(SIMROBOT_LIBRARY_DIR "${OUTPUT_PREFIX}/Build/${OS}/SimRobot/$<CONFIG>/SimRobot.app/Contents/lib")
else()
  set(SIMROBOT_LIBRARY_DIR "${OUTPUT_PREFIX}/Build/${OS}/SimRobot/$<CONFIG>")
endif()

include("../Common/Box2D.cmake")
include("../Common/Eigen.cmake")
include("../Common/ODE.cmake")
include("../Common/OpenGL.cmake")
include("../Common/Qt5.cmake")
include("../Common/SimRobot.cmake")
include("../Common/SimRobotCommon.cmake")
include("../Common/SimRobotCore2.cmake")
include("../Common/SimRobotCore2D.cmake")
include("../Common/SimRobotEditor.cmake")
include("../Common/SimpleVehicle.cmake")
include("../Common/Factory.cmake")
include("../Common/Soccer.cmake")
