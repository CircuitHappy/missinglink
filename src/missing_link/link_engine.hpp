/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <ableton/link.hpp>

namespace MissingLink {

  class LinkEngine {

  public:

    LinkEngine();
    void Run();

  private:

    static constexpr double PULSES_PER_BEAT = 4.0;
    static constexpr double PULSE_LENGTH = 0.030; // seconds
    static constexpr double QUANTUM = 4;

    enum PlayState {
        Stopped,
        Cued,
        Playing
    };

    struct State {
      std::atomic<bool> running;
      std::atomic<PlayState> playState;
      State();
    };

    State m_state;

    ableton::Link m_link;

    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;

    std::chrono::microseconds m_lastOutputTime;

    void runOutput();
    void runDisplaySocket();
  };

}
