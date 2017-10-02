/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

using namespace MissingLink;

// Register address in BANK=1 mode.
// Derive address for each port by OR-ing with port value.
enum IOExpander::Register : uint8_t {
  IODIR     = 0x00, // IO Direction
  IPOL      = 0x01, // Input Polarity
  GPINTEN   = 0x02, // Interrupt-on-change enable
  DEFVAL    = 0x03, // Default value
  INTCON    = 0x04, // Interrupt-on-change control
  IOCON     = 0x05, // Configuration
  GPPU      = 0x06, // Pull Up config
  INTF      = 0x07, // Interrupt flag
  INTCAP    = 0x08, // Interrupt port capture
  GPIO      = 0x09, // GPIO port state
  OLAT      = 0x0A, // Output latch
};

// Configuration option mask
enum IOExpander::ConfigOption : uint8_t {
  INT_ACTIVE_HIGH = 0b00000010, // INT polarity (1 = active-high)
  INT_OPEN_DRAIN  = 0b00000100, // INT open drain (1 = open-drain)
  SEQOP_DISABLE   = 0b00100000, // Sequential operation (1 = disable address incrementing)
  INT_MIRROR      = 0b01000000, // INT mirroring (1 = internally connect interrupts)
  BANK_ADDRESSING = 0b10000000  // Bank mode config (1 = separate banks, 0 = interleaved)
};

IOExpander::IOExpander(uint8_t i2cBus, uint8_t i2cAddress)
  : m_i2cDevice(std::unique_ptr<GPIO::I2CDevice>(new GPIO::I2CDevice(i2cBus, i2cAddress)))
{
  // If not in BANK mode (default state), the IOCON register is at 0x0A.
  // Once the option is set, it will be located at the enum lookup value.
  m_i2cDevice->WriteByte(0x0A, BANK_ADDRESSING);
  m_i2cDevice->WriteByte(IOCON, BANK_ADDRESSING);
  m_i2cDevice->WriteByte(0x0A, 0x00); // reset OLAT if device was not in bank address mode
}

IOExpander::~IOExpander() {}

void IOExpander::ConfigureInterrupt(const InterruptConfig &config) {
  uint8_t opts = BANK_ADDRESSING;
  if (config.activeHigh) opts |= INT_ACTIVE_HIGH;
  if (config.openDrain) opts |= INT_OPEN_DRAIN;
  if (config.mirror) opts |= INT_MIRROR;
  m_i2cDevice->WriteByte(IOCON, opts);
}

void IOExpander::ConfigurePort(Port port, const PortConfig &config) {
  m_i2cDevice->WriteByte(IODIR | port, config.direction);
  m_i2cDevice->WriteByte(IPOL | port, config.inputPolarity);
  m_i2cDevice->WriteByte(DEFVAL | port, config.defaultValue);
  m_i2cDevice->WriteByte(GPINTEN | port, config.iocEnabled);
  m_i2cDevice->WriteByte(INTCON | port, config.iocMode);
  m_i2cDevice->WriteByte(GPPU | port, config.pullUpEnabled);
}

uint8_t IOExpander::ReadInterruptFlag(Port port) {
  return m_i2cDevice->ReadByte(INTF | port);
}

uint8_t IOExpander::ReadCapturedInterruptState(Port port) {
  return m_i2cDevice->ReadByte(INTCAP | port);
}

uint8_t IOExpander::ReadPort(Port port) {
  return m_i2cDevice->ReadByte(GPIO | port);
}

bool IOExpander::ReadPin(const PinDefinition &pin) {
  return ReadPin(pin.port, pin.index);
}

bool IOExpander::ReadPin(Port port, int index) {
  return PinIsOn(index, ReadPort(port));
}

void IOExpander::WritePin(const PinDefinition &pin, bool on) {
  WritePin(pin.port, pin.index, on);
}

void IOExpander::WritePin(Port port, int index, bool on) {
  uint8_t reg = OLAT | port;
  uint8_t state = m_i2cDevice->ReadByte(reg);
  uint8_t pin = 1 << index;
  if (on) {
    state &= ~pin;
  } else {
    state |= pin;
  }
  m_i2cDevice->WriteByte(reg, state);
}

void IOExpander::WriteOutputs(Port port, uint8_t outputs) {
  uint8_t reg = OLAT | port;
  m_i2cDevice->WriteByte(reg, outputs);
}
