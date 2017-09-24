#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#include "missing_link/gpio.hpp"

using std::string;
using namespace MissingLink;
using namespace MissingLink::GPIO;

Pin::Pin(const int address, const Direction direction, const DigitalValue initial)
  : m_address(address)
  , m_direction(direction)
  , m_initialValue(initial)
{}

Pin::~Pin() {}

int Pin::Write(const DigitalValue value) {
  if (m_direction == IN) {
    return -1;
  }
  if (m_lastValueWritten == value) {
    return 0;
  }
  int result = write(value);
  if (result == 0) {
    m_lastValueWritten = value;
  }
  return result;
}

int Pin::Read(DigitalValue *value) {
  if (m_direction == OUT) {
    return -1;
  }
  return read(value);
}

//=======================

const string SysfsPin::s_rootInterfacePath = "/sys/class/gpio";

SysfsPin::SysfsPin(const int address, const Direction direction)
  : Pin(address, direction)
  , m_pinInterfacePath(s_rootInterfacePath + "/gpio" + std::to_string(address))
{}

int SysfsPin::Export() {
  if (doExport() < 0) { return -1; }
  if (doSetDirection() < 0) { return -1; }
  if (m_direction == OUT) {
    return write(m_initialValue);
  }
  return 0;
}

int SysfsPin::Unexport() {
  return doUnexport();
}

int SysfsPin::write(const DigitalValue value) {
  auto strValuePath = m_pinInterfacePath + "/value";
  const char * charValue = value == HIGH ? "1" : "0";
  return writeToFile(strValuePath, charValue, 1);
}

int SysfsPin::read(DigitalValue *value) {
  auto strValuePath = m_pinInterfacePath + "/value";
  char buf[2];
  if (readFromFile(strValuePath, buf, 1) < 0) {
    return -1;
  }
  *value = std::strcmp(buf, "1") == 0 ? HIGH : LOW;
  return 0;
}

int SysfsPin::writeToFile(const std::string &strPath, const void *buf, const size_t nBytes) {
  const int fd = ::open(strPath.c_str(), O_WRONLY);
  if (fd < 0) { return -1; }
  const int result = ::write(fd, buf, nBytes) == (int)nBytes ? 0 : -1;
  if (result < 0) {
    std::cerr << "Error writing to value file\n";
  }
  ::close(fd);
  return result;
}

int SysfsPin::readFromFile(const std::string &strPath, void *buf, const size_t nBytes) {
  const int fd = ::open(strPath.c_str(), O_RDONLY);
  if (fd < 0) { return -1; }
  const int result = ::read(fd, buf, nBytes) == (int)nBytes ? 0 : -1;
  ::close(fd);
  return result;
}

int SysfsPin::doExport() {
  auto strExportPath = s_rootInterfacePath + "/export";
  auto strAddress = std::to_string(m_address);
  return writeToFile(strExportPath, strAddress.c_str(), strAddress.size());
}

int SysfsPin::doUnexport() {
  auto strUnexportPath = s_rootInterfacePath + "/unexport";
  auto strAddress = std::to_string(m_address);
  return writeToFile(strUnexportPath, strAddress.c_str(), strAddress.size());
}

int SysfsPin::doSetDirection() {
  auto strDirectionPath = m_pinInterfacePath +  "/direction";
  string strDirection = m_direction == IN ? "in" : "out";
  return writeToFile(strDirectionPath, strDirection.c_str(), strDirection.size());
}

//======================


Control::Control(Pin &pin, DigitalValue trigger)
  : m_pin(pin)
  , m_trigger(trigger)
{}

Control::~Control() {}

void Control::Process() {
  process();
}


//======================


Button::Button(Pin &pin, DigitalValue trigger)
  : Control(pin, trigger)
  , m_event(NONE)
  , m_lastValue(trigger == HIGH ? LOW : HIGH)
{}

bool Button::IsPressed() const {
  return m_lastValue == m_trigger;
}

Button::Event Button::GetCurrentEvent() const {
  return m_event;
}

void Button::process() {
  DigitalValue value;
  if (m_pin.Read(&value) < 0) {
    return;
  }
  if (value != m_lastValue) {
    m_event = (value == m_trigger) ? TOUCH_DOWN : TOUCH_UP;
  } else {
    m_event = NONE;
  }
  m_lastValue = value;
}


//======================


Toggle::Toggle(Pin &pin, DigitalValue trigger)
  : Control(pin, trigger)
  , m_state(false)
  , m_lastValue(trigger == HIGH ? LOW : HIGH)
{}

void Toggle::process() {
  DigitalValue value;
  if (m_pin.Read(&value) < 0) {
    return;
  }
  if (value == m_trigger && value != m_lastValue) {
    m_state = !m_state;
  }
  m_lastValue = value;
}

bool Toggle::GetState() const {
  return m_state;
}
