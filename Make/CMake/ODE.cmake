add_library(ODE::ODE STATIC IMPORTED)
set_target_properties(ODE::ODE PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}${ARCH}/include")
if(WINDOWS)
  set_target_properties(ODE::ODE PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/ode_doublesd.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}/lib/ode_doubles.lib")
else()
  set_target_properties(ODE::ODE PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/ode/${PLATFORM}${ARCH}/lib/libode.a")
endif()
