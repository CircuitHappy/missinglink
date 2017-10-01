/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include "missing_link/pin_defs.hpp"
#include "missing_link/io.hpp"

#define ML_INTERRUPT_PIN  CHIP_SPI_CS0
#define ML_CLOCK_PIN      CHIP_PE4
#define ML_RESET_PIN      CHIP_PE5

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

namespace MissingLink { namespace ExpanderConstants {

  struct PinDefinition {
    IOExpander::Port port;
    int index;
  };

  static PinDefinition playButtonPin    = { IOExpander::PORTA, 0 }; // in
  static PinDefinition tapButtonPin     = { IOExpander::PORTA, 1 }; // in
  static PinDefinition encAPin          = { IOExpander::PORTA, 2 }; // in
  static PinDefinition encBPin          = { IOExpander::PORTA, 3 }; // in
  static PinDefinition encButtonPin     = { IOExpander::PORTA, 4 }; // in
  static PinDefinition bpmModeLEDPin    = { IOExpander::PORTA, 5 }; // out

  static PinDefinition loopModeLEDPin   = { IOExpander::PORTB, 0 }; // out
  static PinDefinition clockModeLEDPin  = { IOExpander::PORTB, 1 }; // out

  // out
  static PinDefinition animationLEDPin(int index) {
    return { IOExpander::PORTB, index + 2 };
  }

  static IOExpander::InterruptConfig interruptConfig = {
    .activeHigh = false,
    .openDrain  = false,
    .mirror     = false
  };

  // Port A is inputs except pins 5-7
  static IOExpander::PortConfig portAConfig = {
    .direction      = 0b00011111,
    .inputPolarity  = 0b00000000,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00000000,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00000000,
  };

  // Port B is all outputs
  static IOExpander::PortConfig portBConfig = {
    .direction      = 0b00000000,
    .inputPolarity  = 0b00000000,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00000000,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00000000,
  };
}}

IO::IO()
  : f_inputEvent(nullptr)
  , m_pExpander(unique_ptr<IOExpander>(new IOExpander()))
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(ML_INTERRUPT_PIN, Pin::IN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  m_pExpander->ConfigureInterrupt(ExpanderConstants::interruptConfig);
  m_pExpander->ConfigurePort(IOExpander::PORTA, ExpanderConstants::portAConfig);
  m_pExpander->ConfigurePort(IOExpander::PORTB, ExpanderConstants::portBConfig);
}

IO::~IO() {}

void IO::SetBPMModeLED(bool on) {
  auto pinDef = ExpanderConstants::bpmModeLEDPin;
  writeExpanderPin(pinDef.port, pinDef.index, on);
}

void IO::SetLoopModeLED(bool on) {
  auto pinDef = ExpanderConstants::loopModeLEDPin;
  writeExpanderPin(pinDef.port, pinDef.index, on);
}

void IO::SetClockModeLED(bool on) {
  auto pinDef = ExpanderConstants::clockModeLEDPin;
  writeExpanderPin(pinDef.port, pinDef.index, on);
}

void IO::SetClock(bool on) {
  m_pClockOut->Write(on ? HIGH : LOW);
}

void IO::SetReset(bool on) {
  m_pResetOut->Write(on ? HIGH : LOW);
}

void IO::writeExpanderPin(IOExpander::Port port, int index, bool on) {
  lock_guard<mutex> lock(m_expanderMutex);
  m_pExpander->WritePin(port, index, on);
}
