add_library(ODE::ODE STATIC IMPORTED)
set_target_properties(ODE::ODE PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/include")
if((${PLATFORM} STREQUAL Linux) OR APPLE)
  set_target_properties(ODE::ODE PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/libode.a")
elseif(${PLATFORM} STREQUAL Windows)
  set_target_properties(ODE::ODE PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/ode_doublesd.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/ode_doubles.lib")
endif()
