#include <string>
#include <stdio.h>
#include "missing_link/gpio.hpp"

using std::string;
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
  const string strExportPath = "/sys/class/gpio/export";
  if (writeToFile(strExportPath, std::to_string(m_address)) != 0) {
    return -1;
  }

  const string strDirectionPath = getInterfacePath() + "/direction";
  if (writeToFile(strDirectionPath, m_direction == IN ? "in" : "out") != 0) {
    return -1;
  };

  return 0;
}

int SysfsMappedPin::Unexport() {
  const string strUnexportPath = "/sys/class/gpio/unexport";
  if (writeToFile(strUnexportPath, std::to_string(m_address)) != 0) {
    return -1;
  }
  return 0;
}

int SysfsMappedPin::write(const DigitalValue value) {
  const string strValuePath = getInterfacePath() + "/value";
  if (writeToFile(strValuePath, std::to_string(value)) != 0) {
    return -1;
  };
  return 0;
}

int SysfsMappedPin::read(DigitalValue &value) {
  const string strValuePath = getInterfacePath() + "/value";
  string result;
  if (readFromFile(strValuePath, result) != 0) {
    return -1;
  };

  if (result == "1") {
    value = HIGH;
  } else if (result == "0") {
    value = LOW;
  } else {
    return -1;
  }

  return 0;
}

string SysfsMappedPin::getInterfacePath() const {
  return "/sys/class/gpio/gpio" + std::to_string(m_address);
}

int SysfsMappedPin::writeToFile(const string strPath, const string strValue) {
  FILE *fd = NULL;
  if ((fd = fopen(strPath.c_str(), "w")) == NULL) {
    return -1;
  }
  if (fprintf(fd, "%s", strValue.c_str()) < 0) {
    return -1;
  }
  if (fclose(fd) != 0) {
    return -1;
  }
  return 0;
}

int SysfsMappedPin::readFromFile(const string strPath, string &strValue) {
  return -1;
}
