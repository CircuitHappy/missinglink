#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "gpio.hpp"

using namespace std;
using namespace MissingLink;

int main(void) {

  GPIO::Pin blinky(132);
  blinky.Export();
  blinky.SetDirection(GPIO::OUT);

  bool value = false;
  while (1) {
    blinky.Write(value ? GPIO::HIGH : GPIO::LOW);
    value = !value;
    this_thread::sleep_for(chrono::seconds(1));
  }

  //ofstream gpioExport;
  //gpioExport.open("/sys/class/gpio/export");
  //gpioExport << "132";
  //gpioExport.close();

  //ofstream pinDirection;
  //pinDirection.open("/sys/class/gpio/gpio132/direction");
  //pinDirection << "out";
  //pinDirection.close();

  //ofstream pinValue;
  //pinValue.open("/sys/class/gpio/gpio132/value");

  //bool value = false;
  //while (1) {
  //  pinValue << (value ? "1" : "0");
  //  pinValue.flush();
  //  value = !value;
  //  this_thread::sleep_for(chrono::seconds(1));
  //}

  return 0;
}
