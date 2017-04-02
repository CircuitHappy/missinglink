#pragma once

#include <string>
#include <memory>

#include "missing_link/missinglink_common.hpp"
#include "missing_link/file.hpp"

namespace MissingLink {
namespace GPIO {

class Pin {
public:

  enum Direction {
    IN = 0,
    OUT = 1
  };

  Pin(const int address, const Direction direction);
  virtual ~Pin();

  int Read(DigitalValue &value);
  int Write(const DigitalValue value);

protected:
  const int m_address;
  const Direction m_direction;

  virtual int read(DigitalValue &value) = 0;
  virtual int write(const DigitalValue value) = 0;
};

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

  int read(DigitalValue &value) override;
  int write(const DigitalValue value) override;

  int doExport();
  int doUnexport();
  int doSetDirection();
};

}}
