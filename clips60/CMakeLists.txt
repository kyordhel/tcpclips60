cmake_minimum_required(VERSION 3.14)
project(clips)

# add_definitions(-w) # disable all warnings

file(GLOB CLIPS_SRC
  ${PROJECT_SOURCE_DIR}/src/*.c
)

## Declare a C library
add_library(clips60 STATIC
  ${CLIPS_SRC}
)

target_include_directories(clips60
  PRIVATE
  ${TCP_CLIPS60_HEADERS}/clips
)

target_compile_options(clips60
  PRIVATE
  -w
)

# set_target_properties(clips60
#   PROPERTIES
#   OUTPUT_NAME "lib${PROJECT_NAME}.so"
#   PREFIX ""
#   SUFFIX ""
# )
