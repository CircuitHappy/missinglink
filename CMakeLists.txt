cmake_minimum_required(VERSION 3.0)

############
# Toolchain
############

set(_BUILDROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(CMAKE_TOOLCHAIN_FILE cmake_includes/ChipToolchain.cmake)

##########
# Project
##########

project(missing_link)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(vendor/link/AbletonLinkConfig.cmake)
include_directories(${link_HEADERS})

include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE missing_link_sources "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

add_executable(missing_link ${missing_link_sources} main.cpp)

set(
  missing_link_libs
  pthread
  Ableton::Link
)

target_link_libraries(missing_link ${missing_link_libs})
