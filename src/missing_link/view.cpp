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

void MainView::ShowInputModeName(InputMode mode) {
  std::chrono::system_clock::time_point TempMessageExpireTime = std::chrono::system_clock::now() + std::chrono::seconds(2);
  switch (mode) {
    case BPM:
      //ViewUpdateProcess::SetTempMessage("BPM", TempMessageExpireTime);
      break;
    case Loop:
      //ViewUpdateProcess::SetTempMessage("LOOP", TempMessageExpireTime);
      break;
    case Clock:
      //ViewUpdateProcess::SetTempMessage("PPQN", TempMessageExpireTime);
      break;
    default:
      return;
  };
}

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

void MainView::WriteDisplay(std::string const &string) {
  m_pDisplay->Write(string);
}

void MainView::ClearDisplay() {
  m_pDisplay->Clear();
}
