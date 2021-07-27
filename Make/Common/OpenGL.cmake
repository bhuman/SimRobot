set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

if(${PLATFORM} STREQUAL Linux)
  find_package(GLEW REQUIRED)
elseif(${PLATFORM} STREQUAL Windows)
  add_library(GLEW::GLEW SHARED IMPORTED)
  set_target_properties(GLEW::GLEW PROPERTIES
      IMPORTED_IMPLIB "${SIMROBOT_PREFIX}/Util/glew/${PLATFORM}/lib/glew32.lib"
      IMPORTED_LOCATION "${SIMROBOT_PREFIX}/Util/glew/${PLATFORM}/bin/glew32.dll"
      INTERFACE_INCLUDE_DIRECTORIES "${SIMROBOT_PREFIX}/Util/glew/${PLATFORM}/include")
endif()
