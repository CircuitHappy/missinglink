/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

// Register address definitions
enum IOExpander::Register : uint8_t {
  IODIR     = 0x00, // IO Direction
  IPOL      = 0x01, // Input Polarity
  GPINTEN   = 0x02, // Interrupt-on-change enable
  DEFVAL    = 0x03, // Default value
  INTCON    = 0x04, // Interrupt-on-change control
  IOCON     = 0x05, // Configuration
  GPPU      = 0x06, // Pull Up config
  INTF      = 0x07, // Interrupt flag
  INTCAP    = 0x08, // Interrupt capture
  GPIO      = 0x09, // GPIO state
  OLAT      = 0x0A, // Output latch
};

// Configuration option mask
enum IOExpander::ConfigOption : uint8_t {
  INT_ACTIVE_HIGH = 0b00000010, // INT polarity (1 = active-high)
  INT_OPEN_DRAIN  = 0b00000100, // INT open drain (1 = open-drain)
  SEQOP_DISABLE   = 0b00100000 // Sequential operation (1 = disable address incrementing)
};

IOExpander::IOExpander(uint8_t i2cBus, uint8_t i2cAddress)
  : m_i2cDevice(unique_ptr<I2CDevice>(new I2CDevice(i2cBus, i2cAddress)))
{}

IOExpander::~IOExpander() {}

void IOExpander::Configure(const Configuration &config) {
  m_i2cDevice->WriteByte(IODIR, config.direction);
  m_i2cDevice->WriteByte(IPOL, config.inputPolarity);
  m_i2cDevice->WriteByte(DEFVAL, config.defaultValue);
  m_i2cDevice->WriteByte(GPINTEN, config.iocEnabled);
  m_i2cDevice->WriteByte(INTCON, config.iocMode);
  m_i2cDevice->WriteByte(GPPU, config.pullUpEnabled);

  uint8_t opts = 0x00;
  if (config.intConfig.activeHigh) opts |= INT_ACTIVE_HIGH;
  if (config.intConfig.openDrain) opts |= INT_OPEN_DRAIN;
  m_i2cDevice->WriteByte(IOCON, opts);
}

uint8_t IOExpander::ReadInterruptFlag() {
  return m_i2cDevice->ReadByte(INTF);
}

uint8_t IOExpander::ReadCapturedInterruptState() {
  return m_i2cDevice->ReadByte(INTCAP);
}

uint8_t IOExpander::ReadGPIO() {
  return m_i2cDevice->ReadByte(GPIO);
}

bool IOExpander::ReadPin(int index) {
  return PinIsOn(index, ReadGPIO());
}

void IOExpander::WritePin(int index, bool on) {
  uint8_t state = m_i2cDevice->ReadByte(OLAT);
  uint8_t pin = 1 << index;
  if (on) {
    state &= ~pin;
  } else {
    state |= pin;
  }
  m_i2cDevice->WriteByte(OLAT, state);
}

void IOExpander::WriteOutput(uint8_t output) {
  m_i2cDevice->WriteByte(OLAT, output);
}


ExpanderInputLoop::ExpanderInputLoop(shared_ptr<IOExpander> pExpander, int interruptPinAddress)
  : m_pExpander(pExpander)
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(interruptPinAddress, Pin::IN)))
  , m_pPollThread(nullptr)
{
  m_pInterruptIn->SetEdgeMode(Pin::FALLING);
}

ExpanderInputLoop::~ExpanderInputLoop() {
  Stop();
}

void ExpanderInputLoop::RegisterHandler(InterruptHandlerPtr handler) {
  m_interruptHandlers.push_back(handler);
}

void ExpanderInputLoop::Start() {
  if (m_pPollThread != nullptr) { return; }
  m_bStopPolling = false;
  m_pPollThread = unique_ptr<thread>(new thread(&ExpanderInputLoop::runLoop, this));
}

void ExpanderInputLoop::Stop() {
  if (m_pPollThread == nullptr) { return; }
  m_bStopPolling = true;
  m_pPollThread->join();
  m_pPollThread = nullptr;
}


void ExpanderInputLoop::runLoop() {

  // Clear initial interrupt event
  m_pInterruptIn->Read();

  pollfd pfd = m_pInterruptIn->GetPollInfo();
  while (!m_bStopPolling) {

    // If interrupt is already low, handle immediately
    if (m_pInterruptIn->Read() == GPIO::LOW) {
      handleInterrupt();
      continue;
    }

    // Poll for 500ms
    int result = ::poll(&pfd, 1, 500);

    // No interrupts? try again
    if (result == 0) { continue; }

    // Clear interrupt event
    m_pInterruptIn->Read();

    // Handle it
    handleInterrupt();
  }
}

void ExpanderInputLoop::handleInterrupt() {
  uint8_t flag = m_pExpander->ReadInterruptFlag();
  uint8_t state = m_pExpander->ReadGPIO();

  for (auto handler : m_interruptHandlers) {
    if (!handler->CanHandleInterrupt(flag)) {
      continue;
    }
    handler->HandleInterrupt(flag, state, m_pExpander);
  }

  this_thread::sleep_for(chrono::milliseconds(1));
}

ExpanderInputLoop::InterruptHandler::InterruptHandler(vector<int> pinIndices) {
  m_flagMask = 0;
  for (auto index : pinIndices) {
    m_flagMask |= (1 << index);
  }
}

void ExpanderInputLoop::InterruptHandler::HandleInterrupt(uint8_t flag,
                                                          uint8_t state,
                                                          shared_ptr<IOExpander> pExpander) {
  handleInterrupt(flag, state, pExpander);
}

