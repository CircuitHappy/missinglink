#!/bin/bash

echo -e "\n Setting up environment in Ubuntu 16.04"
sudo apt-get -y update
sudo apt-get -y install \
 build-essential \
 git \
 mercurial \
 cmake \
 bc \
 curl \
 screen \
 unzip \
 device-tree-compiler \
 libncurses-dev \
 ppp \
 cu \
 linux-image-extra-virtual \
 u-boot-tools \
 android-tools-fastboot \
 android-tools-fsutils \
 python-dev \
 python-pip \
 libusb-1.0-0-dev \
 g++-arm-linux-gnueabihf \
 pkg-config \
 libacl1-dev \
 zlib1g-dev \
 liblzo2-dev \
 uuid-dev

if uname -a |grep -q 64;
then
  echo -e "\n Installing 32bit compatibility libraries"
  sudo apt-get -y install libc6-i386 lib32stdc++6 lib32z1
fi

echo -e "\n Adding current user to dialout group"
sudo usermod -a -G dialout $(logname)

echo -e "\n Adding current user to plugdev group"
sudo usermod -a -G plugdev $(logname)


echo -e "\n Adding udev rule for Allwinner device"
echo -e 'SUBSYSTEM=="usb", ATTRS{idVendor}=="1f3a", ATTRS{idProduct}=="efe8", GROUP="plugdev", MODE="0660" SYMLINK+="usb-chip"
SUBSYSTEM=="usb", ATTRS{idVendor}=="18d1", ATTRS{idProduct}=="1010", GROUP="plugdev", MODE="0660" SYMLINK+="usb-chip-fastboot"
SUBSYSTEM=="usb", ATTRS{idVendor}=="1f3a", ATTRS{idProduct}=="1010", GROUP="plugdev", MODE="0660" SYMLINK+="usb-chip-fastboot"
SUBSYSTEM=="usb", ATTRS{idVendor}=="067b", ATTRS{idProduct}=="2303", GROUP="plugdev", MODE="0660" SYMLINK+="usb-serial-adapter"
' | sudo tee /etc/udev/rules.d/99-allwinner.rules
sudo udevadm control --reload-rules

echo -e "\n Installing sunxi-tools"
if [ -d sunxi-tools ]; then
  rm -rf sunxi-tools
fi
git clone http://github.com/linux-sunxi/sunxi-tools
pushd sunxi-tools
make
make misc
SUNXI_TOOLS=(sunxi-bootinfo
sunxi-fel
sunxi-fexc
sunxi-nand-part
sunxi-pio
pheonix_info
sunxi-nand-image-builder)
for BIN in ${SUNXI_TOOLS[@]};do
  if [[ -L /usr/local/bin/${BIN} ]]; then
    sudo rm /usr/local/bin/${BIN}
  fi
  sudo ln -s $PWD/${BIN} /usr/local/bin/${BIN}
done
popd

git clone http://github.com/nextthingco/chip-mtd-utils
pushd chip-mtd-utils
git checkout by/1.5.2/next-mlc-debian
make
sudo make install
popd

echo -e "\n Installing CHIP-tools"
if [ -d CHIP-tools ]; then
  pushd CHIP-tools
  git pull
  popd
fi
git clone https://github.com/NextThingCo/CHIP-tools.git

echo -e "\n Installing CHIP-buildroot"
if [ ! -d CHIP-buildroot ]; then
  git clone --branch "missing-link" http://github.com/CircuitHappy/CHIP-buildroot
else
  pushd CHIP-buildroot
  git pull
  popd
fi

if [ $(echo $PWD | grep ubuntu) ];then
  sudo chown -R ubuntu:ubuntu *
fi
