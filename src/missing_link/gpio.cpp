#include <iostream>
#include <string>
#include "missing_link/gpio.hpp"

extern "C" {
  #include <unistd.h>
  #include <fcntl.h>
}

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
  int fd = -1;
  if ((fd = open(strPath.c_str(), O_WRONLY)) < 0) {
    std::cerr << "Failed to open " << strPath << std::endl;
    return -1;
  }
  size_t bytes = strValue.length();
  int writtenBytes = ::write(fd, strValue.c_str(), bytes);
  if (writtenBytes != (int)bytes)  {
    std::cerr << "Failed to write " << strValue << " to " << strPath << std::endl;
    return -1;
  }
  if (close(fd) != 0) {
    std::cerr << "Failed to close " << strPath << std::endl;
    return -1;
  }
  return 0;
}

int SysfsMappedPin::readFromFile(const string strPath, string &strValue) {
  return -1;
}
