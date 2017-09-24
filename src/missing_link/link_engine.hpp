#pragma once

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

    GPIO::SysfsPin m_clockOut;
    GPIO::SysfsPin m_resetOut;
    GPIO::SysfsPin m_playingOut;
    GPIO::SysfsPin m_playStopIn;

    GPIO::Button m_btnPlayStop;

    std::chrono::microseconds m_lastOutputTime;

    void runInput();
    void runOutput();
    void runDisplaySocket();
  };

}
