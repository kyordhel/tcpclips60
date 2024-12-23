cmake_minimum_required(VERSION 3.0.2)
project(tests)

find_package(Boost COMPONENTS thread REQUIRED)

add_executable(testsimple
  simple/main.cpp
)

target_link_libraries(testsimple
  m
  Boost::thread
)


add_executable(testclient
  client/main.cpp
)

target_include_directories(testclient
  PUBLIC
  ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(testclient
  clipsclient
  m
  Boost::thread
)
