# All #includes in test programs have to be #include <morph/header.h>
include_directories(BEFORE ${PROJECT_SOURCE_DIR})

if(HDF5_FOUND AND ARMADILLO_FOUND)
  add_executable(erm erm.cpp)
  target_compile_definitions(erm PUBLIC FLT=float)
  if(APPLE AND OpenMP_CXX_FOUND)
    target_link_libraries(erm OpenMP::OpenMP_CXX ${ARMADILLO_LIBRARY} ${ARMADILLO_LIBRARIES} OpenGL::GL glfw Freetype::Freetype ${HDF5_C_LIBRARIES})
  else()
    target_link_libraries(erm ${ARMADILLO_LIBRARY} ${ARMADILLO_LIBRARIES} OpenGL::GL glfw Freetype::Freetype ${HDF5_C_LIBRARIES})
  endif()
endif()
