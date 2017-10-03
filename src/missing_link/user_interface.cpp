/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <poll.h>

#include "missing_link/pin_defs.hpp"
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"
#include "missing_link/user_interface.hpp"

#define ML_INTERRUPT_PIN  CHIP_SPI_CS0
#define ML_CLOCK_PIN      CHIP_PE4
#define ML_RESET_PIN      CHIP_PE5

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

namespace MissingLink {

  // Inputs all on PORT A
  enum InputPinIndex {
    PLAY_BUTTON   = 0,
    TAP_BUTTON    = 1,
    ENC_A         = 2,
    ENC_B         = 3,
    ENC_BUTTON    = 4
  };

  // Output pin definitions
  static IOExpander::PinDefinition BPMModeLEDPin    = { IOExpander::PORTA, 5 };
  static IOExpander::PinDefinition LoopModeLEDPin   = { IOExpander::PORTB, 0 };
  static IOExpander::PinDefinition ClockModeLEDPin  = { IOExpander::PORTB, 1 };
  static IOExpander::PinDefinition AnimationLEDPin(int index) {
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
    .inputPolarity  = 0b00011111,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00011111,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00011111
  };

  // Port B is all outputs
  static IOExpander::PortConfig PortBConfig = {
    .direction      = 0b00000000,
    .inputPolarity  = 0b00000000,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00000000,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00000000
  };

}

Button::Button(int pinIndex, IOExpander::Port port, chrono::milliseconds debounceInterval)
  : ExpanderInputLoop::InterruptHandler({ pinIndex }, port)
  , m_debounceInterval(debounceInterval)
{}

Button::~Button() {}

bool Button::handleInterrupt(uint8_t flag, uint8_t state, shared_ptr<IOExpander> pExpander) {

  // check change to ON
  if ((flag & state) == 0) { return false; }

  // maximum rate between ON is 5ms
  auto now = chrono::steady_clock::now();
  if ((now - m_lastTriggered) < chrono::milliseconds(5)) { return false; }

  // sleep to debounce
  this_thread::sleep_for(chrono::milliseconds(m_debounceInterval));

  // check again to make sure it's still on
  if ((pExpander->ReadPort(m_port) | flag) == 0) { return false; }

  cout << "Button on!" << endl;
  m_lastTriggered = chrono::steady_clock::now();
  return true;
}

UserInterface::UserInterface()
  : onInputEvent(nullptr)
  , m_pExpander(shared_ptr<IOExpander>(new IOExpander()))
  , m_pInputLoop(unique_ptr<ExpanderInputLoop>(new ExpanderInputLoop(m_pExpander, ML_INTERRUPT_PIN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  // Configure Expander
  m_pExpander->ConfigureInterrupt(IntConfig);
  m_pExpander->ConfigurePort(IOExpander::PORTA, PortAConfig);
  m_pExpander->ConfigurePort(IOExpander::PORTB, PortBConfig);

  // Clear interrupt states
  m_pExpander->ReadCapturedInterruptState(IOExpander::PORTA);
  m_pExpander->ReadCapturedInterruptState(IOExpander::PORTB);

  auto button = shared_ptr<Button>(new Button(PLAY_BUTTON));
  m_pInputLoop->RegisterHandler(button);
}

UserInterface::~UserInterface() {
  StopPollingInput();
}

void UserInterface::StartPollingInput() {
  m_pInputLoop->Start();
}

void UserInterface::StopPollingInput() {
  m_pInputLoop->Stop();
}

void UserInterface::SetBPMModeLED(bool on) {
  m_pExpander->WritePin(BPMModeLEDPin, on);
}

void UserInterface::SetLoopModeLED(bool on) {
  m_pExpander->WritePin(LoopModeLEDPin, on);
}

void UserInterface::SetClockModeLED(bool on) {
  m_pExpander->WritePin(ClockModeLEDPin, on);
}

void UserInterface::SetAnimationLED(int index, bool on) {
  if (index > 5) { return; }
  m_pExpander->WritePin(AnimationLEDPin(index), on);
}

void UserInterface::SetClock(bool on) {
  m_pClockOut->Write(on ? HIGH : LOW);
}

void UserInterface::SetReset(bool on) {
  m_pResetOut->Write(on ? HIGH : LOW);
}
