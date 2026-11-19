set(SIMROBOTCORE3_ROOT_DIR "${SIMROBOT_PREFIX}/Src/SimRobotCore3")

# The graphics API is Metal on macOS and OpenGL on the other platforms.
if(MACOS)
  if(Qt6_VERSION VERSION_LESS "6.7.0")
    message(FATAL_ERROR "SimRobotCore3 requires Qt 6.7 or newer on macOS (QRhiWidget)")
  endif()
  set(SIMROBOTCORE3_GRAPHICS_DEFINITION SIMROBOTCORE3_USE_METAL)
else()
  set(SIMROBOTCORE3_GRAPHICS_DEFINITION SIMROBOTCORE3_USE_OPENGL)
endif()

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
target_link_libraries(SimRobotCore3 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
if(MACOS)
  find_library(METAL_FRAMEWORK Metal)
  find_library(FOUNDATION_FRAMEWORK Foundation)
  find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)

  target_include_directories(SimRobotCore3 SYSTEM PRIVATE "${SIMROBOT_PREFIX}/Util/metal-cpp")
  target_link_libraries(SimRobotCore3 PRIVATE Qt6::GuiPrivate)
  target_link_libraries(SimRobotCore3 PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${QUARTZ_CORE_FRAMEWORK})

  # The implementation of metal-cpp is only generated if its headers are included (with some macros) before anything else.
  set_source_files_properties("${SIMROBOTCORE3_ROOT_DIR}/Graphics/Metal/MetalCpp.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
else()
  target_link_libraries(SimRobotCore3 PRIVATE Qt6::OpenGL Qt6::OpenGLWidgets)
endif()
# The graphics API is part of the interface of SimRobotCore3 (e.g. the signatures of the renderer).
target_compile_definitions(SimRobotCore3 PUBLIC ${SIMROBOTCORE3_GRAPHICS_DEFINITION})
target_link_libraries(SimRobotCore3 PRIVATE Eigen::Eigen)
target_link_libraries(SimRobotCore3 PRIVATE mujoco::mujoco)
target_link_libraries(SimRobotCore3 PRIVATE meshoptimizer)
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
target_compile_definitions(SimRobotCore3Interface INTERFACE ${SIMROBOTCORE3_GRAPHICS_DEFINITION})
target_link_libraries(SimRobotCore3Interface INTERFACE Qt6::Core)
target_link_libraries(SimRobotCore3Interface INTERFACE SimRobotInterface)
