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
#include "missing_link/wifi_status.hpp"

namespace MissingLink {

  class Engine {

    public:

      enum class PlayState {
        Stopped,
        Cued,
        Playing,
        CuedStop
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

      class Process {

        public:

          Process(Engine &engine, std::chrono::microseconds sleepTime);
          virtual ~Process();

          virtual void Run();
          void Stop();

          bool IsRunning() const { return !m_bStopped; }

        protected:

          Engine &m_engine;
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

      const bool isRunning() const { return m_running; }
      const double GetNormalizedPhase() const;
      const OutputModel GetOutputModel(std::chrono::microseconds last) const;

      PlayState GetPlayState() const { return m_playState.load(); }
      void SetPlayState(PlayState state) { m_playState = state; };

      int getWifiStatus();

    private:

      std::atomic<bool> m_running;
      std::atomic<PlayState> m_playState;
      std::atomic<WifiState> m_wifiStatus;
      std::shared_ptr<WifiStatus> m_pWifiStatusFile;
      std::atomic<Settings> m_settings;
      std::atomic<InputMode> m_inputMode;

      ableton::Link m_link;

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
      void displayTempWifiStatus(WifiState status);
      void displayTempo(double tempo, bool force);
      void displayQuantum(int quantum, bool force);
      void displayPPQN(int ppqn, bool force);

      double getCurrentTempo() const;
      int getCurrentQuantum() const;
      int getCurrentPPQN() const;

      TimePoint m_lastToggle;
  };


}
