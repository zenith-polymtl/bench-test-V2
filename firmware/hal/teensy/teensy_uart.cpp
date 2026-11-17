#include "hal/teensy/teensy_uart.hpp"

#include <Arduino.h>

namespace hal {

void TeensyUart::begin(uint32_t baud_bps) {
    port_.begin(baud_bps);  // 8N1 by default
    begun_ = true;
}

bool TeensyUart::tryWrite(const uint8_t* data, size_t len) {
    if (len == 0) return true;
    if (data == nullptr || !begun_) return false;

    // HardwareSerial::write() BLOCKS when the TX ring buffer is full (it waits for the
    // shifter to free a slot). Telemetry must never delay monitoring or commands, so the
    // free space is checked first and the whole message is refused if it doesn't fit.
    // All-or-nothing also avoids sending half a JSON line the Pi would have to discard.
    //
    // Rejected: calling write() directly and trusting its return value (it blocks
    // before returning). Rejected: writing as many bytes as fit (truncated line).
    //
    // Read from the Teensy 4 core API (availableForWrite() = free bytes in the TX buffer),
    // not yet measured on hardware. The TX buffer is small by default: need to check that one
    // telemetry line fits, otherwise every send is refused (see addMemoryForWrite()).
    const int free_bytes = port_.availableForWrite();
    if (free_bytes < 0 || static_cast<size_t>(free_bytes) < len) return false;

    port_.write(data, len);
    return true;
}

}  // namespace hal
