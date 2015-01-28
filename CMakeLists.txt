cmake_minimum_required(VERSION 3.0)

project("airtame-cli")

link_directories(${CMAKE_SOURCE_DIR}/deps/out/lib/)
include_directories(${CMAKE_SOURCE_DIR}/deps/out/include/)

add_subdirectory(src)
