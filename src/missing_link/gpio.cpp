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


SysfsPin::SysfsPin(const int address, const Direction direction)
  : Pin(address, direction)
{}

int SysfsPin::Export() {
  if (doExport() < 0) { return -1; }
  if (doSetDirection() < 0) { return -1; }
  File::Access mode = (m_direction == IN) ? File::READ : File::WRITE;
  m_valueFile = createFile(getInterfacePath() + "/value", mode);
  if (m_valueFile->Open() < 0) {
    return -1;
  };
  return 0;
}

int SysfsPin::Unexport() {
  m_valueFile->Close();
  m_valueFile.reset(nullptr);
  if (doUnexport() < 0) {
    return -1;
  }
  return 0;
}

int SysfsPin::write(const DigitalValue value) {
  if (!m_valueFile.get()) {
    std::cerr << "Cannot write to pin " << m_address << ". Did you Export()?" << std::endl;
    return -1;
  }
  return m_valueFile->Write(value == HIGH ? "1" : "0");
}

int SysfsPin::read(DigitalValue &value) {
  if (!m_valueFile.get()) {
    std::cerr << "Cannot read from pin " << m_address << ". Did you Export()?" << std::endl;
    return -1;
  }
  char buf[16];
  if (m_valueFile->Read(buf, 16) < 0) {
    return -1;
  }
  value = ::strcmp(buf, "0") == 0 ? LOW : HIGH;
  return 0;
}

int SysfsPin::doExport() {
  const string strExportPath = "/sys/class/gpio/export";
  auto exportFile = createFile(strExportPath, File::WRITE);
  if (exportFile->Open() < 0) {
    return -1;
  }
  if (exportFile->Write(std::to_string(m_address)) < 0) {
    return -1;
  }
  exportFile->Close();
  return 0;
}

int SysfsPin::doUnexport() {
  const string strUnexportPath = "/sys/class/gpio/unexport";
  auto file = createFile(strUnexportPath, File::WRITE);
  if (file->Open() < 0) {
    return -1;
  }
  if (file->Write(std::to_string(m_address)) < 0) {
    return -1;
  }
  file->Close();
  return 0;
}

int SysfsPin::doSetDirection() {
  const string strDirectionPath = getInterfacePath() +  "/direction";
  auto file = createFile(strDirectionPath, File::WRITE);
  if (file->Open() < 0) {
    return -1;
  }
  if (file->Write(m_direction == IN ? "in" : "out") < 0) {
    return -1;
  }
  file->Close();
  return 0;
}

string SysfsPin::getInterfacePath() const {
  return "/sys/class/gpio/gpio" + std::to_string(m_address);
}

std::unique_ptr<File> SysfsPin::createFile(const string strPath, const File::Access access) {
  return std::unique_ptr<File>(new MissingLink::UnbufferedFile(strPath, access));
}
