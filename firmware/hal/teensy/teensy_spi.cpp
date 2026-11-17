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
    // CS must idle High (deselected). The order is output first, then High, because on the
    // Teensy 4 core digitalWrite() on a pin still in INPUT mode only toggles its pull-up and
    // does not set the output latch (from memory of the core, not re-read or measured):
    // High-then-OUTPUT could pulse CS Low at boot. Cost: CS may glitch for a few
    // nanoseconds between the two calls, before any clock edge exists, so no chip frame
    // can start. If that glitch matters, add an external pull-up on each CS line.
    // Rejected: relying on the pin's reset state (undefined for this purpose).
    pinMode(cs_pin_, OUTPUT);
    digitalWrite(cs_pin_, HIGH);
    bus_.begin();
    begun_ = true;
}

bool TeensySpi::transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
    if (!begun_ || len == 0 || len > SPI_MAX_TRANSFER_BYTES) return false;
    if (tx == nullptr && rx == nullptr) return false;

    // The Teensy core transfer(buf, retbuf, count) takes one buffer pointer per direction.
    // Pure read / pure write are handled with a local zero buffer / scratch buffer instead of
    // relying on null pointers being accepted: not verified in the core source nor on
    // hardware yet. If the core is confirmed to accept null for both, these can go away.
    uint8_t zeros[SPI_MAX_TRANSFER_BYTES] = {};
    uint8_t scratch[SPI_MAX_TRANSFER_BYTES];
    const uint8_t* out = (tx != nullptr) ? tx : zeros;
    uint8_t* in = (rx != nullptr) ? rx : scratch;

    // beginTransaction() applies THIS device's clock and mode, so devices with different
    // settings can share the bus. Rejected: configuring the bus once globally (the ICM-42688
    // in mode 3 and the nRF24 in mode 0 would corrupt each other).
    bus_.beginTransaction(SPISettings(config_.clock_hz, MSBFIRST, toArduinoMode(config_.mode)));
    digitalWrite(cs_pin_, LOW);
    bus_.transfer(out, in, len);
    digitalWrite(cs_pin_, HIGH);  // always raised, no early return between LOW and here
    bus_.endTransaction();
    return true;
}

}  // namespace hal
