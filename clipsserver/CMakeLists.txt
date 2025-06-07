cmake_minimum_required(VERSION 3.14)
project(clipsserver)

find_package(Boost REQUIRED COMPONENTS thread filesystem)

file(GLOB CLIPSSERVER_SRC
  ${PROJECT_SOURCE_DIR}/src/*.cpp
)

## Declare an executable
add_executable(clipsserver
  ${CLIPSSERVER_SRC}
)

target_include_directories(clipsserver
  PUBLIC
  clips60/include
  clipswrapper/include
  ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(clipsserver
  clips60
  clipswrapper
  Boost::filesystem
  m
  pthread
)

configure_file("./cubes.clp" "./cubes.clp" COPYONLY )
configure_file("./cubes.dat" "./cubes.dat" COPYONLY )

# set_target_properties(clips60
#   PROPERTIES
#   OUTPUT_NAME "lib${PROJECT_NAME}.so"
#   PREFIX ""
#   SUFFIX ""
# )
