#
# wxWindows/wxWidgets setup. This CMakeLists.txt is processed only if the top-level
# find_package(wxWidgets...) call found what it was looking for.
#
# Ensure we can #include <morph/header.h>
include_directories(BEFORE ${PROJECT_SOURCE_DIR})

# wxWidgets has some additional compiler flags...
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")
# ...and include directories
include_directories(${wxWidgets_INCLUDE_DIRS})

# morph::Visual graph1 example with morph/wx/viswx.h
add_executable (wx-graph1 wx-graph1.cpp)
target_compile_definitions(wx-graph1 PRIVATE "${wxWidgets_DEFINITIONS} -Wno-unused-parameter")
target_link_libraries(wx-graph1 ${wxWidgets_LIBRARIES} OpenGL::GL Freetype::Freetype)

# morph::Visual graph6 example with morph/wx/viswx.h
add_executable (wx-graph6 wx-graph6.cpp)
target_compile_definitions(wx-graph6 PRIVATE "${wxWidgets_DEFINITIONS} -Wno-unused-parameter")
target_link_libraries(wx-graph6 ${wxWidgets_LIBRARIES} OpenGL::GL Freetype::Freetype)

# Triaxes
add_executable (wx-triaxes wx-triaxes.cpp)
target_compile_definitions(wx-triaxes PRIVATE "${wxWidgets_DEFINITIONS} -Wno-unused-parameter")
target_link_libraries(wx-triaxes ${wxWidgets_LIBRARIES} OpenGL::GL Freetype::Freetype)
