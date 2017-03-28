#pragma once

#include <string>
#include "missing_link/missinglink_common.hpp"

namespace MissingLink {
namespace GPIO {

class Pin {
public:

  enum Direction {
    IN = 0,
    OUT = 1
  };

  Pin(const int address, const Direction direction);

  int Read(DigitalValue &value);
  int Write(const DigitalValue value);

protected:
  const int m_address;
  const Direction m_direction;

  virtual int read(DigitalValue &value) = 0;
  virtual int write(const DigitalValue value) = 0;
};

class SysfsMappedPin : public Pin {
public:
  SysfsMappedPin(const int address, const Direction direction);

  int Export();
  int Unexport();

protected:
  std::string getInterfacePath() const;
  virtual int writeToFile(const std::string strPath, const std::string strValue);
  virtual int readFromFile(const std::string strPath, std::string &strValue);

private:
  int read(DigitalValue &value) override;
  int write(const DigitalValue value) override;
};

}}
