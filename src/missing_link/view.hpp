/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <stack>
#include <string>
#include <chrono>
#include <mutex>
#include "missing_link/types.hpp"
#include "missing_link/display.hpp"
#include "missing_link/led_driver.hpp"
#include "missing_link/gpio.hpp"

namespace MissingLink {

  enum OutputLEDIndex {
    WIFI_LED        = 7,
    ANIM_LED_START  = 8 // start of 6 consecutive animation LEDs
  };

  class MainView {

    public:

      static constexpr int NumAnimLEDs = 6;

      MainView();
      virtual ~MainView();

      void SetAnimationLEDs(const float frame[NumAnimLEDs]);
      void ClearAnimationLEDs();

      // Set a value to be immediately written to the display.
      // Cancels any temporary messages if `force` is true.
      void WriteDisplay(const std::string &string, bool force = true);

      // Set a value to be written to the display for the given duration in ms
      // after which it will be reverted back to its previous value.
      void WriteDisplayTemporarily(const std::string &string, int millis, bool scrolling = false);

      void ScrollTempMessage();

      // Set the display to be cleared on the next update
      void ClearDisplay();

      // Update the display.
      void UpdateDisplay();

      // Draw a frame of the WiFi Status LED
      void displayWifiStatusFrame(float frame);

      void setLogoLight(double phase);

    private:

      TimePoint m_tempMessageExpiration;
      std::stack<std::string> m_tempDisplayValues;
      std::string m_displayValue;
      std::string m_tempScrollingMessage;
      TimePoint m_lastTempMessageFrame;
      bool m_scrollTempMessage;
      int m_scrollOffset;

      std::mutex m_displayMutex;

      std::unique_ptr<LEDDriver> m_pLEDDriver;
      std::unique_ptr<SegmentDisplay> m_pDisplay;

      std::unique_ptr<GPIO::Pin> m_pLogoLight;
  };
}
