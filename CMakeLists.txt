cmake_minimum_required(VERSION 3.0)

##########
# Options
##########

SET(CHIP false CACHE BOOL "Configure for cross-compilation to CHIP environment")
SET(TESTS false CACHE BOOL "Configure with unit tests")

set(CHIP_HOSTNAME missinglink.local CACHE STRING "Name of CHIP deployment target host on network")

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

if(CHIP)
  add_custom_command(
    TARGET missing_link POST_BUILD
    COMMAND scp bin/missing_link root@${CHIP_HOSTNAME}:/usr/bin 
  )
endif()

########
# Tests
########

if(TESTS)
  add_subdirectory(vendor/googletest/googletest)

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

  if(CHIP)
    add_custom_command(
      TARGET missing_link_tests POST_BUILD
      COMMAND scp bin/missing_link_tests root@${CHIP_HOSTNAME}:/usr/bin 
    )
  endif()

endif()


