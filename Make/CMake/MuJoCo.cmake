add_library(mujoco::mujoco SHARED IMPORTED)
set_target_properties(mujoco::mujoco PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}${ARCH}/include")
if(WINDOWS)
  set_target_properties(mujoco::mujoco PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/MuJoCo/Windows/bin/mujoco.dll"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/MuJoCo/Windows/bin/mujoco.dll"
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/MuJoCo/Windows/lib/mujoco.lib")
endif()
if(MACOS)
  set_target_properties(mujoco::mujoco PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/MuJoCo/macOS/lib/libmujoco.3.2.5.dylib")
endif()
