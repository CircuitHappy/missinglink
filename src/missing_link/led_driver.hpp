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

    LEDDriver(uint8_t i2cBus = 1, uint8_t i2cAddress = 0x60);
    virtual ~LEDDriver();

    // Configure expander options
    // Defaults to oscillator on, all LEDs under individual/group PWM control
    void Configure();

    // Set brightness (0 - 1) for an individual LED
    void SetBrightness(float brightness, int index);

  private:

    enum Register : uint8_t;

    std::unique_ptr<GPIO::I2CDevice> m_i2cDevice;
};

}
