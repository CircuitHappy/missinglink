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

namespace MissingLink {

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
      void WriteDisplayTemporarily(const std::string &string, int millis);

      // Set the display to be cleared on the next update
      void ClearDisplay();

      // Update the display.
      void UpdateDisplay();

    private:

      TimePoint m_tempMessageExpiration;
      std::stack<std::string> m_tempDisplayValues;
      std::string m_displayValue;

      std::mutex m_displayMutex;

      std::unique_ptr<LEDDriver> m_pLEDDriver;
      std::unique_ptr<SegmentDisplay> m_pDisplay;
  };
}
