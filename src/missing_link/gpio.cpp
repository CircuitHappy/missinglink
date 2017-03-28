#include "missing_link/gpio.hpp"

using namespace MissingLink::GPIO;

Pin::Pin(const int address, const Direction direction)
  : m_address(address)
  , m_direction(direction)
{}

int Pin::Write(const DigitalValue value) {
  if (m_direction == IN) {
    return -1;
  }
  return write(value);
}

int Pin::Read(DigitalValue &value) {
  if (m_direction == OUT) {
    return -1;
  }
  return read(value);
}

SysfsMappedPin::SysfsMappedPin(const int address, const Direction direction)
  : Pin(address, direction)
{}

int SysfsMappedPin::Export() {
  return 0;
}

int SysfsMappedPin::Unexport() {
  return 0;
}

int SysfsMappedPin::write(const DigitalValue value) {
  return 0;
}

int SysfsMappedPin::read(DigitalValue &value) {
  return 0;
}

//void Pin::Export() {
//  const string strPath = "/sys/class/gpio/export";
//  FILE *pExp;
//  pExp = fopen(strPath.c_str(), "w");
//  fprintf(pExp, "%d", m_address);
//  fclose(pExp);
//}

