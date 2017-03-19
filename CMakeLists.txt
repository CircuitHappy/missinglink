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
  set(CMAKE_TOOLCHAIN_FILE cmake_includes/ChipToolchain.cmake)
endif()

##########
# Project
##########

project(missing_link)

set(CMAKE_CXX_FLAGS "-g -Wall")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(vendor/link/AbletonLinkConfig.cmake)

include_directories(${link_HEADERS})
include_directories(src)

file(GLOB_RECURSE missing_link_sources "src/*.cpp" "src/*.hpp")
add_executable(missing_link ${missing_link_sources} main.cpp)

set(
  missing_link_libs
  pthread
  Ableton::Link
)

target_link_libraries(missing_link ${missing_link_libs})

########
# Tests
########

if(TESTS)
  add_subdirectory(vendor/googletest/googletest)
  enable_testing()

  file(GLOB_RECURSE missing_link_test_sources "test/*.cpp")

  set(
    missing_link_test_libs
    gtest
    gtest_main
  )
  
  add_executable(
    missing_link_tests 
    ${missing_link_sources}
    ${missing_link_test_sources}
  )

  target_link_libraries(
    missing_link_tests 
    ${missing_link_libs}
    ${missing_link_test_libs}
  )

  add_test(tests missing_link_tests)
endif()


