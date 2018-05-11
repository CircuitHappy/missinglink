/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <chrono>
#include "missing_link/view.hpp"

using namespace MissingLink;

namespace MissingLink {

}

MainView::MainView()
  : m_pLEDDriver(std::unique_ptr<LEDDriver>(new LEDDriver()))
  , m_pDisplay(std::unique_ptr<SegmentDisplay>(new SegmentDisplay()))
{
  m_pLEDDriver->Configure();
  m_pDisplay->Init();
}

MainView::~MainView() {}

void MainView::SetAnimationLEDs(const float frame[NumAnimLEDs]) {
  for (int i = 0; i < NumAnimLEDs; i ++) {
    m_pLEDDriver->SetBrightness(frame[i], ANIM_LED_START + i);
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
    m_pDisplay->Write(string);
  }
}

void MainView::WriteDisplayTemporarily(const std::string &string, int millis) {
  ScopedMutex lock(m_displayMutex);
  m_tempDisplayValues.push(string);
  m_tempMessageExpiration = Clock::now() + std::chrono::milliseconds(millis);
  m_pDisplay->Write(string);
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
  }
}

void MainView::displayWifiStatusFrame(float frame) {
  m_pLEDDriver->SetBrightness(frame, WIFI_LED);
}
