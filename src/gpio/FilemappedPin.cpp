#include "gpio/FilemappedPin.hpp"

using namespace MissingLink::GPIO;

const std::string FilemappedPin::sysfsPrefix = "/sys/class/gpio";

FilemappedPin::FilemappedPin(int pinId)
  : m_pinId(pinId)
{}

FilemappedPin::~FilemappedPin() {}

void FilemappedPin::enable() {
  ofstream gpioExport;
  gpioExport.open(FilemappedPin::sysfsPrefix + "/export");
  gpioExport << m_pinId;
  gpioExport.close();
}


//FilemappedOutput::FilemappedOutput(int pinId)
//  : FilemappedPin::FilemappedPin(pinId)
//{}
