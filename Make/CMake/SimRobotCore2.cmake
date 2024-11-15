set(SIMROBOTCORE2_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobotCore2")

file(GLOB_RECURSE SIMROBOTCORE2_SOURCES CONFIGURE_DEPENDS
    "${SIMROBOTCORE2_ROOT_DIR}/*.cpp" "${SIMROBOTCORE2_ROOT_DIR}/*.h")
list(APPEND SIMROBOTCORE2_SOURCES "${SIMROBOTCORE2_ROOT_DIR}/SimRobotCore2.qrc")

add_library(SimRobotCore2 MODULE EXCLUDE_FROM_ALL ${SIMROBOTCORE2_SOURCES})
set_property(TARGET SimRobotCore2 PROPERTY FOLDER Libs)
set_property(TARGET SimRobotCore2 PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotCore2 PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotCore2 PROPERTY AUTOMOC ON)
set_property(TARGET SimRobotCore2 PROPERTY AUTORCC ON)
target_include_directories(SimRobotCore2 PRIVATE "${SIMROBOTCORE2_ROOT_DIR}")
target_link_libraries(SimRobotCore2 PRIVATE Qt6::Core Qt6::Gui Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Widgets)
target_link_libraries(SimRobotCore2 PRIVATE Eigen::Eigen)
target_link_libraries(SimRobotCore2 PRIVATE ODE::ODE)
target_link_libraries(SimRobotCore2 PRIVATE SimRobotInterface)
target_link_libraries(SimRobotCore2 PRIVATE SimRobotCommon)
target_compile_options(SimRobotCore2 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/GL>>)
target_link_options(SimRobotCore2 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/LTCG>>)
target_link_libraries(SimRobotCore2 PRIVATE Flags::Default)
target_precompile_headers(SimRobotCore2 PRIVATE
    "${SIMROBOTCORE2_ROOT_DIR}/CoreModule.h"
    "${SIMROBOTCORE2_ROOT_DIR}/Simulation/SimObject.h")

source_group(TREE "${SIMROBOTCORE2_ROOT_DIR}" FILES ${SIMROBOTCORE2_SOURCES})

add_library(SimRobotCore2Interface INTERFACE)
target_include_directories(SimRobotCore2Interface SYSTEM INTERFACE "${SIMROBOTCORE2_ROOT_DIR}")
target_link_libraries(SimRobotCore2Interface INTERFACE Qt6::Core)
target_link_libraries(SimRobotCore2Interface INTERFACE SimRobotInterface)
