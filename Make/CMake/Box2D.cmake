if(LINUX)
  find_package(box2d REQUIRED)
else()
  add_library(box2d::box2d STATIC IMPORTED)
  set_target_properties(box2d::box2d PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/box2d/include")
  if(MACOS)
    set_target_properties(box2d::box2d PROPERTIES IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}${ARCH}/libbox2d.a")
  elseif(WINDOWS)
    set_target_properties(box2d::box2d PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug"
        IMPORTED_LOCATION_DEBUG "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Debug/box2d.lib"
        IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/box2d/lib/${PLATFORM}/Release/box2d.lib")
  endif()
endif()
