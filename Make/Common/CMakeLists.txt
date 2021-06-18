cmake_minimum_required(VERSION 3.15)
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
  set(CMAKE_XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
elseif(WIN32)
  add_definitions(-DWINDOWS)
  set(PLATFORM "Windows")
elseif(UNIX)
  add_definitions(-DLINUX)
  set(PLATFORM "Linux")
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
  add_compile_options(/W3 /GF /GS- /MP /Zc:wchar_t- /EHsc /permissive- /arch:AVX)
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
    add_link_options(-fuse-ld=lld LINKER:--threads)
  endif()
endif()

set(_is_msvc "$<CXX_COMPILER_ID:MSVC>")
set(_is_clang "$<NOT:$<CXX_COMPILER_ID:MSVC>>")
set(_not_mac "$<NOT:$<PLATFORM_ID:Darwin>>")

add_library(Flags::ForDevelop INTERFACE IMPORTED)
target_compile_definitions(Flags::ForDevelop INTERFACE
    $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(Flags::ForDevelop INTERFACE
    $<${_is_msvc}:$<$<NOT:$<CONFIG:Release>>:/ZI>>
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
    $<${_is_msvc}:$<$<CONFIG:Develop>:/SAFESEH:NO>>
    $<${_is_msvc}:$<$<CONFIG:Release>:/INCREMENTAL:NO>>
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
    $<$<AND:${_not_mac},${_is_clang}>:$<$<NOT:$<CONFIG:Debug>>:-s>>)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

if(${PLATFORM} STREQUAL Linux)
  find_package(Qt5 COMPONENTS Core Gui Svg OpenGL Widgets REQUIRED)
  find_package(GLEW REQUIRED)

elseif(${PLATFORM} STREQUAL Windows)
  add_library(Qt5::WinMain STATIC IMPORTED)
  set_target_properties(Qt5::WinMain PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/qtmaind.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/qtmain.lib")

  add_library(Qt5::Core SHARED IMPORTED)
  set_target_properties(Qt5::Core PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Cored.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Cored.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Core.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Core.dll"
      INTERFACE_LINK_LIBRARIES "$<$<AND:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>,$<BOOL:$<TARGET_PROPERTY:WIN32_EXECUTABLE>>>:Qt5::WinMain>"
      INTERFACE_COMPILE_DEFINITIONS "$<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>;QT_CORE_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtCore")

  add_library(Qt5::Gui SHARED IMPORTED)
  set_target_properties(Qt5::Gui PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Guid.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Guid.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Gui.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Gui.dll"
      INTERFACE_LINK_LIBRARIES "Qt5::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_GUI_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtGui")

  add_library(Qt5::Widgets SHARED IMPORTED)
  set_target_properties(Qt5::Widgets PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Widgetsd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Widgetsd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Widgets.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Widgets.dll"
      INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_WIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtWidgets")

  add_library(Qt5::OpenGL SHARED IMPORTED)
  set_target_properties(Qt5::OpenGL PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5OpenGLd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5OpenGLd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5OpenGL.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5OpenGL.dll"
      INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Gui;Qt5::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_OPENGL_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtOpenGL")

  add_library(Qt5::Svg SHARED IMPORTED)
  set_target_properties(Qt5::Svg PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Svgd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Svgd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Svg.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Svg.dll"
      INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Gui;Qt5::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_SVG_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtSvg")

  add_executable(Qt5::moc IMPORTED)
  set_target_properties(Qt5::moc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/moc.exe")

  add_executable(Qt5::rcc IMPORTED)
  set_target_properties(Qt5::rcc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/rcc.exe")

  add_library(Qt5::qwindows MODULE IMPORTED)
  set_target_properties(Qt5::qwindows PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/platforms/qwindowsd.dll"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/platforms/qwindows.dll")

  add_library(Qt5::qjpeg MODULE IMPORTED)
  set_target_properties(Qt5::qjpeg PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/imageformats/qjpegd.dll"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/imageformats/qjpeg.dll")

  set(Qt5Core_VERSION_MAJOR 5)
  set(Qt5Core_VERSION_MINOR 9)

  add_library(GLEW::GLEW SHARED IMPORTED)
  set_target_properties(GLEW::GLEW PROPERTIES
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/glew/${PLATFORM}/lib/glew32.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/glew/${PLATFORM}/bin/glew32.dll"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/glew/${PLATFORM}/include")

elseif(${PLATFORM} STREQUAL macOS)
  add_library(Qt5::Core SHARED IMPORTED)
  set_target_properties(Qt5::Core PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework/QtCore"
      INTERFACE_COMPILE_DEFINITIONS "$<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>;QT_CORE_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework/Headers")

  add_library(Qt5::Gui SHARED IMPORTED)
  set_target_properties(Qt5::Gui PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework/QtGui"
      INTERFACE_LINK_LIBRARIES "Qt5::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_GUI_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework/Headers")

  add_library(Qt5::Widgets SHARED IMPORTED)
  set_target_properties(Qt5::Widgets PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework/QtWidgets"
      INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_WIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework/Headers")

  add_library(Qt5::OpenGL SHARED IMPORTED)
  set_target_properties(Qt5::OpenGL PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework/QtOpenGL"
      INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Gui;Qt5::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_OPENGL_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework/Headers")

  add_library(Qt5::Svg SHARED IMPORTED)
  set_target_properties(Qt5::Svg PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework/QtSvg"
      INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Gui;Qt5::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_SVG_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework/Headers")

  add_executable(Qt5::moc IMPORTED)
  set_target_properties(Qt5::moc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/moc")

  add_executable(Qt5::rcc IMPORTED)
  set_target_properties(Qt5::rcc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/rcc")

  set(Qt5Core_VERSION_MAJOR 5)
  set(Qt5Core_VERSION_MINOR 12)
endif()

add_library(ODE::ODE STATIC IMPORTED)
set_target_properties(ODE::ODE PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/include")
if((${PLATFORM} STREQUAL Linux) OR (${PLATFORM} STREQUAL macOS))
  set_target_properties(ODE::ODE PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/libode.a")
elseif(${PLATFORM} STREQUAL Windows)
  set_target_properties(ODE::ODE PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/ode_doublesd.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/ode_doubles.lib")
endif()

add_library(Box2D::Box2D STATIC IMPORTED)
set_target_properties(Box2D::Box2D PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/box2d/include")
if((${PLATFORM} STREQUAL Linux) OR (${PLATFORM} STREQUAL macOS))
  set_target_properties(Box2D::Box2D PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/libbox2d.a")
elseif(${PLATFORM} STREQUAL Windows)
  set_target_properties(Box2D::Box2D PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Debug/box2d.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Release/box2d.lib")
endif()

add_library(Eigen::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/Eigen"
    INTERFACE_COMPILE_DEFINITIONS "$<$<CONFIG:Debug>:EIGEN_INITIALIZE_MATRICES_BY_NAN>")

set(SIMROBOT_CONTROLLERS Factory SimpleVehicle Soccer)
if(${PLATFORM} STREQUAL macOS)
  set(SIMROBOT_LIBRARY_DIR "${OUTPUT_PREFIX}/Build/${PLATFORM}/SimRobot/$<CONFIG>/SimRobot.app/Contents/lib")
else()
  set(SIMROBOT_LIBRARY_DIR "${OUTPUT_PREFIX}/Build/${PLATFORM}/SimRobot/$<CONFIG>")
endif()

include("../Common/SimRobot.cmake")
include("../Common/SimRobotCommon.cmake")
include("../Common/SimRobotCore2.cmake")
include("../Common/SimRobotCore2D.cmake")
include("../Common/SimRobotEditor.cmake")
include("../Common/SimpleVehicle.cmake")
include("../Common/Factory.cmake")
include("../Common/Soccer.cmake")
