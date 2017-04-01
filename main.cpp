#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "missing_link/gpio.hpp"

using namespace std;
using namespace MissingLink;

int main(void) {

  GPIO::SysfsMappedPin blinky(132, GPIO::Pin::OUT);
  if (blinky.Export() < 0) {
    return 1;
  }

  bool value = false;
  while (1) {
    if (blinky.Write(value ? HIGH : LOW) < 0) {
      return 1;
    }
    value = !value;
    this_thread::sleep_for(chrono::milliseconds(50));
  }

  return 0;
}
