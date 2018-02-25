/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <cmath>
#include "missing_link/control.hpp"

using namespace std;
using namespace MissingLink;

Control::Control(vector<int> pinIndices)
{
  for (auto index : pinIndices) {
    m_flagMask |= (1 << index);
  }
}

Control::~Control() {}

void Control::HandleInterrupt(uint8_t flag, uint8_t state, shared_ptr<IOExpander> pExpander) {
  handleInterrupt(flag, state, pExpander);
}

Button::Button(int pinIndex,
               chrono::milliseconds debounceInterval,
               chrono::milliseconds minRepeatInterval)
  : Control({ pinIndex })
  , m_debounceInterval(debounceInterval)
  , m_minRepeatInterval(minRepeatInterval)
{}

Button::~Button() {}

void Button::handleInterrupt(uint8_t flag, uint8_t state, shared_ptr<IOExpander> pExpander) {

  // check change to ON
  if ((flag & state) == 0) {
    return;
  }

  auto now = chrono::steady_clock::now();
  if ((now - m_lastTriggered) < m_minRepeatInterval) {
    return;
  }

  // sleep to debounce
  this_thread::sleep_for(chrono::milliseconds(m_debounceInterval));

  // check again to make sure it's still on
  if ((pExpander->ReadGPIO() | flag) == 0) {
    return;
  }

  if (onTriggered) {
    onTriggered();
  }

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

  float rotationAmount = 0.0;
  if (std::abs(m_encVal) >= 4) {

    float acc = 1.0;
    auto now = std::chrono::steady_clock::now();
    auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTriggered).count();
    if (interval > 0) {
      float factor = fmin(1.0, fmax(0.0, (100.0 - (float)interval)/100.0));
      acc += factor * 3.0;
    }
    m_lastTriggered = now;

    if (m_encVal > 0) {
      rotationAmount = 1.0 * acc;
    } else {
      rotationAmount = -1.0 * acc;
    }
    m_encVal = 0;
  }

  if (rotationAmount && onRotated) {
    onRotated(rotationAmount);
  }
}

