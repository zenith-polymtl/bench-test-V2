#pragma once
#include <stdint.h>

namespace hal {

// Teensy 4.1 has digital pins 0-54. Pins >= GPIO_PIN_COUNT are always refused.
constexpr uint8_t GPIO_PIN_COUNT = 55;

// Named GpioLevel / GpioMode with PascalCase values because Arduino defines the macros
// HIGH, LOW, INPUT and OUTPUT, which would break an enum using those names.
enum class GpioLevel : uint8_t { Low = 0, High = 1 };

class IGpio {
public:
    virtual ~IGpio() = default;

    // Configures `pin` as an input. `pullup` enables the internal pull-up resistor, so a
    // disconnected wire reads High instead of random noise.
    virtual bool configureInput(uint8_t pin, bool pullup) = 0;

    // Configures `pin` as an output that STARTS at `initial_level`: the pin never drives
    // the opposite level, even for a moment.
    virtual bool configureOutput(uint8_t pin, GpioLevel initial_level) = 0;

    // Reads an input pin into `level`. Returns false (and leaves `level` untouched) if the
    // pin is out of range or not configured as an input.
    virtual bool read(uint8_t pin, GpioLevel& level) const = 0;

    // Drives an output pin. Returns false if the pin is out of range or not configured as
    // an output.
    virtual bool write(uint8_t pin, GpioLevel level) = 0;
};

}  // namespace hal
