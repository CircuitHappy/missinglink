#include <iostream>
#include "i2c.h"

using namespace std;

int main(void) {
  int fd = i2c_open(1);
  i2c_init(fd, 0x3c);
  return 0;
}

