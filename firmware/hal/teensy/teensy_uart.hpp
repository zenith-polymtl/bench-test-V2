#pragma once
// Teensy 4.1 implementation of IUart. The only place that touches HardwareSerial.
#include "hal/uart.hpp"

class HardwareSerial;  // Teensy core type (Serial1, Serial2, ...), forward-declared.

namespace hal {

class TeensyUart : public IUart {
public:
    // `port` is injected (e.g. Serial1) instead of hard-wired, so the same class serves
    // any hardware UART and the pin choice stays in the module that wires things together.
    explicit TeensyUart(HardwareSerial& port) : port_(port) {}

    // Opens the port. Call once from setup(). Until then tryWrite() refuses everything.
    void begin(uint32_t baud_bps);

    bool tryWrite(const uint8_t* data, size_t len) override;

private:
    HardwareSerial& port_;
    bool begun_ = false;
};

}  // namespace hal
