#pragma once

#include <iostream>
#include "gpio/gpio_types.hpp"

namespace MissingLink {
namespace GPIO {

class FilemappedPin {
public:
  FilemappedPin(int pinId);
  virtual ~FilemappedPin();
  void enable();
protected:
  static const std::string sysfsPrefix;
  const int m_pinId;
};

//class FilemappedInput : public FilemappedPin {
//public:
//  GPIO::DigitalValue read() const;
//};

//class FilemappedOutput : public FilemappedPin {
//public:
//  FilemappedOutput(int pinId);
//  void write(DigitalValue value) const;
//};

}}
