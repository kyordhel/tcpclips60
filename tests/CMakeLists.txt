cmake_minimum_required(VERSION 3.0.2)
project(testclient)

file(GLOB TESTCLIENT_SRC
  ${PROJECT_SOURCE_DIR}/*.cpp
)

## Declare an executable
add_executable(testclient
  ${TESTCLIENT_SRC}
)

target_include_directories(testclient
  PUBLIC
  ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(testclient
  m
  Boost::thread
)
