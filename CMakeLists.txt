cmake_minimum_required(VERSION 3.0)

############
# Toolchain
############

set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(
  CMAKE_FIND_ROOT_PATH 
  ${TARGET_DIR}
  ${TARGET_DIR}/usr/lib
)

include_directories(
  /usr/include
  /usr/include
  /usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

##########
# Project
##########

project(missing_link)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Os")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Os")

include(vendor/link/AbletonLinkConfig.cmake)

include_directories(src)
include_directories(${link_HEADERS})

file(
  GLOB_RECURSE
  missing_link_sources
  "src/missing_link/*.cpp"
  "src/missing_link_display/*.c"
)

add_executable(missing_link ${missing_link_sources})
target_link_libraries(missing_link pthread Ableton::Link)
