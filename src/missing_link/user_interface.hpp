/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <thread>
#include <functional>
#include <string>
#include <vector>
#include "missing_link/gpio.hpp"
#include "missing_link/control.hpp"
#include "missing_link/io_expander.hpp"
#include "missing_link/led_driver.hpp"

namespace MissingLink {

class UserInterface {

  public:

    enum EncoderMode {
      BPM,
      LOOP,
      CLOCK,
      NUM_MODES
    };

    static float CueAnimation[4][6];
    static float PlayAnimation[4][6];

    UserInterface();
    virtual ~UserInterface();

    void StartPollingInput();
    void StopPollingInput();

    // Outputs
    // These will be called from input polling thread
    std::function<void()> onPlayStop;
    std::function<void()> onTapTempo;
    std::function<void()> onEncoderPress;
    std::function<void(float)> onEncoderRotate;

    void SetModeLED(EncoderMode mode);

    void SetClock(bool on);
    void SetReset(bool on);

    // Index of animation LED starting from 0
    void SetAnimationLEDs(float phase, const float frames[][6]);
    void SetPlayingAnimation(float phase); //this one goes away once I figure out how to point to the array
    void ClearAnimationLEDs();

  private:

    std::shared_ptr<IOExpander> m_pExpander;
    std::shared_ptr<LEDDriver> m_pLEDDriver;
    std::unique_ptr<ExpanderInputLoop> m_pInputLoop;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;
};

} // namespace
