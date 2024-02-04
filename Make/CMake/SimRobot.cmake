set(SIMROBOT_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobot")
set(SIMROBOT_OUTPUT_DIR "${OUTPUT_PREFIX}/Build/${PLATFORM}/SimRobot/$<CONFIG>")

file(GLOB_RECURSE SIMROBOT_SOURCES CONFIGURE_DEPENDS
    "${SIMROBOT_ROOT_DIR}/*.cpp" "${SIMROBOT_ROOT_DIR}/*.h")
list(APPEND SIMROBOT_SOURCES "${SIMROBOT_ROOT_DIR}/SimRobot.qrc")

if(MACOS)
  set(SIMROBOT_ICONS "${SIMROBOT_ROOT_DIR}/Icons/SimRobot.icns" "${SIMROBOT_ROOT_DIR}/Icons/SimRobotDoc.icns")
  list(APPEND SIMROBOT_SOURCES "${SIMROBOT_ICONS}" "${SIMROBOT_ROOT_DIR}/AppleHelper.mm")
else()
  list(APPEND SIMROBOT_SOURCES "${SIMROBOT_ROOT_DIR}/SimRobot.rc")
  list(REMOVE_ITEM SIMROBOT_SOURCES "${SIMROBOT_ROOT_DIR}/AppleHelper.h")
endif()

set(SIMROBOT_TREE "${SIMROBOT_SOURCES}")

if(MACOS)
  set(SIMROBOT_FRAMEWORKS
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtCore.framework"
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtDBus.framework"
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtGui.framework"
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGL.framework"
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtOpenGLWidgets.framework"
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtSvg.framework"
      "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/lib/QtWidgets.framework"
      "${CONTROLLER_FRAMEWORKS}")

  set(SIMROBOT_PLUGIN_COCOA "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/platforms/libqcocoa.dylib")
  set(SIMROBOT_PLUGIN_JPEG "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/imageformats/libqjpeg.dylib")
  set(SIMROBOT_PLUGIN_MACSTYLE "${SIMROBOT_PREFIX}/Util/qt/${PLATFORM}/plugins/styles/libqmacstyle.dylib")
  set(SIMROBOT_PLUGINS "${SIMROBOT_PLUGIN_COCOA}" "${SIMROBOT_PLUGIN_JPEG}" "${SIMROBOT_PLUGIN_MACSTYLE}")

  list(APPEND SIMROBOT_SOURCES "${SIMROBOT_FRAMEWORKS}" "${SIMROBOT_PLUGINS}" "${CONTROLLER_DYLIBS}")

  set_source_files_properties(${SIMROBOT_ICONS} PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources)
  set_source_files_properties(${SIMROBOT_FRAMEWORKS} PROPERTIES
      MACOSX_PACKAGE_LOCATION Frameworks
      XCODE_EXPLICIT_FILE_TYPE wrapper.framework
      XCODE_FILE_ATTRIBUTES "CodeSignOnCopy;RemoveHeadersOnCopy")
  set_source_files_properties(${CONTROLLER_DYLIBS} PROPERTIES
      MACOSX_PACKAGE_LOCATION lib
      XCODE_FILE_ATTRIBUTES "CodeSignOnCopy")
  set_source_files_properties(${SIMROBOT_PLUGIN_COCOA} PROPERTIES
      MACOSX_PACKAGE_LOCATION PlugIns/platforms
      XCODE_FILE_ATTRIBUTES "CodeSignOnCopy")
  set_source_files_properties(${SIMROBOT_PLUGIN_JPEG} PROPERTIES
      MACOSX_PACKAGE_LOCATION PlugIns/imageformats
      XCODE_FILE_ATTRIBUTES "CodeSignOnCopy")
  set_source_files_properties(${SIMROBOT_PLUGIN_MACSTYLE} PROPERTIES
      MACOSX_PACKAGE_LOCATION PlugIns/styles
      XCODE_FILE_ATTRIBUTES "CodeSignOnCopy")

  source_group("Libs" FILES ${SIMROBOT_FRAMEWORKS} ${SIMROBOT_PLUGINS} ${CONTROLLER_DYLIBS})
endif()

add_executable(SimRobot WIN32 MACOSX_BUNDLE "${SIMROBOT_SOURCES}")

set_property(TARGET SimRobot PROPERTY RUNTIME_OUTPUT_DIRECTORY "${SIMROBOT_OUTPUT_DIR}")
set_property(TARGET SimRobot PROPERTY AUTOMOC ON)
set_property(TARGET SimRobot PROPERTY AUTORCC ON)
set_property(TARGET SimRobot PROPERTY MACOSX_BUNDLE_INFO_PLIST "${SIMROBOT_PREFIX}/Make/macOS/Info.plist")
set_property(TARGET SimRobot PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.B-Human.SimRobot")
set_property(TARGET SimRobot PROPERTY XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @executable_path/../lib")
set_property(TARGET SimRobot PROPERTY XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_property(TARGET SimRobot PROPERTY XCODE_GENERATE_SCHEME ON)

target_include_directories(SimRobot PRIVATE "${SIMROBOT_ROOT_DIR}")
target_link_libraries(SimRobot PRIVATE Qt6::Core Qt6::Gui Qt6::OpenGL Qt6::Svg Qt6::Widgets) # Qt6::OpenGL is only needed to register OpenGL support before the main window is created.
if(MACOS)
  target_link_libraries(SimRobot PRIVATE ${APP_KIT_FRAMEWORK})
endif()
add_dependencies(SimRobot SimRobotCore2 SimRobotCore2D SimRobotEditor ${SIMROBOT_CONTROLLERS})

target_link_libraries(SimRobot PRIVATE Flags::Default)

source_group(TREE "${SIMROBOT_ROOT_DIR}" FILES ${SIMROBOT_TREE})

add_library(SimRobotInterface INTERFACE)
target_include_directories(SimRobotInterface SYSTEM INTERFACE "${SIMROBOT_ROOT_DIR}")
target_link_libraries(SimRobotInterface INTERFACE Qt6::Core)

if(WINDOWS)
  add_custom_command(TARGET SimRobot POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:SimRobot>/platforms" "$<TARGET_FILE_DIR:SimRobot>/imageformats"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:Qt6::Core>" "$<TARGET_FILE:Qt6::Gui>" "$<TARGET_FILE:Qt6::OpenGL>" "$<TARGET_FILE:Qt6::OpenGLWidgets>" "$<TARGET_FILE:Qt6::Svg>"
      "$<TARGET_FILE:Qt6::Widgets>" ${CONTROLLER_DYLIBS} "$<TARGET_FILE_DIR:SimRobot>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt6::qwindows>" "$<TARGET_FILE_DIR:SimRobot>/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt6::qjpeg>" "$<TARGET_FILE_DIR:SimRobot>/imageformats")
endif()
