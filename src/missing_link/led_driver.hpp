/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace MissingLink {

namespace GPIO { class I2CDevice; }


// i2c interface abstraction for TLC59116 16-Channel LED Sink Driver
// http://www.ti.com/lit/ds/symlink/tlc59116.pdf
class LEDDriver {

  public:

    enum LEDAddress : uint8_t;

    // Abstraction for port configuration bitmasks.
    // Each bit in each field corresponds to the port pin at that position,
    // starting with pin 0 as the least significant bit.
    struct Configuration {
      uint8_t OscOn;      // 0 = off, 1 = on
      uint8_t LED_ON;  // 0x00 = off, 0xff = on
      uint8_t LEDOut2;
      uint8_t LEDOut3;     // 0 = disabled, 1 = enabled
    };

    LEDDriver(uint8_t i2cBus = 1, uint8_t i2cAddress = 0x60);
    virtual ~LEDDriver();

    // Configure expander options
    void Configure(const Configuration &config);

    // Turn on or off LED, no PWM values allowed for now
    void WriteLED(int address, bool on);

  private:

    enum Register : uint8_t;
    enum ConfigOption : uint8_t;

    std::unique_ptr<GPIO::I2CDevice> m_i2cDevice;
};


}
