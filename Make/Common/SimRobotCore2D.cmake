set(SIMROBOTCORE2D_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobotCore2D")

file(GLOB_RECURSE SIMROBOTCORE2D_SOURCES
    "${SIMROBOTCORE2D_ROOT_DIR}/*.cpp" "${SIMROBOTCORE2D_ROOT_DIR}/*.h")
list(APPEND SIMROBOTCORE2D_SOURCES "${SIMROBOTCORE2D_ROOT_DIR}/SimRobotCore2D.qrc")

add_library(SimRobotCore2D MODULE EXCLUDE_FROM_ALL ${SIMROBOTCORE2D_SOURCES})
set_property(TARGET SimRobotCore2D PROPERTY FOLDER Libs)
set_property(TARGET SimRobotCore2D PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotCore2D PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotCore2D PROPERTY AUTOMOC ON)
set_property(TARGET SimRobotCore2D PROPERTY AUTORCC ON)
target_include_directories(SimRobotCore2D PRIVATE "${SIMROBOTCORE2D_ROOT_DIR}")
target_link_libraries(SimRobotCore2D PRIVATE Qt5::Core Qt5::Gui Qt5::Svg Qt5::Widgets)
target_link_libraries(SimRobotCore2D PRIVATE Eigen::Eigen)
target_link_libraries(SimRobotCore2D PRIVATE Box2D::Box2D)
target_link_libraries(SimRobotCore2D PRIVATE SimRobotInterface)
target_link_libraries(SimRobotCore2D PRIVATE SimRobotCommon)
target_compile_options(SimRobotCore2D PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/GL>>)
target_link_options(SimRobotCore2D PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/LTCG>>)
target_link_libraries(SimRobotCore2D PRIVATE Flags::Default)
target_precompile_headers(SimRobotCore2D PRIVATE
    "${SIMROBOTCORE2D_ROOT_DIR}/Simulation/SimObject.h"
    <box2d/b2_math.h>
    <QIcon>
    <QPainter>)

source_group(TREE "${SIMROBOTCORE2D_ROOT_DIR}" FILES ${SIMROBOTCORE2D_SOURCES})

add_library(SimRobotCore2DInterface INTERFACE)
target_include_directories(SimRobotCore2DInterface SYSTEM INTERFACE "${SIMROBOTCORE2D_ROOT_DIR}")
target_link_libraries(SimRobotCore2DInterface INTERFACE Qt5::Core Qt5::Gui)
target_link_libraries(SimRobotCore2DInterface INTERFACE SimRobotInterface)
