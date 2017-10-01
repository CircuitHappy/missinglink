/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include "missing_link/gpio.hpp"

namespace MissingLink {

// i2c interface abstraction for MCP23017 in BANK=1 mode.
// http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
class IOExpander {

  public:

    enum Port : uint8_t {
      PORTA = 0x00,
      PORTB = 0x10
    };

    // Abstraction for port configuration bitmasks.
    // Each bit in each field corresponds to the port pin at that position,
    // starting with pin 0 as the least significant bit.
    struct PortConfig {
      uint8_t direction;      // 0 = output, 1 = input
      uint8_t inputPolarity;  // 0 = normal, 1 = inverted
      uint8_t defaultValue;
      uint8_t iocEnabled;     // 0 = disabled, 1 = enabled
      uint8_t iocMode;        // 0 = compare with previous, 1 = compare with default
      uint8_t pullUpEnabled;  // 0 = disabled, 1 = enabled
    };

    // Abstraction for interrupt configuration bitmask
    struct InterruptConfig {
      bool activeHigh;
      bool openDrain;
      bool mirror;
    };

    IOExpander();
    virtual ~IOExpander();

    void ConfigureInterrupt(const InterruptConfig &config);
    void ConfigurePort(Port port, const PortConfig &config);

    // Read from the INTF register on the given port.
    // A set bit indicates that the corresponding pin generated the interrupt.
    uint8_t ReadInterruptFlag(Port port);

    // Read from the INTCAP register on the given port.
    // Indicates GPIO state at time of interrupt.
    // Reading this will clear the interrupt.
    uint8_t ReadCapturedInterruptState(Port port);

    // Read from the GPIO register on the given port.
    // This reflects the current state of all I/O pins, input or output.
    // Reading this will clear the interrupt.
    uint8_t ReadPort(Port port);

    // Turn an output on or off. This will write directly to the output
    // latch without modifying other pin states.
    void WritePin(Port port, int index, bool on);

    // Write a full byte to output latch.
    void WriteOutputs(Port port, uint8_t outputs);

  private:

    enum Register : uint8_t;
    enum ConfigOption : uint8_t;

    std::unique_ptr<GPIO::I2CDevice> m_i2cDevice;
};

}
