/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <iomanip>
#include <vector>

#include "missing_link/hw_defs.h"
#include "missing_link/engine.hpp"
#include "missing_link/user_interface.hpp"

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

UserInputProcess::UserInputProcess(Engine::State &state)
  : Engine::Process(state, std::chrono::microseconds(1000))
  , m_pExpander(shared_ptr<IOExpander>(new IOExpander()))
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(ML_INTERRUPT_PIN, Pin::IN)))
{
  // Configure interrupt pin and clear initial interrupt
  m_pInterruptIn->SetEdgeMode(Pin::FALLING);
  m_pInterruptIn->Read();

  // Configure Expander
  m_pExpander->Configure(ExpanderConfig);

  // Clear interrupt state
  m_pExpander->ReadCapturedInterruptState();

  // Register handlers
  auto playButton = unique_ptr<Button>(new Button(PLAY_BUTTON));
  playButton->onTriggered = [=]() {
    if (onPlayStop) {
      onPlayStop();
    }
  };
  m_controls.push_back(std::move(playButton));

  auto tapButton = unique_ptr<Button>(new Button(TAP_BUTTON));
  tapButton->onTriggered = [=]() {
    if (onTapTempo) {
      onTapTempo();
    }
  };
  m_controls.push_back(std::move(tapButton));

  auto encoderButton = unique_ptr<Button>(new Button(ENC_BUTTON, chrono::milliseconds(5), chrono::milliseconds(20)));
  encoderButton->onTriggered = [=]() {
    if (onEncoderPress) {
      onEncoderPress();
    }
  };
  m_controls.push_back(std::move(encoderButton));

  auto encoder = unique_ptr<RotaryEncoder>(new RotaryEncoder(ENC_A, ENC_B));
  encoder->onRotated = [=](float amount) {
    if (onEncoderRotate) {
      onEncoderRotate(amount);
    }
  };
  m_controls.push_back(std::move(encoder));
}

void UserInputProcess::process() {
  pollfd pfd = m_pInterruptIn->GetPollInfo();

  // If interrupt is already low, handle immediately
  if (m_pInterruptIn->Read() == GPIO::LOW) {
    handleInterrupt();
    return;
  }

  // Poll for 500ms
  int result = ::poll(&pfd, 1, 500);

  // No interrupts? try again
  if (result == 0) { return; }

  // Clear interrupt event
  m_pInterruptIn->Read();

  // Handle it
  handleInterrupt();
}

void UserInputProcess::handleInterrupt() {
  uint8_t flag = m_pExpander->ReadInterruptFlag();
  uint8_t state = m_pExpander->ReadGPIO();

  for (auto &control : m_controls) {
    if (control->CanHandleInterrupt(flag)) {
      control->HandleInterrupt(flag, state, m_pExpander);
    }
  }
}
