/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <chrono>
#include <poll.h>
#include "missing_link/pin_defs.hpp"
#include "missing_link/io.hpp"

#define ML_INTERRUPT_PIN  CHIP_SPI_CS0
#define ML_CLOCK_PIN      CHIP_PE4
#define ML_RESET_PIN      CHIP_PE5

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

namespace {

  struct ExpanderPinDefinition {
    IOExpander::Port port;
    int index;

    uint8_t bitmask() {
      return 1 << index;
    }
  };

  static ExpanderPinDefinition PlayButtonPin    = { IOExpander::PORTA, 0 }; // in
  static ExpanderPinDefinition TapButtonPin     = { IOExpander::PORTA, 1 }; // in
  static ExpanderPinDefinition EncAPin          = { IOExpander::PORTA, 2 }; // in
  static ExpanderPinDefinition EncBPin          = { IOExpander::PORTA, 3 }; // in
  static ExpanderPinDefinition EncButtonPin     = { IOExpander::PORTA, 4 }; // in
  static ExpanderPinDefinition BpmModeLEDPin    = { IOExpander::PORTA, 5 }; // out
  static ExpanderPinDefinition LoopModeLEDPin   = { IOExpander::PORTB, 0 }; // out
  static ExpanderPinDefinition ClockModeLEDPin  = { IOExpander::PORTB, 1 }; // out
  static ExpanderPinDefinition AnimationLEDPin(int index) {
    return { IOExpander::PORTB, index + 2 };
  }

  static IOExpander::InterruptConfig IntConfig = {
    .activeHigh = false,
    .openDrain  = false,
    .mirror     = false
  };

  // Port A is inputs except pins 5-7
  static IOExpander::PortConfig PortAConfig = {
    .direction      = 0b00011111,
    .inputPolarity  = 0b00000000,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00011111,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00000000,
  };

  // Port B is all outputs
  static IOExpander::PortConfig PortBConfig = {
    .direction      = 0b00000000,
    .inputPolarity  = 0b00000000,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00000000,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00000000,
  };
}

IO::IO()
  : f_inputEvent(nullptr)
  , m_pExpander(unique_ptr<IOExpander>(new IOExpander()))
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(ML_INTERRUPT_PIN, Pin::IN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  // Configure Interrupt pin
  m_pInterruptIn->SetEdgeMode(Pin::FALLING);

  // Configure Expander
  m_pExpander->ConfigureInterrupt(IntConfig);
  m_pExpander->ConfigurePort(IOExpander::PORTA, PortAConfig);
  m_pExpander->ConfigurePort(IOExpander::PORTB, PortBConfig);

  // Clear interrupt states
  m_pExpander->ReadCapturedInterruptState(IOExpander::PORTA);
  m_pExpander->ReadCapturedInterruptState(IOExpander::PORTB);
}

IO::~IO() {
  StopPollingInput();
}

void IO::StartPollingInput() {
  if (m_pPollThread != nullptr) { return; }
  m_bStopPolling = false;
  m_pPollThread = unique_ptr<thread>(new thread(&IO::runPollInput, this));
}

void IO::StopPollingInput() {
  if (m_pPollThread == nullptr) { return; }
  m_bStopPolling = true;
  m_pPollThread->join();
  m_pPollThread = nullptr;
}

void IO::SetBPMModeLED(bool on) {
  auto pinDef = BpmModeLEDPin;
  writeExpanderPin(pinDef.port, pinDef.index, on);
}

void IO::SetLoopModeLED(bool on) {
  auto pinDef = LoopModeLEDPin;
  writeExpanderPin(pinDef.port, pinDef.index, on);
}

void IO::SetClockModeLED(bool on) {
  auto pinDef = ClockModeLEDPin;
  writeExpanderPin(pinDef.port, pinDef.index, on);
}

void IO::SetClock(bool on) {
  m_pClockOut->Write(on ? HIGH : LOW);
}

void IO::SetReset(bool on) {
  m_pResetOut->Write(on ? HIGH : LOW);
}

void IO::runPollInput() {

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

void IO::handleInterrupt() {
  uint8_t flag = m_pExpander->ReadInterruptFlag(IOExpander::PORTA);
  uint8_t state = m_pExpander->ReadCapturedInterruptState(IOExpander::PORTA);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void IO::writeExpanderPin(IOExpander::Port port, int index, bool on) {
  lock_guard<mutex> lock(m_expanderMutex);
  m_pExpander->WritePin(port, index, on);
}
