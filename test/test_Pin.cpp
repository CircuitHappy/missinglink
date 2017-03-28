#include "missing_link/gpio.hpp"
#include "gtest/gtest.h"

using namespace MissingLink;

namespace {

class TestPin : public GPIO::Pin {
public:
  TestPin(const int address, const GPIO::Pin::Direction direction)
    : Pin(address, direction)
    , valueWritten(LOW)
    , valueToRead(LOW)
  {}

  DigitalValue valueWritten;
  DigitalValue valueToRead;

private:
  int write(const DigitalValue value) override {
    valueWritten = value;
    return 0;
  }
  int read(DigitalValue &value) override {
    value = valueToRead;
    return 0;
  }
};

}

TEST(Pin, WritesIfIsOutput) {
  TestPin output(100, GPIO::Pin::OUT);
  int result = output.Write(HIGH);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(output.valueWritten, HIGH);
  result = output.Write(LOW);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(output.valueWritten, LOW);

};

TEST(Pin, FailsToWriteIfIsInput) {
  TestPin output(100, GPIO::Pin::IN);
  int result = output.Write(HIGH);
  EXPECT_NE(result, 0);
  EXPECT_EQ(output.valueWritten, LOW);
};

TEST(Pin, ReadsIfIsInput) {
  TestPin input(100, GPIO::Pin::IN);

  DigitalValue value = LOW;

  input.valueToRead = HIGH;
  int result = input.Read(value);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(value, HIGH);

  input.valueToRead = LOW;
  result = input.Read(value);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(value, LOW);
};

TEST(Pin, FailsToReadIfIsOutput) {
  TestPin input(100, GPIO::Pin::OUT);
  input.valueToRead = HIGH;

  DigitalValue value = LOW;
  int result = input.Read(value);
  EXPECT_NE(result, 0);
  EXPECT_NE(value, HIGH);
};

