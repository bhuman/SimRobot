add_library(mujoco::mujoco SHARED IMPORTED)
set_target_properties(mujoco::mujoco PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}/include")
if(WINDOWS)
  if(AVX)
    set_target_properties(mujoco::mujoco PROPERTIES
        IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}/bin/mujoco.dll"
        IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}/lib/mujoco.lib")
  else()
    set_target_properties(mujoco::mujoco PROPERTIES
        IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}/bin-noavx/mujoco.dll"
        IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}/lib/mujoco.lib")
  endif()
elseif(MACOS)
  set_target_properties(mujoco::mujoco PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}/lib/libmujoco.3.3.5.dylib")
elseif(LINUX)
  set_target_properties(mujoco::mujoco PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/MuJoCo/${PLATFORM}${ARCH}/lib/libmujoco.so.3.3.5")
endif()
