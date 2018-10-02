/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <cstring>
#include "missing_link/gpio.hpp"
#include "missing_link/display.hpp"

using namespace MissingLink;

const uint16_t SegmentDisplay::ASCIILookup[] =  {
  0b0000000000000001,
  0b0000000000000010,
  0b0000000000000100,
  0b0000000000001000,
  0b0000000000010000,
  0b0000000000100000,
  0b0000000001000000,
  0b0000000010000000,
  0b0000000100000000,
  0b0000001000000000,
  0b0000010000000000,
  0b0000100000000000,
  0b0001000000000000,
  0b0010000000000000,
  0b0100000000000000,
  0b1000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0001001011001001,
  0b0001010111000000,
  0b0001001011111001,
  0b0000000011100011,
  0b0000010100110000,
  0b0001001011001000,
  0b0011101000000000,
  0b0001011100000000,
  0b0000000000000000, //
  0b0000000000000110, // !
  0b0000001000100000, // "
  0b0001001011001110, // #
  0b0001001011101101, // $
  0b0000110000100100, // %
  0b0010001101011101, // &
  0b0000010000000000, // '
  0b0010010000000000, // (
  0b0000100100000000, // )
  0b0011111111000000, // *
  0b0001001011000000, // +
  0b0000100000000000, // ,
  0b0000000011000000, // -
  0b0000000000000000, // .
  0b0000110000000000, // /
  0b0000000000111111, // 0
  0b0000000000000110, // 1
  0b0000000011011011, // 2
  0b0000000011001111, // 3
  0b0000000011100110, // 4
  0b0000000011101101, // 5
  0b0000000011111101, // 6
  0b0000000000000111, // 7
  0b0000000011111111, // 8
  0b0000000011101111, // 9
  0b0001001000000000, // :
  0b0000101000000000, // ;
  0b0010010000000000, // <
  0b0000000011001000, // =
  0b0000100100000000, // >
  0b0001000010000011, // ?
  0b0000001010111011, // @
  0b0000000011110111, // A
  0b0001001010001111, // B
  0b0000000000111001, // C
  0b0001001000001111, // D
  0b0000000011111001, // E
  0b0000000001110001, // F
  0b0000000010111101, // G
  0b0000000011110110, // H
  0b0001001000000000, // I
  0b0000000000011110, // J
  0b0010010001110000, // K
  0b0000000000111000, // L
  0b0000010100110110, // M
  0b0010000100110110, // N
  0b0000000000111111, // O
  0b0000000011110011, // P
  0b0010000000111111, // Q
  0b0010000011110011, // R
  0b0000000011101101, // S
  0b0001001000000001, // T
  0b0000000000111110, // U
  0b0000110000110000, // V
  0b0010100000110110, // W
  0b0010110100000000, // X
  0b0001010100000000, // Y
  0b0000110000001001, // Z
  0b0000000000111001, // [
  0b0010000100000000, //
  0b0000000000001111, // ]
  0b0000110000000011, // ^
  0b0000000000001000, // _
  0b0000000100000000, // `
  0b0001000001011000, // a
  0b0010000001111000, // b
  0b0000000011011000, // c
  0b0000100010001110, // d
  0b0000100001011000, // e
  0b0000000001110001, // f
  0b0000010010001110, // g
  0b0001000001110000, // h
  0b0001000000000000, // i
  0b0000000000001110, // j
  0b0011011000000000, // k
  0b0000000000110000, // l
  0b0001000011010100, // m
  0b0001000001010000, // n
  0b0000000011011100, // o
  0b0000000101110000, // p
  0b0000010010000110, // q
  0b0000000001010000, // r
  0b0010000010001000, // s
  0b0000000001111000, // t
  0b0000000000011100, // u
  0b0010000000000100, // v
  0b0010100000010100, // w
  0b0010100011000000, // x
  0b0010000000001100, // y
  0b0000100001001000, // z
  0b0000100101001001, // {
  0b0001001000000000, // |
  0b0010010010001001, // }
  0b0000010100100000, // ~
  0b0011111111111111
};

SegmentDisplay::SegmentDisplay(uint8_t i2cBus, uint8_t i2cAddress)
  : m_i2cDevice(std::unique_ptr<GPIO::I2CDevice>(new GPIO::I2CDevice(i2cBus, i2cAddress)))
{}

SegmentDisplay::~SegmentDisplay() {}

void SegmentDisplay::Init() {
  Clear();
  m_i2cDevice->Command(0x21); // oscillator on
  m_i2cDevice->Command(0x81); // display on
}

void SegmentDisplay::WriteRaw(uint8_t index, uint16_t bitmask) {
  writeRaw(index, bitmask);
  commit();
}

void SegmentDisplay::WriteAscii(uint8_t index, uint8_t aChar, bool dot) {
  writeAscii(index, aChar, dot);
  commit();
}

void SegmentDisplay::Write(std::string const &string) {
  write(string);
  commit();
}

void SegmentDisplay::Clear() {
  memset(m_displayBuffer, 0, 8);
  commit();
}

void SegmentDisplay::write(std::string const &string) {
  // get length of string collapsing dot characters
  int rawlen = string.length();
  int len = rawlen;
  for (int i = 0; i < rawlen; i++) {
    if (string[i] == '.') { len--; }
  }

  // determine start position for right-justification
  int start = 0;
  if (len < 4) {
    start = 4 - len;
    for (int i = 0; i < start; i++) {
      writeRaw(i, 0x0000);
    }
  }

  // write formatted string
  char a;
  bool dot;
  int digit = start;
  int offset = 0;
  while (digit < 4 && offset < rawlen) {
    a = string[offset];
    if (offset < rawlen - 1 && string[offset + 1] == '.') {
      dot = true;
      offset++;
    } else {
      dot = false;
    }
    writeAscii(digit, a, dot);
    offset++;
    digit++;
  }
}

void SegmentDisplay::writeRaw(uint8_t index, uint16_t bitmask) {
  m_displayBuffer[index] = bitmask;
}

void SegmentDisplay::writeAscii(uint8_t index, uint8_t a, bool dot) {
  m_displayBuffer[index] = ASCIILookup[a];
  if (dot) { m_displayBuffer[index] |= (1 << 14); }
}

void SegmentDisplay::commit() {
  m_i2cDevice->WriteBlock(0x00, (uint8_t *)m_displayBuffer, 8);
}
