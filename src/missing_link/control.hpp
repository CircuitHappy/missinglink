#pragma once

#include <chrono>
#include <memory>
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class Control : public ExpanderInputLoop::InterruptHandler {

  public:

    Control(std::vector<int> pinIndices);
    virtual ~Control();

  protected:

    std::chrono::steady_clock::time_point m_lastTriggered;
};

class Button : public Control {

  public:

    Button(int pinIndex,
           std::chrono::milliseconds debounceInterval = std::chrono::milliseconds(2));

    virtual ~Button();

  private:

    std::chrono::milliseconds m_debounceInterval;

    void handleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander) override;
};

class RotaryEncoder : public Control {

  public:

    RotaryEncoder(int pinIndexA, int pinIndexB);
    virtual ~RotaryEncoder();

  private:

    uint8_t m_aFlag;
    uint8_t m_bFlag;

    long m_encVal;
    unsigned int m_lastEncSeq;

    void handleInterrupt(uint8_t flag,
                         uint8_t state,
                         std::shared_ptr<IOExpander> pExpander) override;

    void decode(bool aOn, bool bOn);
};

}
