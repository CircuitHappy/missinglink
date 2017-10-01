/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include "missing_link/pin_defs.hpp"
#include "missing_link/io.hpp"

#define ML_INTERRUPT_PIN  CHIP_SPI_CS0
#define ML_CLOCK_PIN      CHIP_PE4
#define ML_RESET_PIN      CHIP_PE5

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

IO::IO()
  : m_pExpander(unique_ptr<IOExpander>(new IOExpander()))
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(ML_INTERRUPT_PIN, Pin::IN)))
  , m_pClockOut(unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{}

IO::~IO() {}

void IO::SetClock(DigitalValue value) {
  m_pClockOut->Write(value);
}

void IO::SetReset(DigitalValue value) {
  m_pResetOut->Write(value);
}
