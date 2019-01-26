/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <chrono>
#include "missing_link/view.hpp"
#include "missing_link/hw_defs.h"
#include "missing_link/types.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;

namespace MissingLink {

}

MainView::MainView()
  : m_pLEDDriver(std::unique_ptr<LEDDriver>(new LEDDriver()))
  , m_pDisplay(std::unique_ptr<SegmentDisplay>(new SegmentDisplay()))
  , m_pLogoLight(std::unique_ptr<Pin>(new Pin(ML_LOGO_PIN, Pin::OUT)))
{
  m_pLEDDriver->Configure();
  m_pDisplay->Init();
}

MainView::~MainView() {}

void MainView::SetAnimationLEDs(const float frame[NumAnimLEDs], const float brightness, const bool dontDimFullBright) {
  for (int i = 0; i < NumAnimLEDs; i ++) {
    if (dontDimFullBright && (frame[i] == 1.0)) {
      m_pLEDDriver->SetBrightness(frame[i], ANIM_LED_START + i);
    } else {
      m_pLEDDriver->SetBrightness(frame[i] * brightness, ANIM_LED_START + i);
    }
  }
}

void MainView::ClearAnimationLEDs() {
  for (int i = 0; i < NumAnimLEDs; i ++) {
    m_pLEDDriver->SetBrightness(0.01, ANIM_LED_START + i);
  }
}

void MainView::WriteDisplay(const std::string &string, bool force) {
  ScopedMutex lock(m_displayMutex);
  m_displayValue = string;
  if (force) {
    m_tempDisplayValues = std::stack<std::string>();
    m_scrollTempMessage = false;
    m_pDisplay->Write(string);
  }
}

void MainView::WriteDisplayTemporarily(const std::string &string, int millis, bool scrolling) {
  ScopedMutex lock(m_displayMutex);
  m_scrollTempMessage = scrolling;
  if (scrolling) {
    m_scrollOffset = 0;
    m_tempScrollingMessage = string;
  }
  m_tempDisplayValues.push(string);
  m_tempMessageExpiration = Clock::now() + std::chrono::milliseconds(millis);
  m_pDisplay->Write(string);
}

void MainView::ScrollTempMessage() {
  if (m_scrollTempMessage) {
    auto now = Clock::now();
    if (now >= m_lastTempMessageFrame) {
      m_lastTempMessageFrame = now + std::chrono::milliseconds(150);
      int messageLength = m_tempScrollingMessage.length();
      m_scrollOffset ++;
      if ((m_scrollOffset + 3) > (messageLength - 1)) m_scrollOffset = 0;
      m_tempDisplayValues.push(m_tempScrollingMessage.substr(m_scrollOffset, 4));
    }
  }
}

void MainView::ClearDisplay() {
  ScopedMutex lock(m_displayMutex);
  m_tempDisplayValues = std::stack<std::string>();
  m_displayValue = "";
  m_pDisplay->Clear();
}

void MainView::UpdateDisplay() {
  ScopedMutex lock(m_displayMutex);
  auto now = Clock::now();
  if (m_tempDisplayValues.empty()) {
    m_pDisplay->Write(m_displayValue);
    return;
  }
  m_pDisplay->Write(m_tempDisplayValues.top());
  if (now >= m_tempMessageExpiration) {
    m_tempDisplayValues = std::stack<std::string>();
    m_scrollTempMessage = false;
  }
}

void MainView::displayWifiStatusFrame(float frame) {
  m_pLEDDriver->SetBrightness(frame, WIFI_LED);
}

void MainView::setLogoLight(double phase) {
  if (phase < 0.75) {
    m_pLogoLight->Write(HIGH);
  } else {
    m_pLogoLight->Write(LOW);
  }
}
