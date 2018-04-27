/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <chrono>
#include "missing_link/view.hpp"

using namespace MissingLink;

namespace MissingLink {

  enum OutputLEDIndex {
    CLOCK_LED       = 0,
    RESET_LED       = 1,
    BPM_MODE_LED    = 2,
    LOOP_MODE_LED   = 3,
    CLK_MODE_LED    = 4,
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

void MainView::SetClockLED(bool on) {
  m_pLEDDriver->SetBrightness(on ? 0.25 : 0.0, CLOCK_LED);
}

void MainView::SetResetLED(bool on) {
  m_pLEDDriver->SetBrightness(on ? 0.25 : 0.0, RESET_LED);
}

void MainView::ShowInputModeName(InputMode mode) {
  std::chrono::system_clock::time_point TempMessageExpireTime = std::chrono::system_clock::now() + std::chrono::seconds(2);
  switch (mode) {
    case BPM:
      ViewUpdateProcess::SetTempMessage("BPM", TempMessageExpireTime);
      break;
    case Loop:
      ViewUpdateProcess::SetTempMessage("LOOP", TempMessageExpireTime);
      break;
    case Clock:
      ViewUpdateProcess::SetTempMessage("PPQN", TempMessageExpireTime);
      break;
    default:
      return;
  };
}

void MainView::SetInputModeLED(InputMode mode) {
   // turn off all modes first
   m_pLEDDriver->SetBrightness(0.0, BPM_MODE_LED);
   m_pLEDDriver->SetBrightness(0.0, LOOP_MODE_LED);
   m_pLEDDriver->SetBrightness(0.0, CLK_MODE_LED);

   int ledIndex;
   switch (mode) {
     case BPM:
       ledIndex = BPM_MODE_LED;
       break;
     case Loop:
       ledIndex = LOOP_MODE_LED;
       break;
     case Clock:
       ledIndex = CLK_MODE_LED;
       break;
     default:
       return;
   };
   m_pLEDDriver->SetBrightness(0.25, ledIndex);
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
