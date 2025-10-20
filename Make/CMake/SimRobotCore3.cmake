set(SIMROBOTCORE3_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobotCore3")

file(GLOB_RECURSE SIMROBOTCORE3_SOURCES CONFIGURE_DEPENDS
    "${SIMROBOTCORE3_ROOT_DIR}/*.cpp" "${SIMROBOTCORE3_ROOT_DIR}/*.h")
list(APPEND SIMROBOTCORE3_SOURCES "${SIMROBOTCORE3_ROOT_DIR}/SimRobotCore3.qrc")

add_library(SimRobotCore3 MODULE EXCLUDE_FROM_ALL ${SIMROBOTCORE3_SOURCES})
set_property(TARGET SimRobotCore3 PROPERTY FOLDER Libs)
set_property(TARGET SimRobotCore3 PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotCore3 PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimRobotCore3 PROPERTY AUTOMOC ON)
set_property(TARGET SimRobotCore3 PROPERTY AUTORCC ON)
target_include_directories(SimRobotCore3 PRIVATE "${SIMROBOTCORE3_ROOT_DIR}")
target_link_libraries(SimRobotCore3 PRIVATE Qt6::Core Qt6::Gui Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Widgets)
target_link_libraries(SimRobotCore3 PRIVATE Eigen::Eigen)
target_link_libraries(SimRobotCore3 PRIVATE mujoco::mujoco)
target_link_libraries(SimRobotCore3 PRIVATE SimRobotInterface)
target_link_libraries(SimRobotCore3 PRIVATE SimRobotCommon)
target_compile_options(SimRobotCore3 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/GL>>)
target_link_options(SimRobotCore3 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/LTCG>>)
target_link_libraries(SimRobotCore3 PRIVATE Flags::Default)
target_precompile_headers(SimRobotCore3 PRIVATE
    "${SIMROBOTCORE3_ROOT_DIR}/CoreModule.h"
    "${SIMROBOTCORE3_ROOT_DIR}/Simulation/SimObject.h")

source_group(TREE "${SIMROBOTCORE3_ROOT_DIR}" FILES ${SIMROBOTCORE3_SOURCES})

add_library(SimRobotCore3Interface INTERFACE)
target_include_directories(SimRobotCore3Interface SYSTEM INTERFACE "${SIMROBOTCORE3_ROOT_DIR}")
target_link_libraries(SimRobotCore3Interface INTERFACE Qt6::Core)
target_link_libraries(SimRobotCore3Interface INTERFACE SimRobotInterface)
