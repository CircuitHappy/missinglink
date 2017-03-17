cmake_minimum_required(VERSION 3.0)

SET(CHIP false CACHE BOOL "Configure for cross-compilation to CHIP environment")

if(CHIP)
  set(_BUILDROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../CHIP-SDK)
  set(CMAKE_TOOLCHAIN_FILE ./cmake_includes/chip_toolchain.cmake)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

project(missing_link)

include(vendor/link/AbletonLinkConfig.cmake)
include_directories(${link_HEADERS})

include_directories(./include)
file(GLOB_RECURSE missing_link_sources "./src/*.cpp")
add_executable(missing_link ${missing_link_sources})

target_link_libraries(missing_link pthread)
target_link_libraries(missing_link Ableton::Link)
