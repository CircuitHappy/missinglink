#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <signal.h>
#include <pthread.h>

#include <ableton/Link.hpp>

#include "missing_link/pin_defs.hpp"
#include "missing_link/gpio.hpp"

using namespace std;
using namespace MissingLink;

namespace {

  const double PULSES_PER_BEAT = 4.0;
  const double PULSE_LENGTH = 0.030; // seconds
  const double QUANTUM = 4;

  struct Pins {

    GPIO::SysfsPin clockOut;
    GPIO::SysfsPin resetOut;
    GPIO::SysfsPin playingOut;
    GPIO::SysfsPin playStopIn;

    Pins()
      : clockOut(GPIO::SysfsPin(GPIO::CHIP_D0, GPIO::Pin::OUT))
      , resetOut(GPIO::SysfsPin(GPIO::CHIP_D1, GPIO::Pin::OUT))
      , playingOut(GPIO::SysfsPin(GPIO::CHIP_D6, GPIO::Pin::OUT))
      , playStopIn(GPIO::SysfsPin(GPIO::CHIP_D7, GPIO::Pin::IN))
    {}

    void ExportAll() {
      clockOut.Export();
      resetOut.Export();
      playingOut.Export();
      playStopIn.Export();
    }

    void UnexportAll() {
      clockOut.Unexport();
      resetOut.Unexport();
      playingOut.Unexport();
      playStopIn.Unexport();
    }
  };

  enum PlayState {
      Stopped,
      Cued,
      Playing
  };

  struct State {
      ableton::Link link;
      std::atomic<bool> running;
      std::atomic<PlayState> playState;

      GPIO::Button playStop;

      Pins &pins;

      State(Pins &pins)
        : link(120.0)
        , running(true)
        , playState(Stopped)
        , playStop(GPIO::Button(pins.playStopIn))
        , pins(pins)
      {
        link.enable(true);
      }
  };

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

  void outputClock(const Pins &pins, double beats, double phase, double tempo) {

      // Fractional portion of current beat value
      double intgarbage;
      const double beatFraction = std::modf(beats * PULSES_PER_BEAT, &intgarbage);

      const double secondsPerPhrase = 60.0 / (tempo / QUANTUM);
      const double resetHighFraction = PULSE_LENGTH / secondsPerPhrase;
      const bool resetHigh = (phase <= resetHighFraction);
      pins.resetOut.Write(resetHigh ? HIGH : LOW);

      // Fractional beat value for which clock should be high
      const double secondsPerDivision = 60.0 / (tempo * PULSES_PER_BEAT);
      const double clockHighFraction = PULSE_LENGTH / secondsPerDivision;
      const bool clockHigh = (beatFraction <= clockHighFraction);
      pins.clockOut.Write(clockHigh ? HIGH : LOW);
  }

  void input(State& state) {
    while (state.running) {

      state.playStop.Process();

      if (state.playStop.GetCurrentEvent() == GPIO::Button::TOUCH_DOWN) {
        switch ((PlayState)state.playState) {
          case Stopped:
            state.playState.store(Cued);
            break;
          case Cued:
          case Playing:
            state.playState.store(Stopped);
            break;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void output(State& state) {
      while (state.running) {
          const auto time = state.link.clock().micros();
          auto timeline = state.link.captureAudioTimeline();

          const double beats = timeline.beatAtTime(time, QUANTUM);
          const double phase = timeline.phaseAtTime(time, QUANTUM);
          const double tempo = timeline.tempo();

          switch ((PlayState)state.playState) {
              case Cued: {
                  const bool playHigh = (long)(beats * 2) % 2 == 0;
                  state.pins.playingOut.Write(playHigh ? HIGH : LOW);
                  if (phase <= 0.01) {
                      state.playState.store(Playing);
                  }
                  break;
              }
              case Playing:
                  state.pins.playingOut.Write(HIGH);
                  outputClock(state.pins, beats, phase, tempo);
                  break;
              default:
                  state.pins.playingOut.Write(LOW);
                  state.pins.clockOut.Write(LOW);
                  state.pins.resetOut.Write(LOW);
                  break;
          }

          std::this_thread::sleep_for(std::chrono::microseconds(500));
      }
  }
}

Pins pins;
State state(pins);

int main(void) {

  state.pins.UnexportAll();
  state.pins.ExportAll();

  std::thread inputThread(input, std::ref(state));
  std::thread outputThread(output, std::ref(state));

  while (state.running) {
    //const auto time = state.link.clock().micros();
    //auto timeline = state.link.captureAppTimeline();
    //printState(time, timeline);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  inputThread.join();
  outputThread.join();

  state.pins.UnexportAll();

  return 0;
}
