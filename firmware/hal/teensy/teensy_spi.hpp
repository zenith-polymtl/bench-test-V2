#pragma once
// Teensy 4.1 implementation of ISpi. The only place that touches SPIClass and the
// chip-select pin.
#include "hal/spi.hpp"

class SPIClass;  // Teensy core type (SPI, SPI1, SPI2), forward-declared.

namespace hal {

class TeensySpi : public ISpi {
public:
    // `bus` and `cs_pin` are injected (not hard-wired), so the same class serves any device on
    // any SPI port and the pin choice stays in the module that wires things together, like
    // TeensyUart(HardwareSerial&).
    TeensySpi(SPIClass& bus, uint8_t cs_pin, SpiConfig config)
        : bus_(bus), cs_pin_(cs_pin), config_(config) {}

    // Call once from setup(). Safe to call for several devices sharing a bus. Until then
    // transfer() refuses everything.
    void begin();

    bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) override;

private:
    SPIClass& bus_;
    uint8_t cs_pin_;
    SpiConfig config_;
    bool begun_ = false;
};

}  // namespace hal
