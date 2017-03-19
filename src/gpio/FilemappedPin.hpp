#pragma once

#include "gpio/gpio_types.hpp"

namespace MissingLink {
namespace GPIO {

class FilemappedPin {
public:
  FilemappedPin(int pinId);
  virtual ~FilemappedPin();

  void enable();
private:
  int m_pinId;
};

}}
