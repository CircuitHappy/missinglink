/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <algorithm>
#include <thread>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ableton/Link.hpp>
#include "missing_link/hw_defs.h"
#include "missing_link/types.hpp"
#include "missing_link/view.hpp"
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"
#include "missing_link/wifi_status.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;
using std::min;
using std::max;

OutputProcess::OutputProcess(Engine &engine)
  : Engine::Process(engine, std::chrono::microseconds(500))
  , m_pClockOut(std::unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(std::unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
  , m_pLogoLight(std::unique_ptr<Pin>(new Pin(ML_LOGO_PIN, Pin::OUT)))
  , m_pMidiOut(std::unique_ptr<MidiOut>(new MidiOut()))
{
  m_pClockOut->Write(LOW);
  m_pResetOut->Write(LOW);
  m_pLogoLight->Write(HIGH);
  m_pMidiOut->StopTransport();
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
    if (m_transportStopped == false) {
      m_pMidiOut->StopTransport();
      m_transportStopped = true;
    }
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
      triggerOutputs(model.clockTriggered, model.resetTriggered, model.midiClockTriggered);
      break;
    case Engine::PlayState::CuedStop:
      // stop playing on first clock of loop
      if (model.resetTriggered) {
        m_engine.SetPlayState(Engine::PlayState::Stopped);
      } else {
        //keep playing the clock
        triggerOutputs(model.clockTriggered, model.resetTriggered, model.midiClockTriggered);
      }
      break;
    default:
      break;
  }
}

void OutputProcess::triggerOutputs(bool clockTriggered, bool resetTriggered, bool midiClockTriggered) {
  auto playState = m_engine.GetPlayState();
  bool resetTrig = true;
  if (m_engine.getResetMode() == 2) {
    resetTrig = false;
  }
  if (resetTriggered) {
    setReset(resetTrig);
    m_pMidiOut->StartTransport();
    m_transportStopped = false;
  }
  if (clockTriggered) { setClock(true); }
  if (midiClockTriggered) { m_pMidiOut->ClockOut(); }

  if (clockTriggered || resetTriggered) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    switch (m_engine.getResetMode()) {
      case 0:
        if (playState == Engine::PlayState::Playing) {
          setReset(false);
        }
        break;
      case 1:
        if (playState == Engine::PlayState::Playing) {
          setReset(true);
        }
        break;
      case 2:
        if (playState == Engine::PlayState::Playing) {
          setReset(true);
        }
        break;
      default:
        setReset(false);
        break;
    }
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

  static const int NUM_ANIM_FRAMES = 6;

  static const float CueAnimationFrames[][6] =  {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {1, 0.1, 0.1, 0.1, 0.1, 1},
    {1, 1, 0.1, 0.1, 0.1, 1},
    {1, 1, 0.1, 0.1, 1, 1},
    {1, 1, 1, 0.1, 1, 1},
    {1, 1, 1, 1, 1, 1}
  };

  static const float PlayAnimationFrames[][6] = {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {1, 1, 0.1, 0.1, 0.1, 0.1},
    {1, 1, 1, 0.1, 0.1, 0.1},
    {1, 1, 1, 1, 0.1, 0.1},
    {1, 1, 1, 1, 1, 0.1},
    {1, 1, 1, 1, 1, 1},
  };

  static const float CuedStopAnimationFrames[][6] = {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {0, 1, 0.1, 0.1, 0.1, 0.1},
    {0, 0, 1, 0.1, 0.1, 0.1},
    {0, 0, 0, 1, 0.1, 0.1},
    {0, 0, 0, 0, 1, 0.1},
    {0, 0, 0, 0, 0, 1},
  };

  //Animation for WIFI LED when AP is ready to connect to
  static std::vector<float> WifiAccessPointReady =
    {
      0, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55,
      0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
      0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55, 0.5, 0.45, 0.4,
      0.35, 0.3, 0.25, 0.2, 0.15, 0.1, 0.05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

  //Animation for WIFI LED when failed to connect to known AP, booting AP mode
  static std::vector<float> WifiTryingToConnect =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
  m_pView->displayWifiStatusFrame(getWifiStatusFrame(m_engine.getWifiStatus()));
  m_pView->ScrollTempMessage();
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
    case Engine::PlayState::CuedStop:
      m_pView->SetAnimationLEDs(CuedStopAnimationFrames[animFrameIndex]);
      break;
    default:
      m_pView->ClearAnimationLEDs();
      break;
  }
}

float ViewUpdateProcess::getWifiStatusFrame(int wifiStatus) {
  std::vector<float> animationFrames;
  static int frameCount = 1;
  static int frameIndex = 0;
  //int wifiStatus = AP_MODE;
  switch (wifiStatus) {
    case AP_MODE :
      animationFrames = WifiAccessPointReady;
    break;
    case TRYING_TO_CONNECT :
      animationFrames = WifiTryingToConnect;
      break;
    case NO_WIFI_FOUND :
      animationFrames = {0.0};
      break;
    case WIFI_CONNECTED :
      animationFrames = {1.0};
      break;
    case REBOOT :
      animationFrames = {0.0};
      break;
    default :
      animationFrames = {0.0};
      break;
  }
  frameCount = animationFrames.size();
  frameIndex++;
  if (frameIndex > (frameCount - 1)) {
    frameIndex = 0;
  }
  return animationFrames.at(frameIndex);
}
