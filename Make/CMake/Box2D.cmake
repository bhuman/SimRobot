if(${OS} STREQUAL Linux)
  find_package(box2d REQUIRED)
else()
  add_library(box2d::box2d STATIC IMPORTED)
  set_target_properties(box2d::box2d PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/box2d/include")
  if(APPLE)
    set_target_properties(box2d::box2d PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/libbox2d.a")
  elseif(${PLATFORM} STREQUAL Windows)
    set_target_properties(box2d::box2d PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug"
        IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Debug/box2d.lib"
        IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Release/box2d.lib")
  endif()
endif()
