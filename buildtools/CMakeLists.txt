if(WIN32)
  # incbin tool
  add_executable(incbin incbin.c)
endif()

# Compile tools into a library
add_library(tools_lib STATIC)
target_sources(tools_lib PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${PROJECT_SOURCE_DIR}/mplot/tools.cppm
)

if(NOT WIN32)
  add_executable(process_colourtables process_colourtables.cpp)
  target_link_libraries(process_colourtables tools_lib)
endif()

add_executable(screenshots_to_markdown screenshots_to_markdown.cpp)
target_link_libraries(screenshots_to_markdown tools_lib)
