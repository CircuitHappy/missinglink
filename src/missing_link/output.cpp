/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <thread>
#include <pthread.h>
#include <ableton/Link.hpp>
#include "missing_link/hw_defs.h"
#include "missing_link/gpio.hpp"
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;

namespace {
  static const double ResetPulseLength = 0.050; // seconds
}

OutputLoop::OutputLoop(Engine::State &state)
  : Engine::Process(state)
  , m_pClockOut(std::unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(std::unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  m_pClockOut->Write(LOW);
  m_pResetOut->Write(LOW);
}

void OutputLoop::Run() {
  Process::Run();
  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(m_pThread->native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }
}

void OutputLoop::process() {

  if (m_state.playState == Engine::PlayState::Stopped) {
    setClock(false);
    setReset(false);
    //m_pUI->ClearAnimationLEDs();
    sleep();
    return;
  }

  auto timeline = m_state.link.captureAudioTimeline();
  auto settings = m_state.settings.load();

  const auto now = m_state.link.clock().micros();
  const double tempo = timeline.tempo();
  const double beats = timeline.beatAtTime(now, settings.quantum);
  const double phase = timeline.phaseAtTime(now, settings.quantum);

  const int edgesPerBeat = settings.ppqn * 2;
  const int edgesPerLoop = edgesPerBeat * settings.quantum;
  const int currentEdges = (int)floor(beats * (double)edgesPerBeat);

  //const double normalizedPhase = min(1.0, max(0.0, phase / (double)settings.quantum));
  //const int animFrameIndex = min(NUM_ANIM_FRAMES - 1, max(0, (int)floor(normalizedPhase * NUM_ANIM_FRAMES)));

  switch ((Engine::PlayState)m_state.playState) {
    case Engine::PlayState::Cued:
      //set state to playing if there are no peers or we are at the starting edge of loop
      if ( currentEdges % edgesPerLoop == 0 ) {
        m_state.playState = Engine::PlayState::Playing;
        // Deliberate fallthrough here
      } else {
        //m_pUI->SetAnimationLEDs(CueAnimationFrames[animFrameIndex]);
        break;
      }
    case Engine::PlayState::Playing: {
      const double secondsPerPhrase = 60.0 / (tempo / settings.quantum);
      const double resetHighFraction = ResetPulseLength / secondsPerPhrase;
      const bool resetHigh = (phase <= resetHighFraction);
      setReset(resetHigh);

      const bool clockHigh = currentEdges % 2 == 0;
      setClock(clockHigh);
        //m_pUI->SetAnimationLEDs(PlayAnimationFrames[animFrameIndex]);
      break;
    }
    default:
      break;
  }

  sleep();
}

void OutputLoop::sleep() {
  auto sleepTime = std::chrono::microseconds(500);
  std::this_thread::sleep_for(sleepTime);
}

void OutputLoop::setClock(bool high) {
  if (m_clockHigh == high) { return; }
  m_clockHigh = high;
  m_pClockOut->Write(high ? HIGH : LOW);
}

void OutputLoop::setReset(bool high) {
  if (m_resetHigh == high) { return; }
  m_resetHigh = high;
  m_pResetOut->Write(high ? HIGH : LOW);
}
