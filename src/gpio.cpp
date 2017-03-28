#include <string>
#include <stdio.h>
#include "gpio.hpp"

using namespace std;
using namespace MissingLink::GPIO;

Pin::Pin(int address)
  : m_address(address)
{}

void Pin::Export() {
  const string strPath = "/sys/class/gpio/export";
  FILE *pExp;
  pExp = fopen(strPath.c_str(), "w");
  fprintf(pExp, "%d", m_address);
  fclose(pExp);
}

void Pin::Unexport() {

}

void Pin::SetDirection(Direction direction) {

}

void Pin::Write(DigitalValue value) {

}

DigitalValue Pin::Read() {
  return GPIO::LOW;
}
