/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "missing_link/gpio.hpp"

using std::string;
using namespace MissingLink;
using namespace MissingLink::GPIO;

const string Pin::s_rootInterfacePath = "/sys/class/gpio";

Pin::Pin(int address, Direction direction)
  : m_address(address)
  , m_direction(direction)
  , m_pinInterfacePath(s_rootInterfacePath + "/gpio" + std::to_string(address))
  , m_fd(-1)
{
  open();
}

Pin::~Pin() {
  close();
}

void Pin::Write(DigitalValue value) {
  if (m_direction == IN) {
    std::cerr << "Attempt to write to input pin at " + m_address << std::endl;
    return;
  }
  write(value);
}

DigitalValue Pin::Read() {
  if (m_direction == OUT) {
    std::cerr << "Attempt to read from output pin at " + m_address << std::endl;
    return DigitalValue::LOW;
  }
  return read();
}

void Pin::SetEdgeMode(Edge edge) {
  if (m_direction == OUT) {
    std::cerr << "Attempt to set edge mode for output pin at " + m_address << std::endl;
    return;
  }
  // toggle back to none first to fix buggy driver state
  string strEdgePath = m_pinInterfacePath + "/edge";
  writeToFile(strEdgePath, "none");

  string strEdge;
  switch (edge) {
    case RISING:
      strEdge = "rising";
      break;
    case FALLING:
      strEdge = "falling";
      break;
    case BOTH:
      strEdge = "both";
      break;
    default:
      return;
  }
  int result;
  if ((result = writeToFile(strEdgePath, strEdge)) != 0) {
    std::cerr << "Failed to set edge mode for pin: " << std::strerror(result) << std::endl;
  }
}

pollfd Pin::GetPollInfo() {
  return { m_fd, POLLPRI, 0 };
}

void Pin::write(DigitalValue value) {
  if (m_fd < 0) {
    return;
  }
  string strValue = value == HIGH ? "1" : "0";
  ::write(m_fd, strValue.c_str(), strValue.size());
}

DigitalValue Pin::read() {
  if (m_fd < 0) {
    return LOW;
  }
  auto strValuePath = m_pinInterfacePath + "/value";
  char ch;
  ::lseek(m_fd, 0, SEEK_SET);
  ::read(m_fd, &ch, 1);
  return (ch == '1') ? HIGH : LOW;
}

int Pin::writeToFile(const std::string &strPath, const std::string &strValue) {
  int fd = ::open(strPath.c_str(), O_WRONLY);
  if (fd < 0) {
    return errno;
  }
  int size = strValue.size();
  int result = ::write(fd, strValue.c_str(), size);
  ::close(fd);
  return (result == size) ? 0 : errno;
}

int Pin::readFromFile(const std::string &strPath, void *buf, int nBytes) {
  int fd = ::open(strPath.c_str(), O_RDONLY);
  if (fd < 0) {
    return errno;
  }
  int result = ::read(fd, buf, nBytes);
  ::close(fd);
  return (result == nBytes) ? 0 : errno;
}

void Pin::open() {
  int result;
  auto strExportPath = s_rootInterfacePath + "/export";
  auto strAddress = std::to_string(m_address);
  if((result = writeToFile(strExportPath, strAddress)) != 0) {
    if (errno != EBUSY) {
      std::cerr << "Failed to export interface for pin: " << std::strerror(result) << std::endl;
    }
  }

  auto strDirectionPath = m_pinInterfacePath +  "/direction";
  string strDirection = m_direction == IN ? "in" : "out";
  if((result = writeToFile(strDirectionPath, strDirection)) != 0) {
    std::cerr << "Failed to set direction for pin: " << std::strerror(result) << std::endl;
  }

  auto strValuePath = m_pinInterfacePath + "/value";
  if ((m_fd = ::open(strValuePath.c_str(), O_RDWR)) < 0) {
    std::cerr << "Failed to open value interface for pin: " << std::strerror(errno) << std::endl;
  }
}

void Pin::close() {
  if (m_fd >= 0) {
    ::close(m_fd);
    m_fd = -1;
  }
  auto strUnexportPath = s_rootInterfacePath + "/unexport";
  auto strAddress = std::to_string(m_address);
  writeToFile(strUnexportPath, strAddress);
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
