missinglink
============

_[NOTE 06/25/2017]: The project is intended to be configured and cross-compiled using the 
procedures documented in [CHIP-SDK](https://github.com/NextThingCo/CHIP-SDK). Hopefully 
[Gadget OS](https://github.com/NextThingCo/gadget-buildroot) is ready/stable for usage in 
the near future as it will likely make this process much easier._

### Bootstrapping

**macOS**

- Install virtualbox and vagrant as per [CHIP-SDK instructions](https://github.com/NextThingCo/CHIP-SDK)

- Create and provision the Ubuntu VM

```
vagrant up
```

- Open a separate terminal window/tab and ssh into the VM

```
vagrant ssh
```

- Configure the project and build toolchain and dependencies via buildroot.
This will probably take a long ass time, so be patient.

```
cd missinglink
bin/configure
```

On the VM, the home directory should now contain the following directories.

```
CHIP-buildroot/
CHIP-tools/
chip-mtd-utils/
missinglink/
sunxi-tools/
```

`missinglink/` is a synced directory mirroring the repository clone on the host machine; changes made in this directory on the host are automatically reflected in the VM and vice versa. This allows source to be edited from within macOS even though cross-compilation is performed on the VM.

Of the other directories, only `CHIP-buildroot` is necessary for cross-compilation. Flashing the CHIP using the other tools can only be done from a native Ubuntu host (not a VM).


**Ubuntu 16.04+**

- Run the provisioning script

```
./setup_ubuntu1604.sh
```

- Configure the project and build toolchain and dependencies via buildroot. 
This will probably take a long ass time, so be patient.

```
bin/configure
```

The project directory should now contain the following directories.

```
CHIP-buildroot/
CHIP-tools/
chip-mtd-utils/
sunxi-tools/
```

For cross-compilation, only `CHIP-buildroot` is necessary. To flash the CHIP with the buildroot image, follow the [CHIP-SDK instructions](https://github.com/NextThingCo/CHIP-SDK).

### Building

**macOS**

- SSH into the VM if necessary and navigate to `missinglink/`. 
_Recommend keeping a separate terminal window/tab/pane open within the VM for this_

```
vagrant ssh
cd missinglink
```

- Build

```
bin/build
```

The built executables are output to `build/bin`.

**Ubuntu 16.04+**

- Build

```
bin/build
```

The built executables are output to `build/bin`.

### Deploying

**Important:** In order for the deploy script to work correctly, you must enable root 
login via SSH on the CHIP. **This is for development purposes only.**

**macOS**

- **From macOS**, run the deploy script. 

_Easier to do from macOS because the VM doesn't support multicast DNS properly. 
If you want to manually specify the CHIP's IP address, you can do this from the VM._

```
bin/deploy
```

This will copy the built executables to the CHIP via scp. The default hostname is
`missinglink.local` but can be overriden via the `CHIP_HOSTNAME` environment variable:

```
CHIP_HOSTNAME=192.168.0.43 bin/deploy
```

**Ubuntu 16.04+**

- Run the deploy script


```
bin/deploy
```

This will copy the built executables to the CHIP via scp. The default hostname is
`missinglink.local` but can be overriden via the `CHIP_HOSTNAME` environment variable:

```
CHIP_HOSTNAME=192.168.0.43 bin/deploy
```
