#include "missing_link/gpio.hpp"
#include "gtest/gtest.h"

using namespace MissingLink;
using std::string;

namespace {

class TestSysfsPin : public GPIO::SysfsMappedPin {
public:
  TestSysfsPin(const int address, const GPIO::Pin::Direction direction)
    : SysfsMappedPin(address, direction)
  {}

  std::map<string, string> lastWrites;

private:
  int writeToFile(const string strPath, const string strValue) override {
    lastWrites[strPath] = strValue;
    return 0;
  };
};

}

TEST(SysfsMappedPin, ExportsAddressOnExport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  EXPECT_EQ(pin.lastWrites["/sys/class/gpio/export"], "100");
}

TEST(SysfsMappedPin, WritesDirectionOnExport) {
  TestSysfsPin outPin(100, GPIO::Pin::OUT);
  EXPECT_EQ(outPin.Export(), 0);
  EXPECT_EQ(outPin.lastWrites["/sys/class/gpio/gpio100/direction"], "out");

  TestSysfsPin inPin(100, GPIO::Pin::IN);
  EXPECT_EQ(inPin.Export(), 0);
  EXPECT_EQ(inPin.lastWrites["/sys/class/gpio/gpio100/direction"], "in");
}

TEST(SysfsMappedPin, WritesValues) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  EXPECT_EQ(pin.Write(LOW), 0);
  EXPECT_EQ(pin.lastWrites["/sys/class/gpio/gpio100/value"], "0");
  EXPECT_EQ(pin.Write(HIGH), 0);
  EXPECT_EQ(pin.lastWrites["/sys/class/gpio/gpio100/value"], "1");
}
