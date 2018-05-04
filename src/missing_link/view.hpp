/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <string>
#include "missing_link/types.hpp"
#include "missing_link/display.hpp"
#include "missing_link/led_driver.hpp"

namespace MissingLink {

  class MainView {

    public:

      static constexpr int NumAnimLEDs = 6;

      MainView();
      virtual ~MainView();

      void ShowInputModeName(InputMode mode);

      void SetAnimationLEDs(const float frame[NumAnimLEDs]);
      void ClearAnimationLEDs();

      void WriteDisplay(std::string const &string);
      void ClearDisplay();

    private:

      std::unique_ptr<LEDDriver> m_pLEDDriver;
      std::unique_ptr<SegmentDisplay> m_pDisplay;
  };
}
