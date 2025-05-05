cmake_minimum_required(VERSION 3.6)
project(socketclips)

## Compile as C++14
add_compile_options(-std=c++14)
find_package(Boost COMPONENTS thread REQUIRED)

set(TCP_CLIPS60_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include")

###########
## Build ##
###########

## Build the CLIPS library libclips.a
add_subdirectory(clips60)

## Build the bridge library libclipswrapper.a
add_subdirectory(clipswrapper)

## Build the server
add_subdirectory(clipsserver)

## Build the client
add_subdirectory(clipsclient)

## Build the clipscontrol app
add_subdirectory(clipscontrol)

## Build the testing apps
if(NOT DEFINED TCP_CLIPS60_SKIP_TEST_APPS)
add_subdirectory(tests)
endif()
