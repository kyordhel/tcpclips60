cmake_minimum_required(VERSION 3.6)
project(clipscontrol)


set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread filesystem)
# add_definitions(-w) # disable all warnings

file(GLOB CLIPSCONTROL_SRC
  ${PROJECT_SOURCE_DIR}/src/*.cpp
)


## Declare an executable
add_executable(clipscontrol
  ${CLIPSCONTROL_SRC}
)

target_include_directories(clipscontrol
  PUBLIC
  ${CURSES_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(clipscontrol
  clipsclient
  Boost::filesystem
  Boost::thread
  ${BOOST_LIBRARIES}
  ${CURSES_LIBRARIES}
)

