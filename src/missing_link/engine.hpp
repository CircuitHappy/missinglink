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
#include "missing_link/midi_out.hpp"
#include "missing_link/system_info.hpp"
#include "missing_link/file_io.hpp"

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
        Clock,
        ResetMode,
        DelayCompensation,
        StartStopSync,
        ApMode,
        ApResetScroll,
        DisplayIP,
        RebootScroll,
        NUM_MODES
      };

      /// Model for engine output processes
      struct OutputModel {
        std::chrono::microseconds now;
        double tempo;
        bool clockTriggered;
        bool resetTriggered;
        bool midiClockTriggered;
      };

      class Process {

        public:

          Process(Engine &engine, std::chrono::microseconds LowPrioritySleepTime, std::chrono::microseconds HiPrioritySleepTime);
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
          std::chrono::microseconds m_LowPriSleepTime;
          std::chrono::microseconds m_HiPriSleepTime;
          std::atomic<bool> m_bStopped;
      };

      Engine();

      void Run();

      const bool isRunning() const { return m_running; }
      const double GetNormalizedPhase() const;
      const double GetBeatPhase() const;
      const int GetNumberOfPeers() const;
      const OutputModel GetOutputModel(std::chrono::microseconds last) const;

      PlayState GetPlayState() const { return m_playState.load(); }
      void SetPlayState(PlayState state);

      bool GetQueuedStartTransport();

      int getWifiStatus();
      int getResetMode();

      std::shared_ptr<MidiOut> GetMidiOut();
      std::shared_ptr<MainView> GetMainView();

    private:

      std::atomic<bool> m_running;
      std::atomic<PlayState> m_playState;
      std::atomic<WifiState> m_wifiStatus;
      std::shared_ptr<WifiStatus> m_pWifiStatusFile;
      std::unique_ptr<FileIO::TextFile> m_pApModeFile;
      std::atomic<Settings> m_settings;
      std::atomic<InputMode> m_inputMode;

      ableton::Link m_link;

      std::shared_ptr<MainView> m_pView;
      std::unique_ptr<TapTempo> m_pTapTempo;
      std::shared_ptr<MidiOut> m_pMidiOut;
      std::atomic<bool> m_QueueStartTransport;
      std::string m_currIpAddr;
      std::atomic<int> m_currIpAddrViewSegment;
      std::atomic<int> m_rebootScrollPosition;
      std::atomic<int> m_apResetScrollPosition;
      std::vector<std::unique_ptr<Process>> m_processes;

      SysInfo sysInfo;

      void playStop();
      void queueStartTransportAtLoopStart();
      void zeroTimeline();
      void toggleMode();
      void startTimeline();
      void stopTimeline();
      void setTempo(double tempo);

      void routeEncoderAdjust(float amount);
      void tempoAdjust(float amount);
      void loopAdjust(int amount);
      void ppqnAdjust(int amount);
      void resetModeAdjust(int amount);
      void delayCompensationAdjust(int amount);
      void StartStopSyncAdjust(float amount);
      void apModeAdjust(int amount);
      void apResetScrollAdjust(int amount);
      void rebootScrollAdjust(int amount);
      void ipAddressAdjust(int amount);

      void displayCurrentMode();
      void displayTempWifiStatus(WifiState status);
      void displayTempo(double tempo, bool force);
      void displayQuantum(int quantum, bool force);
      void displayPPQN(int ppqn, bool force);
      void displayResetMode(int mode, bool force);
      void displayApMode(int mode, bool force);
      void displayApResetMenu(int mode, bool force);
      void displayRebootMenu(int mode, bool force);
      void displayDelayCompensation(int delay, bool force);
      void displayStartStopSync(bool sync, bool force);
      void displayIpAddrSegment(int pos, bool force);

      double getCurrentTempo() const;
      int getCurrentQuantum() const;
      int getCurrentPPQN() const;
      int getCurrentResetMode() const;
      int getCurrentDelayCompensation() const;
      int getCurrentStartStopSync() const;
      int getCurrentApMode() const;

      void updateApModeFile();
      void startResetApModeSettings();
      void startRebootProcess();

      TimePoint m_lastToggle;
  };


}
