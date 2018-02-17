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

#define NUM_ANIM_LEDS     6

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

  enum OutputLEDIndex {
    CLOCK_LED       = 0,
    RESET_LED       = 1,
    BPM_MODE_LED    = 2,
    LOOP_MODE_LED   = 3,
    CLK_MODE_LED    = 4,
    WIFI_LED        = 5,
    ANIM_LED_START  = 8 // start of 6 consecutive animation LEDs
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
  , m_pLEDDriver(shared_ptr<LEDDriver>(new LEDDriver()))
  , m_pInputLoop(unique_ptr<ExpanderInputLoop>(new ExpanderInputLoop(m_pExpander, ML_INTERRUPT_PIN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  // Configure Expander
  m_pExpander->Configure(ExpanderConfig);

  // Configure LED Driver
  m_pLEDDriver->Configure();

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

void UserInterface::SetModeLED(EncoderMode mode) {
   // turn off all modes first
   m_pLEDDriver->SetBrightness(0.0, BPM_MODE_LED);
   m_pLEDDriver->SetBrightness(0.0, LOOP_MODE_LED);
   m_pLEDDriver->SetBrightness(0.0, CLK_MODE_LED);

   int ledIndex;
   switch (mode) {
     case BPM:
       ledIndex = BPM_MODE_LED;
       break;
     case LOOP:
       ledIndex = LOOP_MODE_LED;
       break;
     case CLOCK:
       ledIndex = CLK_MODE_LED;
       break;
     default:
       return;
   };
   m_pLEDDriver->SetBrightness(0.25, ledIndex);
}

void UserInterface::SetClock(bool on) {
  m_pClockOut->Write(on ? HIGH : LOW);
  m_pLEDDriver->SetBrightness(on ? 1.0 : 0.0, CLOCK_LED);
}

void UserInterface::SetReset(bool on) {
  m_pResetOut->Write(on ? HIGH : LOW);
  m_pLEDDriver->SetBrightness(on ? 1.0 : 0.0, RESET_LED);
}

void UserInterface::SetAnimationLEDs(const float frame[6]) {
  for (int i = 0; i < NUM_ANIM_LEDS; i ++) {
    m_pLEDDriver->SetBrightness(frame[i], ANIM_LED_START + i);
  }
}

void UserInterface::ClearAnimationLEDs() {
  for (int i = 0; i < NUM_ANIM_LEDS; i ++) {
    m_pLEDDriver->SetBrightness(0.1, ANIM_LED_START + i);
  }
}
