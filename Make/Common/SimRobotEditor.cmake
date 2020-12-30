set(SIMROBOTEDITOR_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobotEditor")

file(GLOB_RECURSE SIMROBOTEDITOR_SOURCES
    "${SIMROBOTEDITOR_ROOT_DIR}/*.cpp" "${SIMROBOTEDITOR_ROOT_DIR}/*.h")
list(APPEND SIMROBOTEDITOR_SOURCES "${SIMROBOTEDITOR_ROOT_DIR}/SimRobotEditor.qrc")

add_library(SimRobotEditor MODULE EXCLUDE_FROM_ALL ${SIMROBOTEDITOR_SOURCES})
set_property(TARGET SimRobotEditor PROPERTY FOLDER Libs)
set_property(TARGET SimRobotEditor PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotEditor PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotEditor PROPERTY AUTOMOC ON)
set_property(TARGET SimRobotEditor PROPERTY AUTORCC ON)
target_include_directories(SimRobotEditor PRIVATE "${SIMROBOTEDITOR_ROOT_DIR}")
target_link_libraries(SimRobotEditor PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
target_link_libraries(SimRobotEditor PRIVATE SimRobotInterface)
target_link_libraries(SimRobotEditor PRIVATE Flags::Default)

source_group(TREE "${SIMROBOTEDITOR_ROOT_DIR}" FILES ${SIMROBOTEDITOR_SOURCES})

add_library(SimRobotEditorInterface INTERFACE)
target_include_directories(SimRobotEditorInterface SYSTEM INTERFACE "${SIMROBOTEDITOR_ROOT_DIR}")
target_link_libraries(SimRobotEditorInterface INTERFACE Qt5::Core)
target_link_libraries(SimRobotEditorInterface INTERFACE SimRobotInterface)
