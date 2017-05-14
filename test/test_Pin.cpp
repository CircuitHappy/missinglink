#include "missing_link/gpio.hpp"
#include "gtest/gtest.h"

using namespace MissingLink;

namespace {

class TestPin : public GPIO::Pin {
public:
  TestPin(const int address, const GPIO::Pin::Direction direction, DigitalValue *outValue)
    : Pin(address, direction)
    , valueToRead(LOW)
    , outValue(outValue)
  {}

  DigitalValue valueToRead;
  DigitalValue *outValue;

private:
  int write(const DigitalValue value) override {
    *outValue = value;
    return 0;
  }
  int read(DigitalValue *value) override {
    *value = valueToRead;
    return 0;
  }
};

}

TEST(Pin, WritesIfIsOutput) {
  DigitalValue outValue;
  TestPin output(100, GPIO::Pin::OUT, &outValue);
  int result = output.Write(HIGH);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(outValue, HIGH);
  result = output.Write(LOW);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(outValue, LOW);
};

TEST(Pin, FailsToWriteIfIsInput) {
  DigitalValue outValue = LOW;
  TestPin output(100, GPIO::Pin::IN, &outValue);
  int result = output.Write(HIGH);
  EXPECT_NE(result, 0);
  EXPECT_EQ(outValue, LOW);
};

TEST(Pin, ReadsIfIsInput) {
  DigitalValue outValue;
  TestPin input(100, GPIO::Pin::IN, &outValue);

  DigitalValue value = LOW;

  input.valueToRead = HIGH;
  int result = input.Read(&value);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(value, HIGH);

  input.valueToRead = LOW;
  result = input.Read(&value);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(value, LOW);
};

TEST(Pin, FailsToReadIfIsOutput) {
  DigitalValue outValue;
  TestPin input(100, GPIO::Pin::OUT, &outValue);
  input.valueToRead = HIGH;

  DigitalValue value = LOW;
  int result = input.Read(&value);
  EXPECT_NE(result, 0);
  EXPECT_NE(value, HIGH);
};

