/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <chrono>
#include <memory>
#include "missing_link/view.hpp"

namespace MissingLink {

  class OutputLoop : public Engine::Process {

    public:

      OutputLoop(Engine::State &state, std::shared_ptr<MainView> pView);
      void Run() override;

    private:

      void process() override;
      void sleep();

      void setClock(bool high);
      void setReset(bool high);

      std::unique_ptr<GPIO::Pin> m_pClockOut;
      std::unique_ptr<GPIO::Pin> m_pResetOut;
      bool m_clockHigh = false;
      bool m_resetHigh = false;

      std::shared_ptr<MainView> m_pView;
  };

};
