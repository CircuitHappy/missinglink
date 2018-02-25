/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <iomanip>
#include <vector>

#include "missing_link/hw_defs.h"
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"
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

UserInterface::UserInterface()
  : m_pExpander(shared_ptr<IOExpander>(new IOExpander()))
  , m_pInputLoop(unique_ptr<ExpanderInputLoop>(new ExpanderInputLoop(m_pExpander, ML_INTERRUPT_PIN)))
{
  // Configure Expander
  m_pExpander->Configure(ExpanderConfig);

  // Clear interrupt state
  m_pExpander->ReadCapturedInterruptState();

  // Register handlers
  auto playButton = shared_ptr<Button>(new Button(PLAY_BUTTON));
  playButton->onTriggered = [=]() {
    if (onPlayStop) {
      onPlayStop();
    }
  };

  auto tapButton = shared_ptr<Button>(new Button(TAP_BUTTON));
  tapButton->onTriggered = [=]() {
    if (onTapTempo) {
      onTapTempo();
    }
  };

  auto encoderButton = shared_ptr<Button>(new Button(ENC_BUTTON, chrono::milliseconds(5), chrono::milliseconds(20)));
  encoderButton->onTriggered = [=]() {
    if (onEncoderPress) {
      onEncoderPress();
    }
  };

  auto encoder = shared_ptr<RotaryEncoder>(new RotaryEncoder(ENC_A, ENC_B));
  encoder->onRotated = [=](float amount) {
    if (onEncoderRotate) {
      onEncoderRotate(amount);
    }
  };

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
