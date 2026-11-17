#pragma once
// Teensy 4.1 implementation of IClock. The only place that calls millis() / micros().
#include <Arduino.h>

#include "hal/clock.hpp"

namespace hal {

class TeensyClock : public IClock {
public:
    uint32_t nowMs() const override { return millis(); }
    uint32_t nowUs() const override { return micros(); }
};

}  // namespace hal
