#pragma once
// Mock IClock for native tests: time only moves when the test advances it, so a test can run
// seconds of firmware time instantly and deterministically. Not built into the Teensy firmware.
#include <stdint.h>

#include "hal/clock.hpp"

namespace hal {

class MockClock : public IClock {
public:
    uint32_t nowMs() const override { return now_us_ / 1000; }
    uint32_t nowUs() const override { return now_us_; }

    // --- Test helpers (not part of IClock) ---

    void advanceMs(uint32_t ms) { now_us_ += ms * 1000; }
    void advanceUs(uint32_t us) { now_us_ += us; }

private:
    // Wraps at 2^32 us (about 71 minutes), like micros() on the real board, so tests can also
    // check the unsigned-subtraction rule for time comparisons (AGENTS.md).
    uint32_t now_us_ = 0;
};

}  // namespace hal
