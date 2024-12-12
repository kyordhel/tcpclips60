cmake_minimum_required(VERSION 3.6)
project(socketclips)

## Compile as C++14
add_compile_options(-std=c++14)


###########
## Build ##
###########

## Build the CLIPS library libclips.a
add_subdirectory(clips60)

## Build the bridge library libclipswrapper.a
add_subdirectory(clipswrapper)

## Build the server
add_subdirectory(clipsserver)

## Build the clipscontrol node
# add_subdirectory(clipscontrol)

## Build the test node
# add_subdirectory(test)
