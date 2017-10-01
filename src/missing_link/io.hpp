/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <thread>

#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class IO {

  public:

    IO();
    virtual ~IO();

    // Below methods are NOT THREAD SAFE.
    // Must be called from single thread or use external mutex.

    void SetClock(GPIO::DigitalValue value);
    void SetReset(GPIO::DigitalValue value);

  private:

    std::unique_ptr<IOExpander> m_pExpander;
    std::unique_ptr<GPIO::Pin> m_pInterruptIn;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;

};

} // namespace
