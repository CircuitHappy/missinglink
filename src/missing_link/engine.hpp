/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <atomic>
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
        Playing,
        NUM_PLAY_STATES
      };

      enum InputMode {
        BPM,
        Loop,
        Clock,
        NUM_INPUT_MODES
      };

      struct State {
        std::atomic<bool> running;
        std::atomic<PlayState> playState;
        std::atomic<InputMode> inputMode;
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

          bool IsRunning() { return !m_bStopped; }

        protected:

          State &m_state;
          std::unique_ptr<std::thread> m_pThread;

          virtual void run();
          virtual void process() = 0;

        private:

          std::atomic<bool> m_bStopped;
      };

    private:

      static constexpr double PULSE_LENGTH = 0.030; // seconds

      State m_state;
      std::shared_ptr<UserInterface> m_pUI;
      std::unique_ptr<TapTempo> m_pTapTempo;

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
