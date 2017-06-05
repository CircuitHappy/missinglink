Missing-Link
============

Development repository for Missing Link.

## Installation

**Note: Only native Ubuntu 16.04+ is currently supported for compilation**

1. First, clone CircuitHappy fork of [CHIP-SDK](https://github.com/CircuitHappy/CHIP-SDK)
1. Run the setup script in `CHIP-SDK`

    ```
    ./setup_ubuntu.sh
    ```
    
1. Build the `missinglink` buildroot configuration. _This will take awhile._

    ```
    cd CHIP-SDK/CHIP-buildroot
    make missinglink_defconfig
    make
    ```
    
1. In the same directory that `CHIP-SDK` is located, clone `MissingLink` 
   recursively or clone and then get submodules
   
    ```
    git clone --recursive https://github.com/CircuitHappy/MissingLink.git
    ```

    or

    ```
    git clone https://github.com/CircuitHappy/MissingLink.git
    git submodule update --init --recursive
    ```
    
1. Run CMake passing the `CHIP` option as `true` to use cross-compilation toolchain in buildroot directory

    ```
    cd build
    cmake -DCHIP=true ..
    ```
    
1. Build it

    ```
    make
    ```