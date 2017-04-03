#include <iostream>
#include <cstring>
#include "missing_link/gpio.hpp"

using std::string;
using namespace MissingLink;
using namespace MissingLink::GPIO;

Pin::Pin(const int address, const Direction direction)
  : m_address(address)
  , m_direction(direction)
{}

Pin::~Pin() {}

int Pin::Write(const DigitalValue value) const {
  if (m_direction == IN) {
    return -1;
  }
  return write(value);
}

int Pin::Read(DigitalValue *value) const {
  if (m_direction == OUT) {
    return -1;
  }
  return read(value);
}

//=======================

const string SysfsPin::s_rootInterfacePath = "/sys/class/gpio";

SysfsPin::SysfsPin(const int address, const Direction direction)
  : Pin(address, direction)
{}

int SysfsPin::Export() {
  if (doExport() < 0) { return -1; }
  if (doSetDirection() < 0) { return -1; }
  File::Access mode = (m_direction == IN) ? File::READ : File::WRITE;
  m_valueFile = createFile(getPinInterfacePath() + "/value", mode);
  if (m_valueFile->Open() < 0) {
    return -1;
  };
  return 0;
}

int SysfsPin::Unexport() {
  if (m_valueFile.get()) {
    m_valueFile->Close();
  }
  m_valueFile.reset(nullptr);
  if (doUnexport() < 0) {
    return -1;
  }
  return 0;
}

int SysfsPin::write(const DigitalValue value) const {
  if (!m_valueFile.get()) {
    std::cerr << "Cannot write to pin " << m_address << ". Did you Export()?" << std::endl;
    return -1;
  }
  return m_valueFile->Write(value == HIGH ? "1" : "0");
}

int SysfsPin::read(DigitalValue *value) const {
  if (!m_valueFile.get()) {
    std::cerr << "Cannot read from pin " << m_address << ". Did you Export()?" << std::endl;
    return -1;
  }
  if (m_valueFile->Seek(0) < 0) {
    return -1;
  }
  char cValue;
  if (m_valueFile->Read(&cValue, 1) < 0) {
    return -1;
  }
  *value = cValue == '0' ? LOW : HIGH;
  return 0;
}

int SysfsPin::doExport() {
  const string strExportPath = s_rootInterfacePath + "/export";
  auto exportFile = createFile(strExportPath, File::WRITE);
  if (exportFile->Open() < 0) {
    return -1;
  }
  int result = exportFile->Write(std::to_string(m_address));
  exportFile->Close();
  return result;
}

int SysfsPin::doUnexport() {
  const string strUnexportPath = s_rootInterfacePath + "/unexport";
  auto file = createFile(strUnexportPath, File::WRITE);
  if (file->Open() < 0) {
    return -1;
  }
  int result = file->Write(std::to_string(m_address));
  file->Close();
  return result;
}

int SysfsPin::doSetDirection() {
  const string strDirectionPath = getPinInterfacePath() +  "/direction";
  auto file = createFile(strDirectionPath, File::WRITE);
  if (file->Open() < 0) {
    return -1;
  }
  int result = file->Write(m_direction == IN ? "in" : "out");
  file->Close();
  return result;
}

string SysfsPin::getPinInterfacePath() const {
  return s_rootInterfacePath + "/gpio" + std::to_string(m_address);
}

std::unique_ptr<File> SysfsPin::createFile(const string strPath, const File::Access access) {
  return std::unique_ptr<File>(new MissingLink::UnbufferedFile(strPath, access));
}


//======================


Control::Control(const Pin &pin, DigitalValue trigger)
  : m_pin(pin)
  , m_trigger(trigger)
{}

Control::~Control() {}

void Control::Process() {
  process();
}


//======================


Button::Button(const Pin &pin, DigitalValue trigger)
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


Toggle::Toggle(const Pin &pin, DigitalValue trigger)
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
