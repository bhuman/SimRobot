set(SIMROBOTCOMMON_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobotCommon")

file(GLOB_RECURSE SIMROBOTCOMMON_SOURCES "${SIMROBOTCOMMON_ROOT_DIR}/*.cpp" "${SIMROBOTCOMMON_ROOT_DIR}/*.h")

add_library(SimRobotCommon STATIC EXCLUDE_FROM_ALL ${SIMROBOTCOMMON_SOURCES})
set_property(TARGET SimRobotCommon PROPERTY FOLDER Libs)
set_property(TARGET SimRobotCommon PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(SimRobotCommon PUBLIC "${SIMROBOTCOMMON_ROOT_DIR}")
target_link_libraries(SimRobotCommon PUBLIC Eigen::Eigen)

target_compile_options(SimRobotCommon PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:/GL>>)
target_link_libraries(SimRobotCommon PRIVATE Flags::Default)
set_property(TARGET SimRobotCommon PROPERTY XCODE_ATTRIBUTE_OTHER_LIBTOOLFLAGS -no_warning_for_no_symbols)

source_group(TREE "${SIMROBOTCOMMON_ROOT_DIR}" FILES ${SIMROBOTCOMMON_SOURCES})
