cmake_minimum_required(VERSION 2.8)

project("airtame-cli")

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEPS_DIR ${ROOT_DIR}/deps)
set(DEPS_OUT_DIR ${DEPS_DIR}/out)

link_directories(${DEPS_OUT_DIR}/lib)
include_directories(${DEPS_OUT_DIR}/include)

add_subdirectory(${DEPS_DIR})
add_subdirectory(src)
