if(${PLATFORM} STREQUAL Linux)
  find_package(Qt5 COMPONENTS Concurrent Core Gui OpenGL Svg Widgets REQUIRED)
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
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtCore"
      INTERFACE_SOURCES "${SIMROBOT_PREFIX}/Util/qt/Windows/debug/qt5.natvis")

  add_library(Qt5::Concurrent SHARED IMPORTED)
  set_target_properties(Qt5::Concurrent PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Concurrentd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Concurrentd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Concurrent.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt5Concurrent.dll"
      INTERFACE_LINK_LIBRARIES "Qt5::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_CONCURRENT_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtConcurrent")

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
elseif(APPLE)
  add_library(Qt5::Concurrent SHARED IMPORTED)
  set_target_properties(Qt5::Concurrent PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtConcurrent.framework/QtConcurrent"
      INTERFACE_LINK_LIBRARIES "Qt5::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_CONCURRENT_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtConcurrent.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtConcurrent.framework/Headers")

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
  set(Qt5Core_VERSION_MINOR 15)
endif()
