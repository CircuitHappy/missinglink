cmake_minimum_required(VERSION 3.0)

##########
# Options
##########

SET(CHIP false CACHE BOOL "Configure for cross-compilation to CHIP environment")
SET(TESTS false CACHE BOOL "Configure with unit tests")

############
# Toolchain
############

if(CHIP)
  set(_BUILDROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../CHIP-SDK)
  set(CMAKE_TOOLCHAIN_FILE ./cmake_includes/chip_toolchain.cmake)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

##########
# Project
##########

project(missing_link)

set(CMAKE_CXX_FLAGS "-g -Wall")

include(vendor/link/AbletonLinkConfig.cmake)
include_directories(${link_HEADERS})
include_directories(./include)

file(GLOB_RECURSE missing_link_sources "./src/*.cpp")

add_library(missing_link_lib ${missing_link_sources})
add_executable(missing_link main.cpp)

target_link_libraries(missing_link missing_link_lib)
target_link_libraries(missing_link pthread)
target_link_libraries(missing_link Ableton::Link)

########
# Tests
########

if(TESTS)
  set(BUILD_GTEST ON CACHE BOOL "D")
  add_subdirectory(vendor/googletest/googletest)
  enable_testing()

  file(GLOB_RECURSE missing_link_test_sources "./test/*.cpp")
  add_executable(missing_link_tests ${missing_link_test_sources})

  target_link_libraries(missing_link_tests missing_link_lib)
  target_link_libraries(missing_link_tests gtest gtest_main)

  add_test(tests missing_link_tests)
endif()


