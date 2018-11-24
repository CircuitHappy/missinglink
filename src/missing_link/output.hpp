/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include "missing_link/gpio.hpp"
#include "missing_link/view.hpp"
#include "missing_link/midi_out.hpp"

namespace MissingLink {

  class OutputProcess : public Engine::Process {

    public:

      OutputProcess(Engine &engine);
      void Run() override;

    private:

      void process() override;
      void triggerOutputs(bool clockTriggered, bool resetTriggered);
      void setClock(bool high);
      void setReset(bool high);


      std::chrono::microseconds m_lastOutTime = std::chrono::microseconds(0);
      bool m_clockHigh = false;
      bool m_resetHigh = false;

      std::unique_ptr<GPIO::Pin> m_pClockOut;
      std::unique_ptr<GPIO::Pin> m_pResetOut;
      std::unique_ptr<GPIO::Pin> m_pLogoLight;

      std::unique_ptr<MidiOut> m_pMidiOut;
  };

  class ViewUpdateProcess : public Engine::Process {

    public:

      ViewUpdateProcess(Engine &engine, std::shared_ptr<MainView> pView);

    private:

      void process() override;
      void animatePhase(float normalizedPhase, Engine::PlayState playState);

      float getWifiStatusFrame(int wifiStatus);

      std::shared_ptr<MainView> m_pView;
  };

};
