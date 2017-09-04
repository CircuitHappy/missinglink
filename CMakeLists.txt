cmake_minimum_required(VERSION 3.0)

############
# Toolchain
############

set(_BUILDROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE STRING "Directory containing CHIP-buildroot")
set(CMAKE_TOOLCHAIN_FILE cmake_includes/ChipToolchain.cmake)

##########
# Project
##########

project(missing_link)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Os")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(vendor/link/AbletonLinkConfig.cmake)

include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${link_HEADERS})

file(
  GLOB_RECURSE
  missing_link_sources
  "${CMAKE_CURRENT_LIST_DIR}/src/missing_link/*.cpp"
)

file(
  GLOB_RECURSE
  missing_link_display_sources
  "${CMAKE_CURRENT_LIST_DIR}/src/display/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/display/*.cpp"
)

add_executable(missing_link ${missing_link_sources})
add_executable(missing_link_display ${missing_link_display_sources})

set(missing_link_libs pthread Ableton::Link)

target_link_libraries(missing_link ${missing_link_libs})
