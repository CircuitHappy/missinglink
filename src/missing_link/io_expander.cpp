/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

// Register address in BANK=1 mode.
// Derive address for each port by OR-ing with port value.
enum IOExpander::Register : uint8_t {
  IODIR     = 0x00, // IO Direction
  IPOL      = 0x01, // Input Polarity
  GPINTEN   = 0x02, // Interrupt-on-change enable
  DEFVAL    = 0x03, // Default value
  INTCON    = 0x04, // Interrupt-on-change control
  IOCON     = 0x05, // Configuration
  GPPU      = 0x06, // Pull Up config
  INTF      = 0x07, // Interrupt flag
  INTCAP    = 0x08, // Interrupt port capture
  GPIO      = 0x09, // GPIO port state
  OLAT      = 0x0A, // Output latch
};

// Configuration option mask
enum IOExpander::ConfigOption : uint8_t {
  INT_ACTIVE_HIGH = 0b00000010, // INT polarity (1 = active-high)
  INT_OPEN_DRAIN  = 0b00000100, // INT open drain (1 = open-drain)
  SEQOP_DISABLE   = 0b00100000, // Sequential operation (1 = disable address incrementing)
  INT_MIRROR      = 0b01000000, // INT mirroring (1 = internally connect interrupts)
  BANK_ADDRESSING = 0b10000000  // Bank mode config (1 = separate banks, 0 = interleaved)
};

IOExpander::IOExpander(uint8_t i2cBus, uint8_t i2cAddress)
  : m_i2cDevice(unique_ptr<I2CDevice>(new I2CDevice(i2cBus, i2cAddress)))
{
  // If not in BANK mode (default state), the IOCON register is at 0x0A.
  // Once the option is set, it will be located at the enum lookup value.
  m_i2cDevice->WriteByte(0x0A, BANK_ADDRESSING);
  m_i2cDevice->WriteByte(IOCON, BANK_ADDRESSING);
  m_i2cDevice->WriteByte(0x0A, 0x00); // reset OLAT if device was not in bank address mode
}

IOExpander::~IOExpander() {}

void IOExpander::ConfigureInterrupt(const InterruptConfig &config) {
  uint8_t opts = BANK_ADDRESSING;
  if (config.activeHigh) opts |= INT_ACTIVE_HIGH;
  if (config.openDrain) opts |= INT_OPEN_DRAIN;
  if (config.mirror) opts |= INT_MIRROR;
  m_i2cDevice->WriteByte(IOCON, opts);
}

void IOExpander::ConfigurePort(Port port, const PortConfig &config) {
  m_i2cDevice->WriteByte(IODIR | port, config.direction);
  m_i2cDevice->WriteByte(IPOL | port, config.inputPolarity);
  m_i2cDevice->WriteByte(DEFVAL | port, config.defaultValue);
  m_i2cDevice->WriteByte(GPINTEN | port, config.iocEnabled);
  m_i2cDevice->WriteByte(INTCON | port, config.iocMode);
  m_i2cDevice->WriteByte(GPPU | port, config.pullUpEnabled);
}

uint8_t IOExpander::ReadInterruptFlag(Port port) {
  return m_i2cDevice->ReadByte(INTF | port);
}

uint8_t IOExpander::ReadCapturedInterruptState(Port port) {
  return m_i2cDevice->ReadByte(INTCAP | port);
}

uint8_t IOExpander::ReadPort(Port port) {
  return m_i2cDevice->ReadByte(GPIO | port);
}

bool IOExpander::ReadPin(const PinDefinition &pin) {
  return ReadPin(pin.port, pin.index);
}

bool IOExpander::ReadPin(Port port, int index) {
  return PinIsOn(index, ReadPort(port));
}

void IOExpander::WritePin(const PinDefinition &pin, bool on) {
  WritePin(pin.port, pin.index, on);
}

void IOExpander::WritePin(Port port, int index, bool on) {
  uint8_t reg = OLAT | port;
  uint8_t state = m_i2cDevice->ReadByte(reg);
  uint8_t pin = 1 << index;
  if (on) {
    state &= ~pin;
  } else {
    state |= pin;
  }
  m_i2cDevice->WriteByte(reg, state);
}

void IOExpander::WriteOutputs(Port port, uint8_t outputs) {
  uint8_t reg = OLAT | port;
  m_i2cDevice->WriteByte(reg, outputs);
}



ExpanderInputLoop::ExpanderInputLoop(shared_ptr<IOExpander> pExpander, int interruptPinAddress)
  : m_pExpander(pExpander)
  , m_pInterruptIn(unique_ptr<Pin>(new Pin(interruptPinAddress, Pin::IN)))
  , m_pPollThread(nullptr)
{
  m_pInterruptIn->SetEdgeMode(Pin::FALLING);
}

ExpanderInputLoop::~ExpanderInputLoop() {
  Stop();
}

void ExpanderInputLoop::RegisterHandler(InterruptHandlerPtr handler) {
  m_interruptHandlers.push_back(handler);
}

void ExpanderInputLoop::Start() {
  if (m_pPollThread != nullptr) { return; }
  m_bStopPolling = false;
  m_pPollThread = unique_ptr<thread>(new thread(&ExpanderInputLoop::runLoop, this));
}

void ExpanderInputLoop::Stop() {
  if (m_pPollThread == nullptr) { return; }
  m_bStopPolling = true;
  m_pPollThread->join();
  m_pPollThread = nullptr;
}


void ExpanderInputLoop::runLoop() {

  // Clear initial interrupt event
  m_pInterruptIn->Read();

  pollfd pfd = m_pInterruptIn->GetPollInfo();
  while (!m_bStopPolling) {

    // If interrupt is already low, handle immediately
    if (m_pInterruptIn->Read() == GPIO::LOW) {
      handleInterrupt();
      continue;
    }

    // Poll for 500ms
    int result = ::poll(&pfd, 1, 500);

    // No interrupts? try again
    if (result == 0) { continue; }

    // Clear interrupt event
    m_pInterruptIn->Read();

    // Handle it
    handleInterrupt();
  }
}

void ExpanderInputLoop::handleInterrupt() {
  uint8_t flag = m_pExpander->ReadInterruptFlag(IOExpander::PORTA);
  uint8_t state = m_pExpander->ReadPort(IOExpander::PORTA);

  for (auto handler : m_interruptHandlers) {
    if (!handler->CanHandleInterrupt(flag)) {
      continue;
    }
    handler->HandleInterrupt(flag, state, m_pExpander);
  }

  this_thread::sleep_for(chrono::milliseconds(1));

  //int pinIndex = 0;
  //while ((flag & 1) == 0) {
  //  pinIndex++;
  //  flag = flag >> 1;
  //}

  //// whether triggered pin is on
  //bool isOn = IOExpander::PinIsOn(pinIndex, state);

  //switch (pinIndex) {
  //  case PLAY_BUTTON:
  //    if (isOn) {
  //      cout << "Play" << endl;
  //      handleInputEvent(InputEvent::PLAY_STOP);
  //    }
  //    break;
  //  case TAP_BUTTON:
  //    if (isOn) {
  //      std::cout << "Tap" << std::endl;
  //      handleInputEvent(InputEvent::TAP_TEMPO);
  //    }
  //    break;
  //  case ENC_BUTTON:
  //    if (isOn) {
  //      std::cout << "Encoder Press" << std::endl;
  //      handleInputEvent(InputEvent::ENC_PRESS);
  //    }
  //    break;
  //  case ENC_A:
  //    decodeEncoder(isOn, IOExpander::PinIsOn(ENC_B, state));
  //    break;
  //  case ENC_B:
  //    decodeEncoder(IOExpander::PinIsOn(ENC_A, state), isOn);
  //    break;
  //}

}

ExpanderInputLoop::InterruptHandler::InterruptHandler(vector<int> pinIndices,
                                                      IOExpander::Port port) {
  for (auto index : pinIndices) {
    m_flagMask |= (1 << index);
  }
}

bool ExpanderInputLoop::InterruptHandler::HandleInterrupt(uint8_t flag,
                                                          uint8_t state,
                                                          shared_ptr<IOExpander> pExpander) {
  return handleInterrupt(flag, state, pExpander);
}

//void UserInterface::handleInputEvent(InputEvent event) {
//    if (onInputEvent != nullptr) {
//      onInputEvent(event);
//    }
//}

//void UserInterface::decodeEncoder(bool aOn, bool bOn) {
//  uint8_t aVal = aOn ? 0x01 : 0x00;
//  uint8_t bVal = bOn ? 0x01 : 0x00;

//  unsigned int seq = (aVal ^ bVal) | (bVal << 1);
//  unsigned int delta = (seq - m_lastEncSeq) & 0b11;

//  m_lastEncSeq = seq;

//  switch (delta) {
//    case 1:
//      m_encVal++;
//      break;
//    case 3:
//      m_encVal--;
//      break;
//    default:
//      break;
//  }

//  if (std::abs(m_encVal) >= 4) {
//    if (m_encVal > 0) {
//      std::cout << "Encoder up" << std::endl;
//      handleInputEvent(InputEvent::ENC_UP);
//    } else {
//      std::cout << "Encoder down" << std::endl;
//      handleInputEvent(InputEvent::ENC_DOWN);
//    }
//    m_encVal = 0;
//  }
//}
