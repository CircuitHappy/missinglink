/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include "missing_link/view.hpp"

namespace MissingLink {

  class OutputProcess : public Engine::Process {

    public:

      OutputProcess(Engine::State &state);
      void Run() override;

    private:

      void process() override;
      void setClock(bool high);
      void setReset(bool high);

      std::unique_ptr<GPIO::Pin> m_pClockOut;
      std::unique_ptr<GPIO::Pin> m_pResetOut;
      bool m_clockHigh = false;
      bool m_resetHigh = false;
  };

  class ViewUpdateProcess : public Engine::Process {

    public:

      ViewUpdateProcess(Engine::State &state, std::shared_ptr<MainView> pView);

    private:

      void process() override;
      void animatePhase(float normalizedPhase, PlayState playState);

      std::string formatDisplayValue(double tempo, const Settings &settings);

      std::shared_ptr<MainView> m_pView;
  };

};
