cmake_minimum_required(VERSION 3.14)
project(clipswrapper)

file(GLOB WRAPPER_SRC
  ${PROJECT_SOURCE_DIR}/src/*.cpp
)

## Declare static library
add_library(clipswrapper STATIC
  ${WRAPPER_SRC}
)

target_include_directories(clipswrapper
  PUBLIC
  ${TCP_CLIPS60_HEADERS}
  ${TCP_CLIPS60_HEADERS}/clipswrapper
)

target_link_libraries(clipswrapper
  clips60
)

# set_target_properties(bridge
#   PROPERTIES
#   OUTPUT_NAME "lib${PROJECT_NAME}.a"
#   PREFIX ""
#   SUFFIX ""
# )
