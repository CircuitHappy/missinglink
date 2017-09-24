#pragma once

#include <string>
#include <memory>

#include "missing_link/common.h"

namespace MissingLink {
namespace GPIO {

// Abstract base class for GPIO pins
class Pin {
public:

  enum Direction {
    IN = 0,
    OUT = 1
  };

  Pin(int address, Direction direction, DigitalValue initial = LOW);
  virtual ~Pin();

  int Read(DigitalValue *value);
  int Write(DigitalValue value);

protected:
  const int m_address;
  const Direction m_direction;
  const DigitalValue m_initialValue;

  virtual int read(DigitalValue *value) = 0;
  virtual int write(DigitalValue value) = 0;

private:
  DigitalValue m_lastValueWritten;
};


// Pin Implementation based on linux sysfs
class SysfsPin : public Pin {
public:
  SysfsPin(int address, Direction direction);

  int Export();
  int Unexport();

protected:
  static const std::string s_rootInterfacePath;

  const std::string m_pinInterfacePath;

  virtual int writeToFile(const std::string &strPath, const void *buf, size_t nBytes);
  virtual int readFromFile(const std::string &strPath, void *buf, size_t nBytes);

private:
  int read(DigitalValue *value) override;
  int write(DigitalValue value) override;

  int doExport();
  int doUnexport();
  int doSetDirection();
};


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
