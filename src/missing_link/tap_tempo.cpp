/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <iostream>
#include "missing_link/tap_tempo.hpp"

using namespace std::chrono;
using namespace MissingLink;

TapTempo::TapTempo() : m_tapCount(0) {}

TapTempo::~TapTempo() {}

void TapTempo::Tap() {
  const auto now = steady_clock::now();
  const auto intervalSinceLast = now - m_previousTapTime;

  // If more than 1.5 seconds have gone by since last tap, reset
  if (m_tapCount == 0 || intervalSinceLast >= milliseconds(1500)) {
    m_tapCount = 1;
    m_startTapTime = now;
    m_previousTapTime = now;
    return;
  }

  m_tapCount += 1;

  const auto totalInterval = now - m_startTapTime;
  const auto averageInterval = totalInterval / (double)(m_tapCount - 1);
  const double newTempo = (int)((seconds(60)/averageInterval) + 0.5);

  if (onNewTempo) {
    onNewTempo(newTempo);
  }

  m_previousTapTime = now;
}
