/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <chrono>
#include "missing_link/view.hpp"

using namespace MissingLink;

namespace MissingLink {

  enum OutputLEDIndex {
    WIFI_LED        = 5,
    ANIM_LED_START  = 8 // start of 6 consecutive animation LEDs
  };
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

void MainView::WriteDisplay(const std::string &string) {
  std::lock_guard<std::mutex> lock(m_displayMutex);
  m_displayValues = std::stack<std::string>();
  m_displayValues.push(string);
  m_pDisplay->Write(string);
}

void MainView::WriteDisplayTemporarily(const std::string &string, int millis) {
  std::lock_guard<std::mutex> lock(m_displayMutex);
  m_displayValues.push(string);
  m_tempMessageExpires = std::chrono::steady_clock::now() + std::chrono::milliseconds(millis);
  m_pDisplay->Write(string);
}

void MainView::ClearDisplay() {
  std::lock_guard<std::mutex> lock(m_displayMutex);
  m_displayValues = std::stack<std::string>();
  m_pDisplay->Clear();
}

void MainView::UpdateDisplay() {
  std::lock_guard<std::mutex> lock(m_displayMutex);
  auto now = std::chrono::steady_clock::now();
  if (m_displayValues.empty()) {
    return;
  }
  if (now >= m_tempMessageExpires && m_displayValues.size() > 1) {
    while (m_displayValues.size() > 1) {
      m_displayValues.pop();
    }
    m_pDisplay->Write(m_displayValues.top());
  }
}
