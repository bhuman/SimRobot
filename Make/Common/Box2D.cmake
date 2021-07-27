add_library(Box2D::Box2D STATIC IMPORTED)
set_target_properties(Box2D::Box2D PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/box2d/include")
if((${PLATFORM} STREQUAL Linux) OR APPLE)
  set_target_properties(Box2D::Box2D PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/libbox2d.a")
elseif(${PLATFORM} STREQUAL Windows)
  set_target_properties(Box2D::Box2D PROPERTIES
      IMPORTED_CONFIGURATIONS "Debug"
      IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Debug/box2d.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Release/box2d.lib")
endif()
