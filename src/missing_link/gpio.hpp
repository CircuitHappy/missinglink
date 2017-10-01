/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <string>
#include <memory>
#include <poll.h>

#include "missing_link/common.h"

namespace MissingLink {
namespace GPIO {

enum DigitalValue {
  LOW = 0,
  HIGH = 1
};

class Pin {

  public:

    enum Direction {
      IN = 0,
      OUT = 1
    };

    enum Edge {
      RISING,
      FALLING,
      BOTH
    };

    Pin(int address, Direction direction);
    virtual ~Pin();

    DigitalValue Read();
    void Write(DigitalValue value);

    void SetEdgeMode(Edge edge);

    pollfd GetPollInfo();

  protected:

    static const std::string s_rootInterfacePath;

    const int m_address;
    const Direction m_direction;
    const std::string m_pinInterfacePath;

    int m_fd;

    void open();
    void close();

    DigitalValue read();
    void write(DigitalValue value);

    int writeToFile(const std::string &strPath, const std::string &strValue);
    int readFromFile(const std::string &strPath, void *buf, int nBytes);
};


class I2CDevice {

  public:

    I2CDevice(uint8_t bus, uint8_t devAddr);
    virtual ~I2CDevice();

    uint8_t ReadByte(uint8_t regAddr);
    void WriteByte(uint8_t regAddr, uint8_t value);

  private:

    int m_fd;

    void open(uint8_t bus, uint8_t devAddr);
    void close();
};

}} // namespaces
