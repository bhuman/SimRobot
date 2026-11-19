set(MESHOPTIMIZER_ROOT_DIR "${SIMROBOT_PREFIX}/Util/meshoptimizer")

# Only the parts that are used (the simplifier and the vertex cache optimizer) are built.
add_library(meshoptimizer STATIC EXCLUDE_FROM_ALL
    "${MESHOPTIMIZER_ROOT_DIR}/src/allocator.cpp"
    "${MESHOPTIMIZER_ROOT_DIR}/src/simplifier.cpp"
    "${MESHOPTIMIZER_ROOT_DIR}/src/vcacheoptimizer.cpp"
    "${MESHOPTIMIZER_ROOT_DIR}/src/meshoptimizer.h")
set_property(TARGET meshoptimizer PROPERTY FOLDER Libs)
set_property(TARGET meshoptimizer PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(meshoptimizer SYSTEM PUBLIC "${MESHOPTIMIZER_ROOT_DIR}/src")
# Meshes are simplified when a scene is loaded, which would take too long without optimization. Nobody needs to debug
# this library, so it is optimized (and its assertions are disabled) in all configurations. That is why it does not use
# Flags::Default, which would add the options of the configuration (e.g. -g or /ZI, which does not work with /O2).
target_compile_definitions(meshoptimizer PRIVATE NDEBUG)
target_compile_options(meshoptimizer PRIVATE
    $<${_is_msvc}:/O2>
    $<${_is_clang}:-O3>
    $<${_is_clang}:-w>)
