cmake_minimum_required(VERSION 3.0)
project(testes)
find_package(SDL REQUIRED)

set(SOURCE_FILES testes.c)
add_executable(testes ${SOURCE_FILES})

TARGET_LINK_LIBRARIES( testes m)
TARGET_LINK_LIBRARIES( testes ${SDL_LIBRARY} )