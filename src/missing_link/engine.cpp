/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"
#include "missing_link/user_interface.hpp"

#define MIN_TEMPO 40.0
#define MAX_TEMPO 200.0

using namespace std;
using namespace MissingLink;

Engine::Process::Process(Engine &engine, std::chrono::microseconds sleepTime)
  : m_engine(engine)
  , m_sleepTime(sleepTime)
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
    sleep();
  }
}

void Engine::Process::sleep() {
  std::this_thread::sleep_for(m_sleepTime);
}

Engine::Engine()
  : m_running(true)
  , m_playState(PlayState::Stopped)
  , m_settings(Settings::Load())
  , m_inputMode(InputMode::BPM)
  , m_link(m_settings.load().tempo)
  , m_pView(shared_ptr<MainView>(new MainView()))
  , m_pTapTempo(unique_ptr<TapTempo>(new TapTempo()))
{
  m_link.enable(true);

  auto outputProcess = unique_ptr<OutputProcess>(new OutputProcess(*this));
  m_processes.push_back(std::move(outputProcess));

  auto viewProcess = unique_ptr<ViewUpdateProcess>(new ViewUpdateProcess(*this, m_pView));
  m_processes.push_back(std::move(viewProcess));

  auto uiProcess = unique_ptr<UserInputProcess>(new UserInputProcess(*this));
  uiProcess->onPlayStop = bind(&Engine::playStop, this);
  uiProcess->onTapTempo = bind(&TapTempo::Tap, m_pTapTempo.get());
  uiProcess->onEncoderRotate = bind(&Engine::routeEncoderAdjust, this, placeholders::_1);
  uiProcess->onEncoderPress = bind(&Engine::toggleMode, this);
  m_processes.push_back(std::move(uiProcess));

  m_pTapTempo->onNewTempo = bind(&Engine::setTempo, this, placeholders::_1);

  m_link.setTempoCallback([this](const double tempo) {
    if (m_inputMode == InputMode::BPM) {
      displayTempo(tempo, false);
    }
  });
}

void Engine::Run() {

  displayTempo(getCurrentTempo(), true);

  for (auto &process : m_processes) {
    process->Run();
  }

  while (isRunning()) {
    Settings settings = m_settings.load();
    Settings::Save(settings);
    this_thread::sleep_for(chrono::seconds(1));
  }

  for (auto &process : m_processes) {
    process->Stop();
  }
}

const double Engine::GetNormalizedPhase() const {
  const auto now = m_link.clock().micros();
  const auto currentSettings = m_settings.load();
  const auto timeline = m_link.captureAppTimeline();
  const double phase = timeline.phaseAtTime(now, currentSettings.quantum);
  return min(1.0, max(0.0, phase / (double)currentSettings.quantum));
}

const Engine::OutputModel Engine::GetOutputModel(std::chrono::microseconds last) const {
  OutputModel output;

  const auto now = m_link.clock().micros();
  output.now = now;

  auto timeline = m_link.captureAudioTimeline();
  output.tempo = timeline.tempo();

  if (last == std::chrono::microseconds(0)) {
    output.clockTriggered = false;
    output.resetTriggered = false;
    return output;
  }

  const auto currentSettings = m_settings.load();
  const double beats = timeline.beatAtTime(now, currentSettings.quantum);
  const double lastBeats = timeline.beatAtTime(last, currentSettings.quantum);

  const int edgesPerBeat = currentSettings.getPPQN() * 2;
  const int edgesPerLoop = edgesPerBeat * currentSettings.quantum;

  const int edge = (int)floor(beats * (double)edgesPerBeat);
  const int lastEdge = (int)floor(lastBeats * (double)edgesPerBeat);

  output.clockTriggered = (edge % 2 == 0) && (edge != lastEdge);
  output.resetTriggered = output.clockTriggered && (edge % edgesPerLoop == 0);

  return output;
}

void Engine::playStop() {
  switch (m_playState) {
    case PlayState::Stopped:
      // reset the timeline to zero if there are no peers
      if (m_link.numPeers() == 0) {
        resetTimeline();
      }
      m_playState = PlayState::Cued;
      break;
    case PlayState::Playing:
    case PlayState::Cued:
      m_playState = PlayState::Stopped;
      break;
    default:
      break;
  }
}

void Engine::toggleMode() {
  auto now = Clock::now();
  // Only switch to next mode if toggle pressed twice within 1 second
  if (now - m_lastToggle < std::chrono::seconds(1)) {
    m_inputMode = static_cast<InputMode>((static_cast<int>(m_inputMode.load()) + 1) % 3);
  }
  m_lastToggle = Clock::now();
  displayCurrentMode();
}

void Engine::resetTimeline() {
  // Reset to beat zero in 1 ms
  auto timeline = m_link.captureAppTimeline();
  auto resetTime = m_link.clock().micros() + std::chrono::milliseconds(1);
  timeline.forceBeatAtTime(0, resetTime, m_settings.load().quantum);
  m_link.commitAppTimeline(timeline);
}

void Engine::setTempo(double tempo) {

  tempo = std::max(MIN_TEMPO, std::min(MAX_TEMPO, tempo));

  auto now = m_link.clock().micros();
  auto timeline = m_link.captureAppTimeline();
  timeline.setTempo(tempo, now);
  m_link.commitAppTimeline(timeline);

  auto settings = m_settings.load();
  settings.tempo = tempo;
  m_settings = settings;

  // switch back to tempo mode
  m_inputMode = InputMode::BPM;
  displayTempo(tempo, true);
}

void Engine::routeEncoderAdjust(float amount) {
  const float rounded = round(amount);
  switch (m_inputMode) {
    case InputMode::BPM:
      tempoAdjust(round(amount));
      break;
    case InputMode::Loop:
      loopAdjust(amount > 0.0 ? 1 : -1);
      break;
    case InputMode::Clock:
      ppqnAdjust(amount > 0.0 ? 1 : -1);
      break;
    default:
      break;
  }
}

void Engine::tempoAdjust(float amount) {
  setTempo(getCurrentTempo() + amount);
}

void Engine::loopAdjust(int amount) {
  auto settings = m_settings.load();
  int quantum = std::max(1, settings.quantum + amount);
  settings.quantum = quantum;
  m_settings = settings;
  displayQuantum(quantum, true);
}

void Engine::ppqnAdjust(int amount) {
  int max_index = Settings::ppqn_options.size() - 1;
  auto settings = m_settings.load();
  int index = std::min(max_index, std::max(0, settings.ppqn_index + amount));
  settings.ppqn_index = index;
  m_settings = settings;
  int ppqn = Settings::ppqn_options[index];
  displayPPQN(ppqn, true);
}

void Engine::displayCurrentMode() {
  const int holdTime = 1000;
  switch (m_inputMode) {
    case InputMode::BPM: {
      m_pView->WriteDisplayTemporarily("BPM", holdTime);
      displayTempo(getCurrentTempo(), false);
      break;
    }
    case InputMode::Loop: {
      m_pView->WriteDisplayTemporarily("LOOP", holdTime);
      displayQuantum(getCurrentQuantum(), false);
      break;
    }
    case InputMode::Clock: {
      m_pView->WriteDisplayTemporarily("CLK", holdTime);
      displayPPQN(getCurrentPPQN(), false);
      break;
    }
    default:
      break;
  }
}

void Engine::displayTempo(double tempo, bool force) {
  std::ostringstream stringStream;
  stringStream.setf(std::ios::fixed, std::ios::floatfield);
  stringStream.precision(1);
  stringStream << tempo;
  m_pView->WriteDisplay(stringStream.str(), force);
}

void Engine::displayQuantum(int quantum, bool force) {
  m_pView->WriteDisplay(std::to_string(quantum), force);
}

void Engine::displayPPQN(int ppqn, bool force) {
  m_pView->WriteDisplay(std::to_string(ppqn), force);
}

double Engine::getCurrentTempo() const {
  auto timeline = m_link.captureAppTimeline();
  return timeline.tempo();
}

int Engine::getCurrentQuantum() const {
  auto settings = m_settings.load();
  return settings.quantum;
}

int Engine::getCurrentPPQN() const {
  auto settings = m_settings.load();
  return Settings::ppqn_options[settings.ppqn_index];
}
