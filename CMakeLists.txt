cmake_minimum_required(VERSION 2.8)

project(libpc)

set(TARGET libpc)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wall")

link_directories("/usr/local/lib")
include_directories("${PROJECT_SOURCE_DIR}/src")
AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/src" SRC_LIST)

message("Build libpc.a")
add_library(pc STATIC ${SRC_LIST})
set(CMAKE_INSTALL_PREFIX "/usr/local/")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/" DESTINATION "include/libpc" FILES_MATCHING PATTERN "*.h")
install(FILES "${PROJECT_BINARY_DIR}/libpc.a" DESTINATION "lib")
