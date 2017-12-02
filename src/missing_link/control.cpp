#include <iostream>
#include "missing_link/control.hpp"

using namespace std;
using namespace MissingLink;

Control::Control(vector<int> pinIndices)
  : ExpanderInputLoop::InterruptHandler(pinIndices)
{}

Control::~Control() {}

Button::Button(int pinIndex, chrono::milliseconds debounceInterval)
  : Control({ pinIndex })
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
  : Control({ pinIndexA, pinIndexB })
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


