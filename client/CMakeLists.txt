cmake_minimum_required(VERSION 3.0)
project(client)

find_package(SDL REQUIRED)


set(SOURCE_FILES client.c)
add_executable(client ${SOURCE_FILES})

TARGET_LINK_LIBRARIES( client pthread)
TARGET_LINK_LIBRARIES( client m)
TARGET_LINK_LIBRARIES( client ${SDL_LIBRARY} )
