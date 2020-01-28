set(FACTORY_ROOT_DIR "${SIMROBOT_PREFIX}/Src/Controllers")

set(FACTORY_SOURCES "${FACTORY_ROOT_DIR}/FactoryController.cpp")

add_library(Factory MODULE EXCLUDE_FROM_ALL ${FACTORY_SOURCES})
set_property(TARGET Factory PROPERTY FOLDER Controllers)
set_property(TARGET Factory PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET Factory PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
target_include_directories(Factory PRIVATE "${FACTORY_ROOT_DIR}")
target_link_libraries(Factory PRIVATE SimRobotCore2Interface)
target_compile_options(Factory PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/GL>>)
target_link_options(Factory PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/LTCG>>)
target_link_libraries(Factory PRIVATE Flags::ForDevelop)

source_group(TREE "${FACTORY_ROOT_DIR}" FILES ${FACTORY_SOURCES})
