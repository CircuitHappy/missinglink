/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <thread>
#include <ableton/link.hpp>
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

      std::chrono::microseconds m_lastOutputTime;

      void runOutput();
      void runDisplayLoop();

      void playStop();
      void toggleMode();
      void formatDisplayValue(char *display);
      void routeEncoderAdjust(float amount);
      void resetTimeline(const std::chrono::microseconds hostTime);
      void tempoAdjust(float amount);
      void loopAdjust(int amount);
      void ppqnAdjust(int amount);
  };

}
