/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <chrono>
#include <memory>
#include <functional>
#include "missing_link/types.hpp"
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class Control {

  public:

    Control(std::vector<int> pinIndices);
    virtual ~Control();

    bool CanHandleInterrupt(uint8_t flag) {
      return (flag & m_flagMask) != 0;
    }

    void HandleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander);

  protected:

    virtual void handleInterrupt(uint8_t flag,
                                 uint8_t state,
                                 std::shared_ptr<IOExpander> pExpander) = 0;
    uint8_t m_flagMask = 0;
};

class Button : public Control {

  public:

    Button(int pinIndex);

    virtual ~Button();

    // Press down detected once.
    // TODO: break out into separate press states if needed.
    std::function<void(void)> onButtonDown;
    std::function<void(void)> onButtonUp;

  private:

    uint8_t m_lastState;
    TimePoint m_lastEvent;
    void handleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander) override;
};

class RotaryEncoder : public Control {

  public:

    RotaryEncoder(int pinIndexA, int pinIndexB);
    virtual ~RotaryEncoder();

    // Float indicates rotation amount (1.0 == one notch up, -1.0 == one notch down)
    // TODO: account for speed/acceleration
    std::function<void(float)> onRotated;

  private:

    uint8_t m_aFlag;
    uint8_t m_bFlag;

    long m_encVal;
    unsigned int m_lastEncSeq;
    TimePoint m_lastChange;

    void handleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander) override;

    void decode(bool aOn, bool bOn);
};

}
