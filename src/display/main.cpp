#include <iostream>
#include "display/oled.h"

using namespace std;

int main(void) {
  oled_init();
  oled_write(0, "MISSING LINK");
  oled_write(1, "v0.1");
  return 0;
}

