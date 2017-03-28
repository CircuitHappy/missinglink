#include "gpio.hpp"

using namespace MissingLink::GPIO;

Pin::Pin(const int address, const Direction direction)
  : m_address(address)
  , m_direction(direction)
{}


int Pin::Write(const DigitalValue value) {
  return write(value);
}

int Pin::Read(DigitalValue &value) {
  return read(value);
}

//void Pin::Export() {
//  const string strPath = "/sys/class/gpio/export";
//  FILE *pExp;
//  pExp = fopen(strPath.c_str(), "w");
//  fprintf(pExp, "%d", m_address);
//  fclose(pExp);
//}

//void Pin::Unexport() {

//}
