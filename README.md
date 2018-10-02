# Setting up and building on a Raspberry Pi Zero W
## Burn microSD card
Get the latest OS here:
https://www.raspberrypi.org/downloads/raspbian/
## Edit /boot files
Mount the SD card on your computer desktop to edit the boot volume

### Enabling SSH
Create an empty text file named ‘ssh’ at the root of `/boot`

### Enabling UART console
`sudo vi /boot/config.txt`

At the bottom, last line, add enable_uart=1 on it's own line

### Enable i2c
In the same file, `/boot/config.txt`

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

## Get Missing Link building
`sudo apt-get install cmake git libconfig++-dev libconfig++9v5`

Clone the git repo on the pi

`git clone https://github.com/CircuitHappy/missinglink.git`

`sudo cmake -Bbuild -H.`

`sudo make -C build`

`sudo build/bin/missing_link`

Run the binary `sudo ./build/bin/missing_link`
