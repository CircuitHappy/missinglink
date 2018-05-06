/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <algorithm>
#include <thread>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <ableton/Link.hpp>
#include "missing_link/hw_defs.h"
#include "missing_link/types.hpp"
#include "missing_link/view.hpp"
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;
using std::min;
using std::max;

OutputProcess::OutputProcess(Engine &engine)
  : Engine::Process(engine, std::chrono::microseconds(500))
  , m_pClockOut(std::unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(std::unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
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

  auto playState = m_engine.GetPlayState();
  if (playState == Engine::PlayState::Stopped) {
    setClock(false);
    setReset(false);
    return;
  }

  const auto model = m_engine.GetOutputModel(m_lastOutTime);
  m_lastOutTime = model.now;

  switch (playState) {
    case Engine::PlayState::Cued:
      // start playing on first clock of loop
      if (!model.resetTriggered) {
        break;
      }
      // Deliberate fallthrough here
      m_engine.SetPlayState(Engine::PlayState::Playing);
    case Engine::PlayState::Playing:
      triggerOutputs(model.clockTriggered, model.resetTriggered);
      break;
    default:
      break;
  }
}

void OutputProcess::triggerOutputs(bool clockTriggered, bool resetTriggered) {
  if (resetTriggered) { setReset(true); }
  if (clockTriggered) { setClock(true); }
  if (clockTriggered || resetTriggered) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    setReset(false);
    setClock(false);
  }
}

void OutputProcess::setClock(bool high) {
  if (m_clockHigh == high) { return; }
  m_clockHigh = high;
  m_pClockOut->Write(high ? HIGH : LOW);
}

void OutputProcess::setReset(bool high) {
  if (m_resetHigh == high) { return; }
  m_resetHigh = high;
  m_pResetOut->Write(high ? HIGH : LOW);
}

namespace MissingLink {

  static const int NUM_ANIM_FRAMES = 4;

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

}

ViewUpdateProcess::ViewUpdateProcess(Engine &engine, std::shared_ptr<MainView> pView)
  : Engine::Process(engine, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(15)))
  , m_pView(pView)
{}

void ViewUpdateProcess::process() {
  const auto phase = m_engine.GetNormalizedPhase();
  const auto playState = m_engine.GetPlayState();
  animatePhase(phase, playState);
  m_pView->UpdateDisplay();
}

void ViewUpdateProcess::animatePhase(float normalizedPhase, Engine::PlayState playState) {

  const int animFrameIndex = min(
    NUM_ANIM_FRAMES - 1,
    max(0, (int)floor(normalizedPhase * NUM_ANIM_FRAMES))
  );

  switch (playState) {
    case Engine::PlayState::Cued:
      m_pView->SetAnimationLEDs(CueAnimationFrames[animFrameIndex]);
      break;
    case Engine::PlayState::Playing:
      m_pView->SetAnimationLEDs(PlayAnimationFrames[animFrameIndex]);
      break;
    default:
      m_pView->ClearAnimationLEDs();
      break;
  }
}
