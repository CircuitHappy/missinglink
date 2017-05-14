#include "missing_link/gpio.hpp"
#include "gtest/gtest.h"

#include <cstring>
#include <memory>

using namespace MissingLink;
using std::string;

namespace {

class TestSysfsPin : public GPIO::SysfsPin {
public:
  TestSysfsPin(const int address, const GPIO::Pin::Direction direction)
    : SysfsPin(address, direction)
  {}

private:

};

}

TEST(SysfsPin, ExportsAddressOnExport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  auto fileStorage = pin.GetExportFileStorage();
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "100");
}

TEST(SysfsPin, WritesDirectionOnExport) {
  TestSysfsPin outPin(100, GPIO::Pin::OUT);
  EXPECT_EQ(outPin.Export(), 0);
  auto fileStorage = outPin.GetDirectionFileStorage();
  ASSERT_NE(fileStorage, nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "out");

  TestSysfsPin inPin(100, GPIO::Pin::IN);
  EXPECT_EQ(inPin.Export(), 0);
  fileStorage = inPin.GetDirectionFileStorage();
  ASSERT_NE(fileStorage, nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "in");
}

TEST(SysfsPin, OpensValueFileOnExport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  auto fileStorage = pin.GetValueFileStorage();
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_TRUE(fileStorage->isOpen);
}

TEST(SysfsPin, UnexportsAddressOnUnexport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  EXPECT_EQ(pin.Unexport(), 0);
  auto fileStorage = pin.GetUnexportFileStorage();
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "100");
}

TEST(SysfsPin, ClosesValueFileOnUnexport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  EXPECT_EQ(pin.Unexport(), 0);
  auto fileStorage = pin.GetValueFileStorage();
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
}

TEST(SysfsPin, WritesValues) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);

  auto fileStorage = pin.GetValueFileStorage();
  ASSERT_NE(fileStorage.get(), nullptr);

  EXPECT_EQ(pin.Write(LOW), 0);
  EXPECT_STREQ(fileStorage->buffer, "0");

  EXPECT_EQ(pin.Write(HIGH), 0);
  EXPECT_STREQ(fileStorage->buffer, "1");
}

TEST(SysfsPin, ReadsValues) {
  TestSysfsPin pin(100, GPIO::Pin::IN);
  EXPECT_EQ(pin.Export(), 0);

  auto fileStorage = pin.GetValueFileStorage();
  ASSERT_NE(fileStorage.get(), nullptr);

  DigitalValue result;

  // This implicitly tests seek because the fake storage
  // maintains its own buffer offset (like a real file) when reading

  ::strcpy(fileStorage->buffer, "0");
  EXPECT_EQ(pin.Read(&result), 0);
  EXPECT_EQ(result, LOW);

  ::strcpy(fileStorage->buffer, "1");
  EXPECT_EQ(pin.Read(&result), 0);
  EXPECT_EQ(result, HIGH);

}
