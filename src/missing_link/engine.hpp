/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <atomic>
#include <vector>
#include <chrono>
#include <memory>
#include <thread>
#include <ableton/Link.hpp>
#include "missing_link/types.hpp"
#include "missing_link/tap_tempo.hpp"
#include "missing_link/settings.hpp"
#include "missing_link/view.hpp"

namespace MissingLink {

  class Engine {

    public:

      enum class PlayState {
        Stopped,
        Cued,
        Playing
      };

      enum class InputMode {
        BPM,
        Loop,
        Clock
      };

      /// Model for engine output processes
      struct OutputModel {
        std::chrono::microseconds now;
        double tempo;
        bool clockTriggered;
        bool resetTriggered;
      };

      struct State {

        std::atomic<bool> running;
        std::atomic<PlayState> playState;
        std::atomic<Settings> settings;
        ableton::Link link;

        State();

        const OutputModel getOutput(std::chrono::microseconds last);
        const double getNormalizedPhase();
      };

      class Process {

        public:

          Process(State &state, std::chrono::microseconds sleepTime);
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

          void sleep();
          std::chrono::microseconds m_sleepTime;
          std::atomic<bool> m_bStopped;
      };

      Engine();

      void Run();

    private:

      State m_state;
      InputMode m_inputMode;

      std::shared_ptr<MainView> m_pView;
      std::unique_ptr<TapTempo> m_pTapTempo;
      std::vector<std::unique_ptr<Process>> m_processes;

      void playStop();
      void toggleMode();
      void resetTimeline();
      void setTempo(double tempo);

      void routeEncoderAdjust(float amount);
      void tempoAdjust(float amount);
      void loopAdjust(int amount);
      void ppqnAdjust(int amount);

      void displayCurrentMode();
      void displayTempo(double tempo, bool force);
      void displayQuantum(int quantum, bool force);
      void displayPPQN(int ppqn, bool force);

      double getCurrentTempo() const;
      int getCurrentQuantum() const;
      int getCurrentPPQN() const;

      TimePoint m_lastToggle;
  };


}
