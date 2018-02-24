/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <string>
#include <thread>
#include <ableton/Link.hpp>
#include "missing_link/tap_tempo.hpp"
#include "missing_link/settings.hpp"
#include "missing_link/user_interface.hpp"

namespace MissingLink {

  class Engine {

    public:

      Engine();

      void Run();

      enum PlayState {
          Stopped,
          Cued,
          Playing
      };

      struct State {
        std::atomic<bool> running;
        std::atomic<PlayState> playState;
        std::atomic<UserInterface::EncoderMode> encoderMode;
        std::atomic<Settings> settings;
        ableton::Link link;
        State();
      };

      class Process {

        public:

          Process(State &state);
          virtual ~Process();

          virtual void Run();
          void Stop();

        protected:

          State &m_state;
          std::atomic<bool> m_bStopped;
          std::unique_ptr<std::thread> m_pThread;

          virtual void run() = 0;
      };

    private:

      static constexpr double PULSE_LENGTH = 0.030; // seconds

      State m_state;
      std::shared_ptr<UserInterface> m_pUI;
      std::unique_ptr<TapTempo> m_pTapTempo;
      std::chrono::microseconds m_lastOutputTime;

      void runOutput();
      void runDisplayLoop();

      void playStop();
      void toggleMode();
      std::string formatDisplayValue();
      void routeEncoderAdjust(float amount);
      void resetTimeline();
      void tempoAdjust(float amount);
      void loopAdjust(int amount);
      void ppqnAdjust(int amount);
      void setTempo(double tempo);
  };

}
