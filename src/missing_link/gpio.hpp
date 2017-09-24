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

// BELOW THIS LINE IS DEPRECATED AND WILL BE REMOVED

class Control {
public:
  Control(Pin &pin, DigitalValue trigger = HIGH);
  virtual ~Control();
  void Process();
protected:
  Pin &m_pin;
  const DigitalValue m_trigger;
  virtual void process() = 0;
};


class Button : public Control {
public:

  enum Event {
    NONE,
    TOUCH_DOWN,
    TOUCH_UP
  };

  // Caller is responsible for managing pin initialization
  Button(Pin &pin, DigitalValue trigger = HIGH);

  // Returns true while button is pressed
  bool IsPressed() const;

  Event GetCurrentEvent() const;

protected:
  Event m_event;
  DigitalValue m_lastValue;
  void process() override;
};


class Toggle : public Control {
public:
  // Caller is responsible for managing pin initialization
  Toggle(Pin &pin, DigitalValue trigger = HIGH);

  bool GetState() const;

protected:
  bool m_state;
  DigitalValue m_lastValue;
  void process() override;
};

}}
