#include <iostream>
#include "i2c.h"

using namespace std;

int main(void) {
  unsigned char command[2] = {0x00, 0x01};
  i2c_write(1, 0x3c, command, 2);
  return 0;
}

