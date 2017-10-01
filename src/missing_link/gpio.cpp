/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "missing_link/gpio.hpp"

using std::string;
using namespace MissingLink;
using namespace MissingLink::GPIO;

const string Pin::s_rootInterfacePath = "/sys/class/gpio";

Pin::Pin(int address, Direction direction, DigitalValue initial)
  : m_address(address)
  , m_direction(direction)
  , m_initialValue(initial)
  , m_pinInterfacePath(s_rootInterfacePath + "/gpio" + std::to_string(address))
{}

Pin::~Pin() {}

int Pin::Write(DigitalValue value) {
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

int Pin::Export() {
  if (doExport() < 0) { return -1; }
  if (doSetDirection() < 0) { return -1; }
  if (m_direction == OUT) {
    return write(m_initialValue);
  }
  return 0;
}

int Pin::Unexport() {
  return doUnexport();
}

int Pin::write(DigitalValue value) {
  auto strValuePath = m_pinInterfacePath + "/value";
  const char * charValue = value == HIGH ? "1" : "0";
  return writeToFile(strValuePath, charValue, 1);
}

int Pin::read(DigitalValue *value) {
  auto strValuePath = m_pinInterfacePath + "/value";
  char buf[2];
  if (readFromFile(strValuePath, buf, 1) < 0) {
    return -1;
  }
  *value = std::strcmp(buf, "1") == 0 ? HIGH : LOW;
  return 0;
}

int Pin::writeToFile(const std::string &strPath, const void *buf, size_t nBytes) {
  int fd = ::open(strPath.c_str(), O_WRONLY);
  if (fd < 0) { return -1; }
  int result = ::write(fd, buf, nBytes) == (int)nBytes ? 0 : -1;
  if (result < 0) {
    std::cerr << "Error writing to value file\n";
  }
  ::close(fd);
  return result;
}

int Pin::readFromFile(const std::string &strPath, void *buf, size_t nBytes) {
  int fd = ::open(strPath.c_str(), O_RDONLY);
  if (fd < 0) { return -1; }
  int result = ::read(fd, buf, nBytes) == (int)nBytes ? 0 : -1;
  ::close(fd);
  return result;
}

int Pin::doExport() {
  auto strExportPath = s_rootInterfacePath + "/export";
  auto strAddress = std::to_string(m_address);
  return writeToFile(strExportPath, strAddress.c_str(), strAddress.size());
}

int Pin::doUnexport() {
  auto strUnexportPath = s_rootInterfacePath + "/unexport";
  auto strAddress = std::to_string(m_address);
  return writeToFile(strUnexportPath, strAddress.c_str(), strAddress.size());
}

int Pin::doSetDirection() {
  auto strDirectionPath = m_pinInterfacePath +  "/direction";
  string strDirection = m_direction == IN ? "in" : "out";
  return writeToFile(strDirectionPath, strDirection.c_str(), strDirection.size());
}

//======================

static inline void i2c_smbus_transaction(int fd, char rw, uint8_t regAddr, union i2c_smbus_data *data, int size) {
  i2c_smbus_ioctl_data args;
  args.read_write = rw;
  args.command = regAddr;
  args.size = size;
  args.data = data;
  if (::ioctl(fd, I2C_SMBUS, &args)) {
    std::cerr << "[ERROR] i2c SMBUS failed" << std::endl;
  }
}

I2CDevice::I2CDevice(uint8_t bus, uint8_t devAddr) : m_fd(-1) {
  open(bus, devAddr);
}

I2CDevice::~I2CDevice() {
  close();
}

uint8_t I2CDevice::ReadByte(uint8_t regAddr) {
  i2c_smbus_data data;
  i2c_smbus_transaction(m_fd, I2C_SMBUS_READ, regAddr, &data, 2);
  return data.byte;
}

void I2CDevice::WriteByte(uint8_t regAddr, uint8_t value) {
  i2c_smbus_data data;
  data.byte = value;
  i2c_smbus_transaction(m_fd, I2C_SMBUS_WRITE, regAddr, &data, 2);
}

void I2CDevice::open(uint8_t bus, uint8_t devAddr) {
  string interface = "/dev/i2c-" + std::to_string(bus);
  m_fd = ::open(interface.c_str(), O_RDWR);
  if (::ioctl(m_fd, I2C_SLAVE, devAddr)) {
    std::cerr << "[ERROR] Could not open I2C interface" << std::endl;
  }
}

void I2CDevice::close() {
  if (m_fd >= 0) {
    ::close(m_fd);
    m_fd = -1;
  }
}
