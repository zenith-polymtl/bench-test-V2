#pragma once
// HAL interface for digital pins (GPIO). Everything outside communication/ talks to pins
// through this interface, never through pinMode / digitalRead / digitalWrite (AGENTS.md).
//
// Design choices (AGENTS.md: "Document implementation choices")
//
// 1. Generic per-pin interface, no named signals ("contactor", "e-stop").
//    Why: the HAL only knows pins and voltage levels. What a pin means and which level is
//    "active" (wiring dependent, see MAT-01 open point) belongs to the layer above.
//    Rejected: a Contactor / EStop interface in the HAL (mixes hardware access with logic).
//
// 2. Two configure functions instead of one configure(pin, mode, level).
//    Why: configureOutput() REQUIRES the initial level, so it is impossible to create an
//    output without choosing its safe state (PRE-02: outputs rest open).
//    Rejected: a single configure() with an optional level (easy to forget it).
//
// 3. Failures are returned as `false`, never ignored (SUR-04: invalid data counts as out
//    of bounds). A pin that is out of range, not configured, or used in the wrong
//    direction (reading an output, writing an input) is refused.
//    Rejected: reading an unconfigured pin as LOW (silent wrong data on a safety input).
//
// 4. No exceptions, no dynamic allocation: usable from the safety loop (AGENTS.md).

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
