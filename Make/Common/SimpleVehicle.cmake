set(SIMPLEVEHICLE_ROOT_DIR "${SIMROBOT_PREFIX}/Src/Controllers")

set(SIMPLEVEHICLE_SOURCES "${SIMPLEVEHICLE_ROOT_DIR}/SimpleVehicleController.cpp")

add_library(SimpleVehicle MODULE EXCLUDE_FROM_ALL ${SIMPLEVEHICLE_SOURCES})
set_property(TARGET SimpleVehicle PROPERTY FOLDER Controllers)
set_property(TARGET SimpleVehicle PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET SimpleVehicle PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
target_include_directories(SimpleVehicle PRIVATE "${SIMPLEVEHICLE_ROOT_DIR}")
target_link_libraries(SimpleVehicle PRIVATE SimRobotCore2Interface)
target_link_libraries(SimpleVehicle PRIVATE OpenGL::GL)

if(NOT APPLE)
  target_link_libraries(SimpleVehicle PRIVATE GLEW::GLEW OpenGL::GL OpenGL::GLU)
endif()

target_compile_options(SimpleVehicle PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/GL>>)
target_link_options(SimpleVehicle PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/LTCG>>)
target_link_libraries(SimpleVehicle PRIVATE Flags::ForDevelop)

source_group(TREE "${SIMPLEVEHICLE_ROOT_DIR}" FILES ${SIMPLEVEHICLE_SOURCES})
