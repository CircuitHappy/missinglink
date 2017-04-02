#include <iostream>
#include <thread>
#include <signal.h>

#include "missing_link/pin_defs.hpp"
#include "missing_link/gpio.hpp"

using namespace std;
using namespace MissingLink;

GPIO::SysfsPin outPin(GPIO::CHIP_DO, GPIO::Pin::OUT);
GPIO::SysfsPin inPin(GPIO::CHIP_D1, GPIO::Pin::IN);

void signalHandler(int s) {
  cout << "Caught signal " << s << endl;
  outPin.Unexport();
  inPin.Unexport();
  exit(1);
}

void configureSignalHandler() {
  struct ::sigaction sigIntHandler;
  sigIntHandler.sa_handler = signalHandler;
  ::sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  ::sigaction(SIGINT, &sigIntHandler, NULL);
  ::sigaction(SIGSEGV, &sigIntHandler, NULL);
}

int main(void) {

  configureSignalHandler();

  // Unexport every time first (don't care if it fails)
  // to clean up any other process state
  inPin.Unexport();
  outPin.Unexport();

  inPin.Export();
  outPin.Export();

  GPIO::Toggle toggle(inPin);
  while (1) {
    toggle.Process();
    if (outPin.Write(toggle.GetState() ? HIGH : LOW) < 0) {
      std::cerr << "Failed to write pin" << std::endl;
    }
    this_thread::sleep_for(chrono::milliseconds(50));
  }

  return 0;
}
