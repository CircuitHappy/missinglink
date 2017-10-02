/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
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

namespace {

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

UserInterface::UserInterface()
  : onInputEvent(nullptr)
  , m_pExpander(unique_ptr<IOExpander>(new IOExpander()))
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(ML_INTERRUPT_PIN, Pin::IN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
  , m_lastEncSeq(0)
  , m_encVal(0)
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

UserInterface::~UserInterface() {
  StopPollingInput();
}

void UserInterface::StartPollingInput() {
  if (m_pPollThread != nullptr) { return; }
  m_bStopPolling = false;
  m_pPollThread = unique_ptr<thread>(new thread(&UserInterface::runPollInput, this));
}

void UserInterface::StopPollingInput() {
  if (m_pPollThread == nullptr) { return; }
  m_bStopPolling = true;
  m_pPollThread->join();
  m_pPollThread = nullptr;
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

void UserInterface::runPollInput() {

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

void UserInterface::handleInterrupt() {
  uint8_t flag = m_pExpander->ReadInterruptFlag(IOExpander::PORTA);
  uint8_t state = m_pExpander->ReadPort(IOExpander::PORTA);

  int pinIndex = 0;
  while ((flag & 1) == 0) {
    pinIndex++;
    flag = flag >> 1;
  }

  // whether triggered pin is on
  bool isOn = IOExpander::PinIsOn(pinIndex, state);

  switch (pinIndex) {
    case PLAY_BUTTON:
      if (isOn) {
        std::cout << "Play" << std::endl;
        handleInputEvent(InputEvent::PLAY_STOP);
      }
      break;
    case TAP_BUTTON:
      if (isOn) {
        std::cout << "Tap" << std::endl;
        handleInputEvent(InputEvent::TAP_TEMPO);
      }
      break;
    case ENC_BUTTON:
      if (isOn) {
        std::cout << "Encoder Press" << std::endl;
        handleInputEvent(InputEvent::ENC_PRESS);
      }
      break;
    case ENC_A:
      decodeEncoder(isOn, IOExpander::PinIsOn(ENC_B, state));
      break;
    case ENC_B:
      decodeEncoder(IOExpander::PinIsOn(ENC_A, state), isOn);
      break;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void UserInterface::handleInputEvent(InputEvent event) {
    if (onInputEvent != nullptr) {
      onInputEvent(event);
    }
}

void UserInterface::decodeEncoder(bool aOn, bool bOn) {
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
      handleInputEvent(InputEvent::ENC_UP);
    } else {
      std::cout << "Encoder down" << std::endl;
      handleInputEvent(InputEvent::ENC_DOWN);
    }
    m_encVal = 0;
  }
}
