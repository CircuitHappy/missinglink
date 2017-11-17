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

// FIXME: This SHOULD be CHIP_SPI_CS0 according to the schematic
#define ML_INTERRUPT_PIN  CHIP_SPI_CLK

#define ML_CLOCK_PIN      CHIP_PE4
#define ML_RESET_PIN      CHIP_PE5

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

namespace MissingLink {

  enum InputPinIndex {
    ENC_A         = 0,
    ENC_B         = 1,
    ENC_BUTTON    = 2,
    PLAY_BUTTON   = 3,
    TAP_BUTTON    = 4
  };

  static IOExpander::InterruptConfiguration ExpanderIntConfig = {
    .activeHigh = false,
    .openDrain  = false
  };

  // Port A is inputs
  static IOExpander::Configuration ExpanderConfig = {
    .direction      = 0b00011111,
    .inputPolarity  = 0b00011111,
    .defaultValue   = 0b00000000,
    .iocEnabled     = 0b00011111,
    .iocMode        = 0b00000000,
    .pullUpEnabled  = 0b00011111,
    .intConfig      = ExpanderIntConfig
  };
}

Button::Button(int pinIndex, chrono::milliseconds debounceInterval)
  : ExpanderInputLoop::InterruptHandler({ pinIndex })
  , m_debounceInterval(debounceInterval)
{}

Button::~Button() {}

void Button::handleInterrupt(uint8_t flag, uint8_t state, shared_ptr<IOExpander> pExpander) {

  // check change to ON
  if ((flag & state) == 0) {
    return;
  }

  // maximum rate between ON is 5ms
  auto now = chrono::steady_clock::now();
  if ((now - m_lastTriggered) < chrono::milliseconds(5)) {
    return;
  }

  // sleep to debounce
  this_thread::sleep_for(chrono::milliseconds(m_debounceInterval));

  // check again to make sure it's still on
  if ((pExpander->ReadGPIO() | flag) == 0) {
    return;
  }

  cout << "Button on!" << endl;
  m_lastTriggered = chrono::steady_clock::now();
}


RotaryEncoder::RotaryEncoder(int pinIndexA, int pinIndexB)
  : ExpanderInputLoop::InterruptHandler({ pinIndexA, pinIndexB })
  , m_aFlag(1 << pinIndexA)
  , m_bFlag(1 << pinIndexB)
  , m_lastEncSeq(0)
{}

RotaryEncoder::~RotaryEncoder() {}

void RotaryEncoder::handleInterrupt(uint8_t flag, uint8_t state, shared_ptr<IOExpander> pExpander) {
  bool aOn = (m_aFlag & state) != 0;
  bool bOn = (m_bFlag & state) != 0;
  decode(aOn, bOn);
}

void RotaryEncoder::decode(bool aOn, bool bOn) {
  uint8_t aVal = aOn ? 0x01 : 0x00;
  uint8_t bVal = bOn ? 0x01 : 0x00;

  unsigned int seq = (aVal ^ bVal) | (bVal << 1);
  unsigned int delta = (seq - m_lastEncSeq) & 0b11;

  m_lastEncSeq = seq;

  switch (delta) {
    case 1:
      m_encVal++;
      break;
    case 3:
      m_encVal--;
      break;
    default:
      break;
  }

  if (std::abs(m_encVal) >= 4) {
    if (m_encVal > 0) {
      std::cout << "Encoder up" << std::endl;
    } else {
      std::cout << "Encoder down" << std::endl;
    }
    m_encVal = 0;
  }
}


UserInterface::UserInterface()
  : onInputEvent(nullptr)
  , m_pExpander(shared_ptr<IOExpander>(new IOExpander()))
  , m_pInputLoop(unique_ptr<ExpanderInputLoop>(new ExpanderInputLoop(m_pExpander, ML_INTERRUPT_PIN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  // Configure Expander
  m_pExpander->Configure(ExpanderConfig);

  // Clear interrupt state
  m_pExpander->ReadCapturedInterruptState();

  // Register handlers
  auto playButton = shared_ptr<Button>(new Button(PLAY_BUTTON));
  auto tapButton = shared_ptr<Button>(new Button(TAP_BUTTON));
  auto encoderButton = shared_ptr<Button>(new Button(ENC_BUTTON));
  auto encoder = shared_ptr<RotaryEncoder>(new RotaryEncoder(ENC_A, ENC_B));
  m_pInputLoop->RegisterHandler(playButton);
  m_pInputLoop->RegisterHandler(tapButton);
  m_pInputLoop->RegisterHandler(encoderButton);
  m_pInputLoop->RegisterHandler(encoder);
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

}

void UserInterface::SetLoopModeLED(bool on) {

}

void UserInterface::SetClockModeLED(bool on) {

}

void UserInterface::SetAnimationLED(int index, bool on) {

}

void UserInterface::SetClock(bool on) {
  m_pClockOut->Write(on ? HIGH : LOW);
}

void UserInterface::SetReset(bool on) {
  m_pResetOut->Write(on ? HIGH : LOW);
}
