if(LINUX)
  find_package(Qt6 COMPONENTS Concurrent Core Gui OpenGL OpenGLWidgets Svg SvgWidgets Widgets REQUIRED)
elseif(WINDOWS)
  add_library(Qt6::Concurrent SHARED IMPORTED)
  set_target_properties(Qt6::Concurrent PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Concurrentd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Concurrentd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Concurrent.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Concurrent.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_CONCURRENT_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtConcurrent")

  add_library(Qt6::Core SHARED IMPORTED)
  set_target_properties(Qt6::Core PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Cored.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Cored.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Core.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Core.dll"
      INTERFACE_LINK_LIBRARIES "$<$<AND:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>,$<BOOL:$<TARGET_PROPERTY:WIN32_EXECUTABLE>>>:Qt6::EntryPoint>"
      INTERFACE_COMPILE_DEFINITIONS "$<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>;QT_CORE_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtCore")

  add_library(Qt6::EntryPoint STATIC IMPORTED)
  set_target_properties(Qt6::EntryPoint PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6EntryPointd.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6EntryPoint.lib")

  add_library(Qt6::Gui SHARED IMPORTED)
  set_target_properties(Qt6::Gui PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Guid.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Guid.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Gui.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Gui.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_GUI_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtGui")

  add_library(Qt6::OpenGL SHARED IMPORTED)
  set_target_properties(Qt6::OpenGL PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6OpenGLd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6OpenGLd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6OpenGL.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6OpenGL.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_OPENGL_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtOpenGL")

  add_library(Qt6::OpenGLWidgets SHARED IMPORTED)
  set_target_properties(Qt6::OpenGLWidgets PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6OpenGLWidgetsd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6OpenGLWidgetsd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6OpenGLWidgets.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6OpenGLWidgets.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::OpenGL;Qt6::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_OPENGLWIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtOpenGLWidgets")

  add_library(Qt6::Svg SHARED IMPORTED)
  set_target_properties(Qt6::Svg PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Svgd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Svgd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Svg.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Svg.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_SVG_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtSvg")

  add_library(Qt6::SvgWidgets SHARED IMPORTED)
  set_target_properties(Qt6::SvgWidgets PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6SvgWidgetsd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6SvgWidgetsd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6SvgWidgets.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6SvgWidgets.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui;Qt6::Svg;Qt6::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_SVGWIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtSvgWidgets")

  add_library(Qt6::Widgets SHARED IMPORTED)
  set_target_properties(Qt6::Widgets PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_IMPLIB_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Widgetsd.lib"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Widgetsd.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/Qt6Widgets.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/Qt6Widgets.dll"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_WIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/include/QtWidgets")

  add_executable(Qt6::moc IMPORTED)
  set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/moc.exe")

  add_executable(Qt6::rcc IMPORTED)
  set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/bin/rcc.exe")

  add_library(Qt6::qwindows MODULE IMPORTED)
  set_target_properties(Qt6::qwindows PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/platforms/qwindowsd.dll"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/platforms/qwindows.dll")

  add_library(Qt6::qjpeg MODULE IMPORTED)
  set_target_properties(Qt6::qjpeg PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/imageformats/qjpegd.dll"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/imageformats/qjpeg.dll")

  set(Qt6Core_VERSION_MAJOR 6)
  set(Qt6Core_VERSION_MINOR 3)
elseif(MACOS)
  add_library(Qt6::Concurrent SHARED IMPORTED)
  set_target_properties(Qt6::Concurrent PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtConcurrent.framework/QtConcurrent"
      INTERFACE_LINK_LIBRARIES "Qt6::Core"
      INTERFACE_COMPILE_DEFINITIONS "QT_CONCURRENT_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtConcurrent.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtConcurrent.framework/Headers")

  add_library(Qt6::Core SHARED IMPORTED)
  set_target_properties(Qt6::Core PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework/QtCore"
      INTERFACE_COMPILE_DEFINITIONS "$<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>;QT_CORE_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework/Headers")

  add_library(Qt6::DBus SHARED IMPORTED)
  set_target_properties(Qt6::DBus PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtDBus.framework/QtDBus"
      INTERFACE_COMPILE_DEFINITIONS "QT_DBUS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtDBus.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtDBus.framework/Headers")

  add_library(Qt6::Gui SHARED IMPORTED)
  set_target_properties(Qt6::Gui PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework/QtGui"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::DBus"
      INTERFACE_COMPILE_DEFINITIONS "QT_GUI_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework/Headers")

  add_library(Qt6::OpenGL SHARED IMPORTED)
  set_target_properties(Qt6::OpenGL PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework/QtOpenGL"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_OPENGL_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework/Headers")

  add_library(Qt6::OpenGLWidgets SHARED IMPORTED)
  set_target_properties(Qt6::OpenGLWidgets PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGLWidgets.framework/QtOpenGLWidgets"
      INTERFACE_LINK_LIBRARIES "Qt6::OpenGL;Qt6::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_OPENGLWIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGLWidgets.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGLWidgets.framework/Headers")

  add_library(Qt6::Svg SHARED IMPORTED)
  set_target_properties(Qt6::Svg PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework/QtSvg"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_SVG_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework/Headers")

  add_library(Qt6::SvgWidgets SHARED IMPORTED)
  set_target_properties(Qt6::SvgWidgets PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvgWidgets.framework/QtSvgWidgets"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui;Qt6::Svg;Qt6::Widgets"
      INTERFACE_COMPILE_DEFINITIONS "QT_SVGWIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvgWidgets.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvgWidgets.framework/Headers")

  add_library(Qt6::Widgets SHARED IMPORTED)
  set_target_properties(Qt6::Widgets PROPERTIES
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework/QtWidgets"
      INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Gui"
      INTERFACE_COMPILE_DEFINITIONS "QT_WIDGETS_LIB"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework;${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework/Headers")

  add_executable(Qt6::moc IMPORTED)
  set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/libexec/moc")

  add_executable(Qt6::rcc IMPORTED)
  set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/libexec/rcc")

  set(Qt6Core_VERSION_MAJOR 6)
  set(Qt6Core_VERSION_MINOR 3)
endif()
