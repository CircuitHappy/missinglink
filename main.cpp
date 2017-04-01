#include <iostream>
#include <thread>
#include <signal.h>

#include "missing_link/gpio.hpp"

using namespace std;
using namespace MissingLink;

GPIO::SysfsMappedPin blinky(132, GPIO::Pin::OUT);

void signalHandler(int s) {
  cout << "Caught signal " << s << endl;
  blinky.Unexport();
  exit(1);
}

void configureInterruptHandler() {
  struct ::sigaction sigIntHandler;
  sigIntHandler.sa_handler = signalHandler;
  ::sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  ::sigaction(SIGINT, &sigIntHandler, NULL);
}

int main(void) {

  configureInterruptHandler();

  blinky.Export();

  bool value = false;
  while (1) {
    blinky.Write(value ? HIGH : LOW);
    value = !value;
    this_thread::sleep_for(chrono::milliseconds(50));
  }

  return 0;
}
