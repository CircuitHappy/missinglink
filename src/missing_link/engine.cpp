/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <vector>
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"
#include "missing_link/user_interface.hpp"

using namespace std;
using namespace MissingLink;

Engine::State::State()
  : running(true)
  , playState(Stopped)
  , inputMode(BPM)
  , settings(Settings::Load())
  , link(settings.load().tempo)
{
  link.enable(true);
}

Engine::Process::Process(State &state, std::chrono::microseconds sleepTime)
  : m_state(state)
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
  : m_pView(shared_ptr<MainView>(new MainView()))
  , m_pTapTempo(unique_ptr<TapTempo>(new TapTempo()))
{
  auto outputProcess = unique_ptr<OutputProcess>(new OutputProcess(m_state));
  m_processes.push_back(std::move(outputProcess));

  auto viewProcess = unique_ptr<ViewUpdateProcess>(new ViewUpdateProcess(m_state, m_pView));
  m_processes.push_back(std::move(viewProcess));

  auto uiProcess = unique_ptr<UserInputProcess>(new UserInputProcess(m_state));
  uiProcess->onPlayStop = bind(&Engine::playStop, this);
  uiProcess->onTapTempo = bind(&TapTempo::Tap, m_pTapTempo.get());
  uiProcess->onEncoderRotate = bind(&Engine::routeEncoderAdjust, this, placeholders::_1);
  uiProcess->onEncoderPress = bind(&Engine::toggleMode, this);
  m_processes.push_back(std::move(uiProcess));

  m_pTapTempo->onNewTempo = bind(&Engine::setTempo, this, placeholders::_1);
}

void Engine::Run() {
  for (auto &process : m_processes) {
    process->Run();
  }

  while (m_state.running) {
    Settings settings = m_state.settings.load();
    Settings::Save(settings);
    this_thread::sleep_for(chrono::seconds(1));
  }

  for (auto &process : m_processes) {
    process->Stop();
  }
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
  InputMode inputMode = m_state.inputMode.load();
  inputMode = (InputMode)((inputMode + 1) % (int)NUM_INPUT_MODES);
  m_state.inputMode = inputMode;
  m_pView->ShowInputModeName(inputMode);
}

void Engine::resetTimeline() {
  // Reset to beat zero in 1 ms
  auto timeline = m_state.link.captureAppTimeline();
  auto resetTime = m_state.link.clock().micros() + std::chrono::milliseconds(1);
  timeline.forceBeatAtTime(0, resetTime, m_state.settings.load().quantum);
  m_state.link.commitAppTimeline(timeline);
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
  if (m_state.inputMode != BPM) {
    m_state.inputMode = BPM;
  }
}

void Engine::routeEncoderAdjust(float amount) {
  float rounded = round(amount);
  switch (m_state.inputMode) {
    case BPM:
      tempoAdjust(rounded);
      break;
    case Loop:
      loopAdjust((int)rounded);
      break;
    case Clock:
      ppqnAdjust((int)rounded);
      break;
    default:
      break;
  }
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
  int max_index = Settings::ppqn_options.size() - 1;
  auto settings = m_state.settings.load();
  settings.ppqn_index = std::min(max_index, std::max(0, settings.ppqn_index + amount));
  m_state.settings = settings;
}


OutputModel::OutputModel(ableton::Link &link, const Settings &settings, bool audioThread) {
  auto timeline = audioThread ? link.captureAudioTimeline() : link.captureAppTimeline();
  tempo = timeline.tempo();

  const auto now = link.clock().micros();
  const double beats = timeline.beatAtTime(now, settings.quantum);
  const double phase = timeline.phaseAtTime(now, settings.quantum);
  normalizedPhase = min(1.0, max(0.0, phase / (double)settings.quantum));

  const int edgesPerBeat = settings.getPPQN() * 2;
  const int edgesPerLoop = edgesPerBeat * settings.quantum;
  const int currentEdges = (int)floor(beats * (double)edgesPerBeat);
  isFirstClock = (currentEdges % edgesPerLoop) == 0;
  clockHigh = currentEdges % 2 == 0;

  const double secondsPerPhrase = 60.0 / (tempo / settings.quantum);
  const double resetHighFraction = ML_RESET_PULSE_LENGTH / secondsPerPhrase;
  resetHigh = (phase <= resetHighFraction);
}
