# C.H.I.P. / ARM

set(BUILDROOT_DIR ${_BUILDROOT_DIR}/CHIP-buildroot)

set(BUILD_DIR     ${BUILDROOT_DIR}/output)
set(STAGING_DIR   ${BUILD_DIR}/staging)
set(HOST_DIR      ${BUILD_DIR}/host) 
set(TARGET_DIR    ${BUILD_DIR}/target)

# Symbol define for conditional C/C++ pre-processing

add_definitions(-D_PLATFORM_CHIP)

# CMake toolchain

set(CMAKE_SYSTEM_NAME CHIP_Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(TOOLCHAIN_DIR      ${HOST_DIR}/opt/ext-toolchain/bin)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-linux-gnueabihf-g++)

set(
  CMAKE_FIND_ROOT_PATH 
  ${TARGET_DIR}
  ${TARGET_DIR}/usr/lib
)

include_directories(
  ${HOST_DIR}/usr/include
  ${TARGET_DIR}/usr/include
  ${HOST_DIR}/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
