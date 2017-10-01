/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <string>
#include <memory>

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

    Pin(int address, Direction direction, DigitalValue initial = LOW);
    virtual ~Pin();

    int Export();
    int Unexport();

    int Read(DigitalValue *value);
    int Write(DigitalValue value);

  protected:

    static const std::string s_rootInterfacePath;

    const int m_address;
    const Direction m_direction;
    const DigitalValue m_initialValue;
    const std::string m_pinInterfacePath;

    int read(DigitalValue *value);
    int write(DigitalValue value);

    int doExport();
    int doUnexport();
    int doSetDirection();

    virtual int writeToFile(const std::string &strPath, const void *buf, size_t nBytes);
    virtual int readFromFile(const std::string &strPath, void *buf, size_t nBytes);

  private:

    DigitalValue m_lastValueWritten;

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
