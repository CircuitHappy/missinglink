/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <string>
#include <memory>
#include "missing_link/hw_defs.h"

namespace MissingLink {

namespace GPIO { class I2CDevice; }

// Interface for ht16k33 i2c quad 14-seg display backpack
class SegmentDisplay {

  public:

    SegmentDisplay(uint8_t i2cBus = ML_DEFAULT_I2C_BUS, uint8_t i2cAddress = 0x70);
    virtual ~SegmentDisplay();

    void Init();
    void Write(std::string const &string);
    void WriteRaw(uint8_t index, uint16_t bitmask);
    void WriteAscii(uint8_t index, uint8_t aChar, bool dot);
    void Clear();

  private:

    static const uint16_t ASCIILookup[];

    void write(std::string const &string);
    void writeRaw(uint8_t index, uint16_t bitmask);
    void writeAscii(uint8_t index, uint8_t aChar, bool dot);
    void commit();

    uint16_t m_displayBuffer[4] = { 0x0000, 0x0000, 0x0000, 0x0000 };
    std::unique_ptr<GPIO::I2CDevice> m_i2cDevice;
};

}
