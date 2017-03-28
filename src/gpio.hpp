#pragma once

namespace MissingLink {
namespace GPIO {

enum DigitalValue {
  LOW = 0,
  HIGH = 1
};

enum Direction {
  IN = 0,
  OUT = 1
};

class Pin {

public:
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
