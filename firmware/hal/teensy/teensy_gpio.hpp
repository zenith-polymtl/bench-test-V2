#pragma once
// Teensy 4.1 implementation of IGpio. The only place that touches Arduino GPIO functions.
#include "communication/gpio.hpp"

namespace hal {

class TeensyGpio : public IGpio {
public:
    bool configureInput(uint8_t pin, bool pullup) override;
    bool configureOutput(uint8_t pin, GpioLevel initial_level) override;
    bool read(uint8_t pin, GpioLevel& level) const override;
    bool write(uint8_t pin, GpioLevel level) override;

private:
    enum class Mode : uint8_t { Unconfigured = 0, Input, Output };

    // Tracks how each pin was configured so wrong-direction use can be refused.
    // Static storage: no allocation after setup() (AGENTS.md).
    Mode modes_[GPIO_PIN_COUNT] = {};
};

}  // namespace hal
