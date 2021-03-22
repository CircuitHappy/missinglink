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
//#include "missing_link/midi_out.hpp"

namespace MissingLink {

  struct buffer_t {
    bool state;
    std::chrono::microseconds timestamp;
  };

  class OutputJack {

    public:

      enum output_mode_t {
        CLOCK_NORMAL       = 0,
        CLOCK_ALWAYS_ON    = 1,
        RESET_TRIGGER      = 2,
        RUN_HIGH           = 3,
      };

      OutputJack(Engine * pEngine, uint8_t pin, output_mode_t mode, uint8_t ppqn);
      void begin();
      // void setBuffer(const std::chrono::microseconds last, const std::chrono::microseconds now, Engine::PlayState playState, const double qnPhase, const double beat);
      void setJack(const double beat, const bool resetTrig, const Engine::PlayState playState);
      void writeToJack(bool state);
  private:

      Engine * m_pEngine;
      uint8_t m_trigHighCount;
      std::unique_ptr<GPIO::Pin> m_pGpioPin;
      output_mode_t m_outMode;
      uint8_t m_ppqn;
      bool m_currentState;
      int m_lastCalcNum;
      int m_sampleCount;

      bool calcClockTrig(double beat);
      bool calcResetTrig(double beat, double quantum);
  };

  class OutputProcess : public Engine::Process {

    public:

      OutputProcess(Engine &engine);
      void Run() override;

    private:

      void process() override;

      std::chrono::microseconds m_lastTime = std::chrono::microseconds(0);
      double m_lastBeat;
      bool m_transportStopped = true;

      std::unique_ptr<OutputJack> m_pOutA;
      std::unique_ptr<OutputJack> m_pOutB;

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
