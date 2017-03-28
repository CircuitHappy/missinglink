#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "missing_link/gpio.hpp"

using namespace std;
using namespace MissingLink;

int main(void) {

  GPIO::SysfsMappedPin blinky(132, GPIO::Pin::OUT);
  blinky.Export();

  bool value = false;
  while (1) {
    blinky.Write(value ? HIGH : LOW);
    value = !value;
    this_thread::sleep_for(chrono::milliseconds(50));
  }

  return 0;
}
