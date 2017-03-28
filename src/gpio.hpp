#pragma once

#include "missinglink_types.hpp"

namespace MissingLink {
namespace GPIO {

class Pin {

public:

  enum Direction {
    IN = 0,
    OUT = 1
  };

  Pin(const int address);

  void Export();
  void Unexport();

  void SetDirection(Direction direction);

  void Write(DigitalValue value);

  DigitalValue Read();

protected:

private:
  const int m_address;
};

}}
