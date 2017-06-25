/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

static int i2c_open(unsigned int bus) {
  int fd;
  char filename[16];
  sprintf(filename, "/dev/i2c-%u", bus);
  if ((fd = open(filename, O_RDWR)) < 0) {
    perror("Failed to open the i2c bus");
    exit(1);
  }
  return fd;
}

static void i2c_init(int fd, int addr) {
  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
    perror("Failed to acquire i2c bus access");
    exit(1);
  }
}

void i2c_write(int bus, int addr, const unsigned char *data, size_t nbytes) {
  int fd = i2c_open(bus);
  i2c_init(fd, addr);
  if (write(fd, data, nbytes) != nbytes) {
    perror("Failed to write to i2c device");
    exit(1);
  }
  close(fd);
}
