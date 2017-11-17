/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <string>
#include <vector>
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class Button : public ExpanderInputLoop::InterruptHandler {

  public:

    Button(int pinIndex,
           std::chrono::milliseconds debounceInterval = std::chrono::milliseconds(2));

    virtual ~Button();

  private:

    std::chrono::milliseconds m_debounceInterval;
    std::chrono::steady_clock::time_point m_lastTriggered;

    void handleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander) override;
};

class RotaryEncoder : public ExpanderInputLoop::InterruptHandler {

  public:

    RotaryEncoder(int pinIndexA, int pinIndexB);
    virtual ~RotaryEncoder();

  private:

    uint8_t m_aFlag;
    uint8_t m_bFlag;

    long m_encVal;
    unsigned int m_lastEncSeq;

    std::chrono::steady_clock::time_point m_lastTriggered;

    void handleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander) override;

    void decode(bool aOn, bool bOn);
};

class UserInterface {

  public:

    enum class InputEvent {
      PlayStop,
      TapTempo,
      EncoderDown,
      EncoderUp,
      EncoderPress
    };

    UserInterface();
    virtual ~UserInterface();

    // Inputs

    void StartPollingInput();
    void StopPollingInput();

    // This will be called from input polling thread
    std::function<void(InputEvent)> onInputEvent;

    // Outputs

    void SetBPMModeLED(bool on);
    void SetLoopModeLED(bool on);
    void SetClockModeLED(bool on);

    // Index of animation LED starting from 0
    void SetAnimationLED(int index, bool on);

    void SetClock(bool on);
    void SetReset(bool on);

  private:

    std::shared_ptr<IOExpander> m_pExpander;
    std::unique_ptr<ExpanderInputLoop> m_pInputLoop;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;

};

} // namespace
