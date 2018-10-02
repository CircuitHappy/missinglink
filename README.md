# Setting up and building on a Raspberry Pi Zero W
## Burn microSD card
Get the latest OS here:
https://www.raspberrypi.org/downloads/raspbian/
## Edit /boot files
Mount the SD card on your computer. You should see a "boot" volume

### Enabling SSH
Create an empty text file named ‘ssh’ at the root of `Volumes/boot`

### Enabling UART console
`sudo vi /Volumes/boot/config.txt`

At the bottom, last line, add enable_uart=1 on it's own line

### Enable i2c
In the same file, `/Volumes/boot/config.txt`

Uncomment the line that contains `dtparam=i2c_arm=on`

### Pre-configure WiFi Network
Create a file named `wpa_supplicant.conf` at the root of `/boot`

The contents of this file should look like this:

`country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
network={
      ssid="viznet2"
      psk="4646464646"
      key_mgmt=WPA-PSK
}`

## Boot your RPi Zero W
Put the microSD card in the RPi Zero W's reader and power on the RPi.

SSH in to your RPi via your terminal `ssh pi@raspberrypi.local`

Password should be the default password for the `pi` user.

## Get Missing Link building
Install these files to get all the tools you will need to build and run the `missing_link` binary.

`sudo apt-get install cmake git libconfig++-dev libconfig++9v5`

Clone the git repo in your home directory

`cd ~/`

`git clone https://github.com/CircuitHappy/missinglink.git`

`sudo cmake -Bbuild -H.`

`sudo make -C build`

`sudo build/bin/missing_link`

Run the binary `sudo ./build/bin/missing_link`

*Note: missing_link binary is expecting to talk to an LED driver, LED display, and GPIO expander over the i2c buss. You will have to disable some of these dependencies if you don't have those wired up to the RPi Zero W.*
