/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <sstream>
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"

using namespace std;
using namespace MissingLink;

Engine::State::State()
  : running(true)
  , playState(Stopped)
  , settings(Settings::Load())
  , link(settings.load().tempo)
{
  link.enable(true);
}

Engine::Process::Process(State &state)
  : m_state(state)
  , m_bStopped(true)
{}

Engine::Process::~Process() {
  Stop();
}

void Engine::Process::Run() {
  if (m_pThread != nullptr) { return; }
  m_bStopped = false;
  m_pThread = unique_ptr<thread>(new thread(&Process::run, this));
}

void Engine::Process::Stop() {
  if (m_pThread == nullptr) { return; }
  m_bStopped = true;
  m_pThread->join();
  m_pThread = nullptr;
}

void Engine::Process::run() {
  while (!m_bStopped) {
    process();
  }
}

Engine::Engine()
  : m_pUI(shared_ptr<UserInterface>(new UserInterface()))
  , m_pView(shared_ptr<MainView>(new MainView()))
  , m_pTapTempo(unique_ptr<TapTempo>(new TapTempo()))
  , m_inputMode(BPM)
{
  m_pUI->onPlayStop = bind(&Engine::playStop, this);
  m_pUI->onTapTempo = bind(&TapTempo::Tap, m_pTapTempo.get());
  m_pUI->onEncoderRotate = bind(&Engine::routeEncoderAdjust, this, placeholders::_1);
  m_pUI->onEncoderPress = bind(&Engine::toggleMode, this);
  m_pView->SetInputModeLED(m_inputMode);
  m_pTapTempo->onNewTempo = bind(&Engine::setTempo, this, placeholders::_1);
}

void Engine::Run() {
  OutputLoop output(m_state, m_pView);

  output.Run();
  m_pUI->StartPollingInput();

  std::thread displayThread(&Engine::runDisplayLoop, this);

  while (m_state.running) {
    Settings settings = m_state.settings.load();
    Settings::Save(settings);
    this_thread::sleep_for(chrono::seconds(1));
  }

  output.Stop();
  m_pUI->StopPollingInput();
  displayThread.join();
}

void Engine::runDisplayLoop() {
  while (m_state.running) {
    auto value = formatDisplayValue();
    m_pView->WriteDisplay(value);
    this_thread::sleep_for(chrono::milliseconds(50));
  }
}

std::string Engine::formatDisplayValue() {

  ostringstream stringStream;
  stringStream.setf(ios::fixed, ios::floatfield);
  stringStream.precision(1);

  switch (m_inputMode) {
    case BPM: {
      auto timeline = m_state.link.captureAppTimeline();
      stringStream << timeline.tempo();
      break;
    }
    case Loop:
      stringStream << (int)m_state.settings.load().quantum;
      break;
    case Clock:
      stringStream << (int)m_state.settings.load().ppqn;
      break;
    default:
      break;
  }

  return stringStream.str();
}

void Engine::playStop() {
  switch (m_state.playState) {
    case Stopped:
      // reset the timeline to zero if there are no peers
      if (m_state.link.numPeers() == 0) {
        resetTimeline();
      }
      m_state.playState = Cued;
      break;
    case Playing:
    case Cued:
      m_state.playState = Stopped;
      break;
    default:
      break;
  }
}

void Engine::toggleMode() {
  m_inputMode = (InputMode)((m_inputMode + 1) % (int)NUM_INPUT_MODES);
  m_pView->SetInputModeLED(m_inputMode);
}

void Engine::routeEncoderAdjust(float amount) {
  switch (m_inputMode) {
    case BPM:
      tempoAdjust(amount);
      break;
    case Loop:
      loopAdjust((int)amount);
      break;
    case Clock:
      ppqnAdjust((int)amount);
      break;
    default:
      break;
  }
}

void Engine::resetTimeline() {
  // Reset to beat zero in 1 ms
  auto timeline = m_state.link.captureAppTimeline();
  auto resetTime = m_state.link.clock().micros() + std::chrono::milliseconds(1);
  timeline.forceBeatAtTime(0, resetTime, m_state.settings.load().quantum);
  m_state.link.commitAppTimeline(timeline);
}

void Engine::tempoAdjust(float amount) {
  auto timeline = m_state.link.captureAppTimeline();
  double tempo = timeline.tempo() + amount;
  setTempo(tempo);
}

void Engine::loopAdjust(int amount) {
  auto settings = m_state.settings.load();
  settings.quantum = std::max(1, settings.quantum + amount);
  m_state.settings = settings;
}

void Engine::ppqnAdjust(int amount) {
  auto settings = m_state.settings.load();
  settings.ppqn = std::min(24, std::max(1, settings.ppqn + amount));
  m_state.settings = settings;
}

void Engine::setTempo(double tempo) {
  auto now = m_state.link.clock().micros();
  auto timeline = m_state.link.captureAppTimeline();
  timeline.setTempo(tempo, now);
  m_state.link.commitAppTimeline(timeline);

  auto settings = m_state.settings.load();
  settings.tempo = tempo;
  m_state.settings = settings;

  // switch back to tempo mode
  if (m_inputMode != BPM) {
    m_inputMode = BPM;
    m_pView->SetInputModeLED(BPM);
  }
}
