set(BENCHMARK_ROOT_DIR "${SIMROBOT_PREFIX}/Src/Controllers")

set(BENCHMARK_SOURCES "${BENCHMARK_ROOT_DIR}/BenchmarkController.cpp")

add_library(Benchmark MODULE EXCLUDE_FROM_ALL ${BENCHMARK_SOURCES})
set_property(TARGET Benchmark PROPERTY FOLDER Controllers)
set_property(TARGET Benchmark PROPERTY LIBRARY_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
set_property(TARGET Benchmark PROPERTY PDB_OUTPUT_DIRECTORY "${SIMROBOT_LIBRARY_DIR}")
target_include_directories(Benchmark PRIVATE "${BENCHMARK_ROOT_DIR}")
target_link_libraries(Benchmark PRIVATE SimRobotInterface)
target_link_libraries(Benchmark PRIVATE SimRobotCore2Interface)
target_compile_options(Benchmark PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/GL>>)
target_link_options(Benchmark PRIVATE $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/LTCG>>)
target_link_libraries(Benchmark PRIVATE Flags::DebugInDevelop)

source_group(TREE "${BENCHMARK_ROOT_DIR}" FILES ${BENCHMARK_SOURCES})
