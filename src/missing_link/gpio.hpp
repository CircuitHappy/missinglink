#pragma once

#include <string>
#include <memory>

#include "missing_link/missinglink_common.hpp"
#include "missing_link/file.hpp"

namespace MissingLink {
namespace GPIO {

// Abstract base class for GPIO pins
class Pin {
public:

  enum Direction {
    IN = 0,
    OUT = 1
  };

  Pin(const int address, const Direction direction);
  virtual ~Pin();

  int Read(DigitalValue *value) const;
  int Write(const DigitalValue value) const;

protected:
  const int m_address;
  const Direction m_direction;

  virtual int read(DigitalValue *value) const = 0;
  virtual int write(const DigitalValue value) const = 0;
};


// Pin Implementation based on linux sysfs
class SysfsPin : public Pin {
public:
  SysfsPin(const int address, const Direction direction);

  int Export();
  int Unexport();

protected:
  static const std::string s_rootInterfacePath;
  std::string getPinInterfacePath() const;
  virtual std::unique_ptr<File> createFile(const std::string strPath, const File::Access access);

private:
  std::unique_ptr<File> m_valueFile;

  int read(DigitalValue *value)const override;
  int write(const DigitalValue value) const override;

  int doExport();
  int doUnexport();
  int doSetDirection();
};


class Control {
public:
  Control(const Pin &pin, DigitalValue trigger = HIGH);
  virtual ~Control();
  void Process();
protected:
  const Pin &m_pin;
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
  Button(const Pin &pin, DigitalValue trigger = HIGH);

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
  Toggle(const Pin &pin, DigitalValue trigger = HIGH);

  bool GetState() const;

protected:
  bool m_state;
  DigitalValue m_lastValue;
  void process() override;
};

}}
