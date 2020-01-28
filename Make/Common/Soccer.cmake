set(SOCCER_ROOT_DIR "${SIMROBOT_PREFIX}/Src/Controllers")

set(SOCCER_SOURCES "${SOCCER_ROOT_DIR}/SoccerController.cpp")

add_library(Soccer MODULE EXCLUDE_FROM_ALL ${SOCCER_SOURCES})
set_property(TARGET Soccer PROPERTY FOLDER Controllers)
set_property(TARGET Soccer PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET Soccer PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
target_include_directories(Soccer PRIVATE "${SOCCER_ROOT_DIR}")
target_link_libraries(Soccer PRIVATE SimRobotCore2DInterface)
target_compile_options(Soccer PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/GL>>)
target_link_options(Soccer PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/LTCG>>)
target_link_libraries(Soccer PRIVATE Flags::ForDevelop)

source_group(TREE "${SOCCER_ROOT_DIR}" FILES ${SOCCER_SOURCES})
