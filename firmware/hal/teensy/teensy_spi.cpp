#include "hal/teensy/teensy_spi.hpp"

#include <Arduino.h>
#include <SPI.h>

namespace hal {

namespace {
uint8_t toArduinoMode(SpiMode mode) {
    switch (mode) {
        case SpiMode::Mode0: return SPI_MODE0;
        case SpiMode::Mode1: return SPI_MODE1;
        case SpiMode::Mode2: return SPI_MODE2;
        case SpiMode::Mode3: return SPI_MODE3;
    }
    return SPI_MODE0;
}
}  // namespace

void TeensySpi::begin() {
    // CS must idle High (deselected) from the very first moment it is driven, so the latch
    // is preloaded BEFORE the pin becomes an output, like TeensyGpio::configureOutput().
    // digitalWriteFast() is required, not digitalWrite(): in the Teensy 4 core
    // (cores/teensy4/digital.c) digitalWrite() on a pin still in INPUT mode only changes the
    // pull resistor, while digitalWriteFast() (core_pins.h, runtime-pin branch) writes the
    // set / clear register whatever the direction. Read from the core source (1.62.0), not
    // yet measured on hardware.
    // Rejected: pinMode(OUTPUT) then digitalWrite(HIGH) (CS could pulse Low in between).
    digitalWriteFast(cs_pin_, HIGH);
    pinMode(cs_pin_, OUTPUT);
    bus_.begin();
    // A pure read (tx == nullptr) shifts out this fill byte. It is 0 by default; set it
    // explicitly so the ISpi contract ("sends 0x00") doesn't depend on that default.
    bus_.setTransferWriteFill(0x00);
    begun_ = true;
}

bool TeensySpi::transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
    if (!begun_ || len == 0 || len > SPI_MAX_TRANSFER_BYTES) return false;
    if (tx == nullptr && rx == nullptr) return false;

    // beginTransaction() applies THIS device's clock and mode, so devices with different
    // settings can share the bus. Rejected: configuring the bus once globally (the ICM-42688
    // in mode 3 and the nRF24 in mode 0 would corrupt each other).
    bus_.beginTransaction(SPISettings(config_.clock_hz, MSBFIRST, toArduinoMode(config_.mode)));
    digitalWrite(cs_pin_, LOW);
    // Teensy 4 SPIClass::transfer(buf, retbuf, count) (libraries/SPI/SPI.cpp, LPSPI version)
    // accepts a null buf (sends the fill byte, see begin()) and a null retbuf (discards the
    // received bytes), which is exactly the ISpi contract. Read from the library source
    // (1.62.0), not yet measured on hardware.
    bus_.transfer(tx, rx, len);
    digitalWrite(cs_pin_, HIGH);  // always raised, no early return between LOW and here
    bus_.endTransaction();
    return true;
}

}  // namespace hal
