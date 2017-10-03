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

// i2c interface abstraction for MCP23017 in BANK=1 mode.
// http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
class IOExpander {

  public:

    enum Port : uint8_t {
      PORTA = 0x00,
      PORTB = 0x10
    };

    struct PinDefinition {
      Port port;
      int index;
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

    IOExpander(uint8_t i2cBus = 1, uint8_t i2cAddress = 0x20);
    virtual ~IOExpander();

    void ConfigureInterrupt(const InterruptConfig &config);

    // Configure port from
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

    // Read the value of a specific pin from the GPIO register.
    // This will reset pending interrupts.
    bool ReadPin(const PinDefinition &pin);
    bool ReadPin(Port port, int index);

    // Turn an output on or off. This will write directly to the output
    // latch without modifying other pin states.
    void WritePin(const PinDefinition &pin, bool on);
    void WritePin(Port port, int index, bool on);

    // Write a full byte to output latch.
    void WriteOutputs(Port port, uint8_t outputs);

    static bool PinIsOn(int pinIndex, uint8_t portState) {
      uint8_t mask = (1 << pinIndex);
      return (mask & portState) != 0;
    }

  private:

    enum Register : uint8_t;
    enum ConfigOption : uint8_t;

    std::unique_ptr<GPIO::I2CDevice> m_i2cDevice;
};

// Abstraction for handling single-pin interrupt based input from the IO expander
class ExpanderInputLoop {

  public:

    class InterruptHandler;

    typedef std::shared_ptr<InterruptHandler> InterruptHandlerPtr;

    ExpanderInputLoop(std::shared_ptr<IOExpander> m_pExpander, int interruptPinAddress);
    virtual ~ExpanderInputLoop();

    // Must register interrupt handlers before starting polling loop!
    void RegisterHandler(InterruptHandlerPtr handler);

    void Start();
    void Stop();

  private:

    std::shared_ptr<IOExpander> m_pExpander;
    std::unique_ptr<GPIO::Pin> m_pInterruptIn;

    std::atomic<bool> m_bStopPolling;
    std::unique_ptr<std::thread> m_pPollThread;

    std::vector<InterruptHandlerPtr> m_interruptHandlers;

    void runLoop();
    void handleInterrupt();
};

class ExpanderInputLoop::InterruptHandler {

  public:

    InterruptHandler(std::vector<int> pinIndices,
                     IOExpander::Port port = IOExpander::PORTA);

    virtual ~InterruptHandler() {};

    bool CanHandleInterrupt(uint8_t flag) {
      return (flag & m_flagMask) != 0;
    }

    bool HandleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander);

  protected:

    uint8_t m_flagMask;
    IOExpander::Port m_port;

    virtual bool handleInterrupt(uint8_t flag,
                                 uint8_t state,
                                 std::shared_ptr<IOExpander> pExpander) = 0;
};


}
