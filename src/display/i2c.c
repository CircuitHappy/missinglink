#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int i2c_open(unsigned int bus) {
  int fd;
  char filename[16];
  sprintf(filename, "/dev/i2c-%u", bus);
  if ((fd = open(filename, O_RDWR)) < 0) {
    perror("Failed to open the i2c bus");
    exit(1);
  }
  return fd;
}

void i2c_init(int fd, int addr) {
  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
    perror("Failed to acquire i2c bus access");
    exit(1);
  }
}

