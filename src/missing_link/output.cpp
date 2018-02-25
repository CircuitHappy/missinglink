/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <algorithm>
#include <thread>
#include <pthread.h>
#include <ableton/Link.hpp>
#include "missing_link/hw_defs.h"
#include "missing_link/types.hpp"
#include "missing_link/gpio.hpp"
#include "missing_link/view.hpp"
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;
using std::min;
using std::max;

namespace MissingLink {

  static const double ResetPulseLength = 0.050; // seconds

  static const int NumAnimationFrames = 4;

  static const float CueAnimationFrames[][6] =  {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {1, 1, 0.1, 0.1, 0.1, 1},
    {1, 1, 1, 0.1, 1, 1},
    {1, 1, 1, 1, 1, 1}
  };

  static const float PlayAnimationFrames[][6] = {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {0.1, 1, 1, 0.1, 0.1, 0.1},
    {0.1, 0.1, 0.1, 1, 0.1, 0.1},
    {0.1, 0.1, 0.1, 0.1, 1, 1}
  };

  struct OutputModel {

    bool isFirstClock;
    bool clockHigh;
    bool resetHigh;
    int animFrameIndex;

    OutputModel(ableton::Link &link, Settings &settings) {
      auto timeline = link.captureAudioTimeline();

      const auto now = link.clock().micros();
      const double tempo = timeline.tempo();
      const double beats = timeline.beatAtTime(now, settings.quantum);
      const double phase = timeline.phaseAtTime(now, settings.quantum);

      const int edgesPerBeat = settings.ppqn * 2;
      const int edgesPerLoop = edgesPerBeat * settings.quantum;
      const int currentEdges = (int)floor(beats * (double)edgesPerBeat);
      isFirstClock = (currentEdges % edgesPerLoop) == 0;
      clockHigh = currentEdges % 2 == 0;

      const double secondsPerPhrase = 60.0 / (tempo / settings.quantum);
      const double resetHighFraction = ResetPulseLength / secondsPerPhrase;
      resetHigh = (phase <= resetHighFraction);

      const double normalizedPhase = min(1.0, max(0.0, phase / (double)settings.quantum));
      const int totalFrames = NumAnimationFrames;
      animFrameIndex = min(totalFrames - 1, max(0, (int)floor(normalizedPhase * totalFrames)));
    }
  };
}

OutputProcess::OutputProcess(Engine::State &state, std::shared_ptr<MainView> pView)
  : Engine::Process(state)
  , m_pClockOut(std::unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(std::unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
  , m_pView(pView)
{
  m_pClockOut->Write(LOW);
  m_pResetOut->Write(LOW);
}

void OutputProcess::Run() {
  Process::Run();
  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(m_pThread->native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }
}

void OutputProcess::process() {

  if (m_state.playState == PlayState::Stopped) {
    setClock(false);
    setReset(false);
    m_pView->ClearAnimationLEDs();
    sleep();
    return;
  }

  auto settings = m_state.settings.load();
  OutputModel model(m_state.link, settings);

  switch (m_state.playState) {
    case Cued:
      // start playing on first clock of loop
      if (model.isFirstClock) {
        m_state.playState = Playing;
        // Deliberate fallthrough here
      } else {
        m_pView->SetAnimationLEDs(CueAnimationFrames[model.animFrameIndex]);
        break;
      }
    case Playing: {
      setReset(model.resetHigh);
      setClock(model.clockHigh);
      m_pView->SetAnimationLEDs(PlayAnimationFrames[model.animFrameIndex]);
      break;
    }
    default:
      break;
  }

  sleep();
}

void OutputProcess::sleep() {
  auto sleepTime = std::chrono::microseconds(500);
  std::this_thread::sleep_for(sleepTime);
}

void OutputProcess::setClock(bool high) {
  if (m_clockHigh == high) { return; }
  m_clockHigh = high;
  m_pClockOut->Write(high ? HIGH : LOW);
  m_pView->SetClockLED(high);
}

void OutputProcess::setReset(bool high) {
  if (m_resetHigh == high) { return; }
  m_resetHigh = high;
  m_pResetOut->Write(high ? HIGH : LOW);
  m_pView->SetResetLED(high);
}
