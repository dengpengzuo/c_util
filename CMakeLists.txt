cmake_minimum_required(VERSION 2.8.4)
project(ez_cutil)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_FLAGS "-Wall -std=gnu99 -pedantic")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${PROJECT_SOURCE_DIR}/lib)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)