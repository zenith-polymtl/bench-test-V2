#pragma once
// Mock IUart for native tests: captures what the code under test sent and lets a test
// simulate a full TX buffer. Not built into the Teensy firmware.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hal/uart.hpp"

namespace hal {

class MockUart : public IUart {
public:
    static constexpr size_t CAPTURE_MAX_BYTES = 2048;

    bool tryWrite(const uint8_t* data, size_t len) override {
        if (len == 0) return true;  // zero-length write is a successful no-op
        if (data == nullptr) {
            rejected_count_++;
            return false;
        }
        // All-or-nothing, like the real port: refused if the simulated TX buffer is too full
        // or the capture buffer cannot hold the whole message.
        if (len > free_bytes_ || len > CAPTURE_MAX_BYTES - captured_size_) {
            rejected_count_++;
            return false;
        }
        memcpy(&captured_[captured_size_], data, len);
        captured_size_ += len;
        free_bytes_ -= len;
        return true;
    }

    // --- Test controls (not part of IUart) ---

    // Simulates the free space of the TX buffer (e.g. 0 = buffer full, nothing fits).
    void setFreeBytes(size_t free_bytes) { free_bytes_ = free_bytes; }

    void clearCapture() {
        captured_size_ = 0;
        rejected_count_ = 0;
    }

    // --- Test inspection ---

    const uint8_t* captured() const { return captured_; }
    size_t capturedSize() const { return captured_size_; }
    size_t freeBytes() const { return free_bytes_; }
    unsigned rejectedCount() const { return rejected_count_; }

private:
    // What was actually "sent".
    uint8_t captured_[CAPTURE_MAX_BYTES] = {};
    size_t captured_size_ = 0;
    // Simulated state of the port.
    size_t free_bytes_ = CAPTURE_MAX_BYTES;
    unsigned rejected_count_ = 0;
};

}  // namespace hal
