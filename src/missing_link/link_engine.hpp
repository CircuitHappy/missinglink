#pragma once

#include <ableton/link.hpp>
#include "missing_link/gpio.hpp"

namespace MissingLink {

  class LinkEngine {

  public:

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

    struct Pins {
      GPIO::SysfsPin clockOut;
      GPIO::SysfsPin resetOut;
      GPIO::SysfsPin playingOut;
      GPIO::SysfsPin playStopIn;

      Pins();
      void ExportAll();
      void UnexportAll();
    };

    struct State {
      ableton::Link link;
      std::atomic<bool> running;
      std::atomic<PlayState> playState;
      GPIO::Button playStop;
      Pins &pins;

      State(Pins &pins);
    };

    void runInput(State& state);
    void runOutput(State& state);
    void runDisplaySocket(State& state);
    void outputClock(Pins &pins, double beats, double phase, double tempo);
  };

}
