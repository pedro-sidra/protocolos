cmake_minimum_required(VERSION 3.0)
project(server)
find_package(SDL REQUIRED)

set(SOURCE_FILES server.c)
add_executable(server ${SOURCE_FILES})


TARGET_LINK_LIBRARIES( server pthread)
TARGET_LINK_LIBRARIES( server m)
TARGET_LINK_LIBRARIES( server ${SDL_LIBRARY} )
