/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <thread>
#include <ableton/Link.hpp>
#include "missing_link/tap_tempo.hpp"
#include "missing_link/user_interface.hpp"

namespace MissingLink {

  class LinkEngine {

    public:

      LinkEngine();
      void Run();

    private:

      static constexpr double PULSE_LENGTH = 0.030; // seconds

      enum PlayState {
          Stopped,
          Cued,
          Playing
      };

      struct State {
        std::atomic<bool> running;
        std::atomic<PlayState> playState;
        std::atomic<UserInterface::EncoderMode> encoderMode;
        ableton::Link link;
        std::atomic<int> quantum;
        std::atomic<int> pulsesPerQuarterNote;
        State();
      };

      State m_state;
      std::shared_ptr<UserInterface> m_pUI;
      std::unique_ptr<TapTempo> m_pTapTempo;
      std::chrono::microseconds m_lastOutputTime;

      void runOutput();
      void runDisplayLoop();

      void playStop();
      void toggleMode();
      void formatDisplayValue(char *display);
      void routeEncoderAdjust(float amount);
      void resetTimeline();
      void tempoAdjust(float amount);
      void loopAdjust(int amount);
      void ppqnAdjust(int amount);
      void setTempo(double tempo);
  };

}
