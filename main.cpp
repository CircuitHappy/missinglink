#include <iostream>
#include <thread>
#include <signal.h>

#include "missing_link/gpio.hpp"

using namespace std;
using namespace MissingLink;

GPIO::SysfsPin outPin(132, GPIO::Pin::OUT);
GPIO::SysfsPin inPin(133, GPIO::Pin::IN);

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

  bool value = false;
  while (1) {
    DigitalValue inValue = LOW;
    if (inPin.Read(&inValue) < 0){
      std::cerr << "Failed to read pin" << std::endl;
    }
    if (inValue == HIGH) {
      if (outPin.Write(value ? HIGH : LOW) < 0) {
        std::cerr << "Failed to write pin" << std::endl;
      }
      value = !value;
    }
    this_thread::sleep_for(chrono::milliseconds(50));
  }


  return 0;
}
