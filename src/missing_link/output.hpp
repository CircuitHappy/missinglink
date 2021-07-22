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
#include "missing_link/output_jack.hpp"
//#include "missing_link/midi_out.hpp"

namespace MissingLink {

  class OutputProcess : public Engine::Process {

    public:

      enum class PpqnItem {
        PPQN_1,
        PPQN_2,
        PPQN_4,
        PPQN_8,
        PPQN_12,
        PPQN_24,
        PPQN_MAX
      };

      enum class OutputMode {
        CLOCK_NORMAL_MODE,
        CLOCK_ALWAYS_ON_MODE,
        RESET_TRIGGER_MODE,
        GATE_MODE
      };

      OutputProcess(Engine &engine, std::shared_ptr<OutputJack> pJackA, std::shared_ptr<OutputJack> pJackB);
      void Run() override;

    private:

      void process() override;
      bool GetOutState(bool triggered, uint16_t clockNum, Engine::PlayState playState, int ppqn, OutputMode mode);
      void setOutA(bool high);
      void setOutB(bool high);
      OutputMode intToOutputMode(uint8_t mode);
      int OutputModeToInt(OutputMode mode);
      uint16_t GetPpqnValue(uint16_t index);
      uint16_t GetPpqnItemIndex(uint8_t ppqnValue);

      std::chrono::microseconds m_lastOutTime = std::chrono::microseconds(0);
      bool m_clockHigh = false;
      bool m_resetHigh = false;

      bool m_transportStopped = true;

      std::shared_ptr<OutputJack> m_pJackA;
      std::shared_ptr<OutputJack> m_pJackB;
      std::unique_ptr<GPIO::Pin> m_pClockOut;
      std::unique_ptr<GPIO::Pin> m_pResetOut;
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

  class JackProcess : public Engine::Process {

    public:

    JackProcess(Engine &engine, std::shared_ptr<OutputJack> pJack);
    void Run() override;

    private:

    std::shared_ptr<OutputJack> m_pJack;
    bool m_run;
    void process() override;

  };

};
