## Minimal FindOpenCV module
## Light-weight helper that attempts to locate common OpenCV components (core, imgproc, highgui)
## This module is used by find_package(OpenCV) as a fallback when a config package is not available.

set(_opencv_libs)

foreach(_comp IN ITEMS core imgproc highgui)
  # common library names (static/shared)
  find_library(_lib
    NAMES opencv_${_comp} libopencv_${_comp}
    PATHS ${CMAKE_FIND_ROOT_PATH} ${CMAKE_PREFIX_PATH} /usr/lib /usr/local/lib
  )
  if(_lib)
    list(APPEND _opencv_libs ${_lib})
  else()
    message(STATUS "FindOpenCV: component '${_comp}' not found by fallback search")
  endif()
endforeach()

if(_opencv_libs)
  set(OpenCV_LIBS ${_opencv_libs} PARENT_SCOPE)
  set(OpenCV_FOUND TRUE PARENT_SCOPE)
else()
  set(OpenCV_FOUND FALSE PARENT_SCOPE)
endif()
