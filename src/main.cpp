#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <ableton/Link.hpp>

using namespace std;

namespace {

  const double PULSES_PER_BEAT = 4.0;
  const double PULSE_LENGTH = 0.015; // seconds
  const double QUANTUM = 4;

  float clock_div = 1.0; // clock division for supporting volca/eurorack/etc, multiply by PULSES_PER_BEAT

  // for first command line argument to set clock division
  enum ClockDivModes {
      Sixteenth = 0,
      Eighth,
      Quarter,
      NUM_CLOCK_DIVS
  };

  int selectedClockDiv = Sixteenth;

  // Using WiringPi numbering scheme
  enum InPin {
      PlayStop = 1,
      ClockDiv = 10
  };

  enum OutPin {
      Clock = 25,
      Reset = 28,
      PlayIndicator = 4
  };

  enum PlayState {
      Stopped,
      Cued,
      Playing
  };

  struct State {
      ableton::Link link;
      std::atomic<bool> running;
      std::atomic<bool> playPressed;
      std::atomic<PlayState> playState;
      std::atomic<bool> clockDivPressed;

      State()
        : link(120.0)
        , running(true)
        , playPressed(false)
        , playState(Stopped)
        , clockDivPressed(false)
      {
        link.enable(true);
      }
  };

  void configurePins() {}

  void clearLine() {
      std::cout << "   \r" << std::flush;
      std::cout.fill(' ');
  }

  void printState(const std::chrono::microseconds time,
      const ableton::Link::Timeline timeline)
  {
      const auto beats = timeline.beatAtTime(time, QUANTUM);
      const auto phase = timeline.phaseAtTime(time, QUANTUM);
      std::cout << "tempo: " << timeline.tempo()
          << " | " << std::fixed << "beats: " << beats
          << " | " << std::fixed << "phase: " << phase;
      clearLine();
  }

  void outputClock(double beats, double phase, double tempo) {
      const double secondsPerBeat = 60.0 / tempo;

      // Fractional portion of current beat value
      double intgarbage;
      const auto beatFraction = std::modf(beats * PULSES_PER_BEAT * clock_div, &intgarbage);

      // Fractional beat value for which clock should be high
      const auto highFraction = PULSE_LENGTH / secondsPerBeat;

      const bool resetHigh = (phase <= highFraction);
      //digitalWrite(Reset, resetHigh ? HIGH : LOW);

      const bool clockHigh = (beatFraction <= highFraction);
      //digitalWrite(Clock, clockHigh ? HIGH : LOW);
  }

  void input(State& state) {
      while (state.running) {

          const bool clockDivPressed = false; //digitalRead(ClockDiv) == HIGH;
          const bool playPressed = false; //digitalRead(PlayStop) == HIGH;
          //if (playPressed && !state.playPressed) {
          //    switch (state.playState) {
          //        case Stopped:
          //            state.playState.store(Cued);
          //            break;
          //        case Cued:
          //        case Playing:
          //            state.playState.store(Stopped);
          //            break;
          //    }
          //}

          if (clockDivPressed && !state.clockDivPressed) {
              selectedClockDiv = (selectedClockDiv + 1) % NUM_CLOCK_DIVS;
              switch (selectedClockDiv) {
                  case Sixteenth:
                      clock_div = 1.0;
                      break;
                  case Eighth:
                      clock_div = 0.5;
                      break;
                  case Quarter:
                      clock_div = 0.25;
                      break;
                  default:
                      clock_div = 1.0;
                      break;
              }
          }
          state.playPressed.store(playPressed);
          state.clockDivPressed.store(clockDivPressed);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
  }

  void output(State& state) {
      while (state.running) {
          const auto time = state.link.clock().micros();
          auto timeline = state.link.captureAppTimeline();

          const double beats = timeline.beatAtTime(time, QUANTUM);
          const double phase = timeline.phaseAtTime(time, QUANTUM);
          const double tempo = timeline.tempo();

          switch ((PlayState)state.playState) {
              case Cued: {
                      // Tweak this
                      const bool playHigh = (long)(beats * 2) % 2 == 0;
                      //digitalWrite(PlayIndicator, playHigh ? HIGH : LOW);
                      if (phase <= 0.01) {
                          state.playState.store(Playing);
                      }
                  break;
              }
              case Playing:
                  //digitalWrite(PlayIndicator, HIGH);
                  outputClock(beats, phase, tempo);
                  break;
              default:
                  //digitalWrite(PlayIndicator, LOW);
                  break;
          }

          std::this_thread::sleep_for(std::chrono::microseconds(250));
      }
  }
}

int main(void) {
    configurePins();
    State state;

    std::thread inputThread(input, std::ref(state));
    std::thread outputThread(output, std::ref(state));

    while (state.running) {
        const auto time = state.link.clock().micros();
        auto timeline = state.link.captureAppTimeline();
        printState(time, timeline);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    inputThread.join();
    outputThread.join();

    return 0;
}

