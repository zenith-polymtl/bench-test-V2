#include "communication/teensy/teensy_gpio.hpp"

#include <Arduino.h>

namespace hal {

static_assert(CORE_NUM_DIGITAL >= GPIO_PIN_COUNT,
              "GPIO_PIN_COUNT is larger than the number of pins of this board");

bool TeensyGpio::configureInput(uint8_t pin, bool pullup) {
    if (pin >= GPIO_PIN_COUNT) return false;
    pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
    modes_[pin] = Mode::Input;
    return true;
}

bool TeensyGpio::configureOutput(uint8_t pin, GpioLevel initial_level) {
    if (pin >= GPIO_PIN_COUNT) return false;

    // Preload the output latch BEFORE switching the pin to output, so the first level the
    // pin drives is `initial_level` (PRE-02: contactor rests open).
    //
    // digitalWriteFast() is required here, not digitalWrite(). In the Teensy 4 core
    // (cores/teensy4/digital.c), digitalWrite() on a pin that is still an input does not
    // set the output latch: it changes the pull-up / pull-down resistor instead, and the
    // pin would then come up at whatever level the latch held (normally LOW).
    // digitalWriteFast() writes the set / clear register directly, whatever the direction.
    //
    // Read from the core source, not yet measured on hardware: check with a scope or a
    // multimeter on the real board before relying on it for the contactor.
    digitalWriteFast(pin, initial_level == GpioLevel::High ? HIGH : LOW);
    pinMode(pin, OUTPUT);

    modes_[pin] = Mode::Output;
    return true;
}

bool TeensyGpio::read(uint8_t pin, GpioLevel& level) const {
    if (pin >= GPIO_PIN_COUNT || modes_[pin] != Mode::Input) return false;
    level = digitalReadFast(pin) ? GpioLevel::High : GpioLevel::Low;
    return true;
}

bool TeensyGpio::write(uint8_t pin, GpioLevel level) {
    if (pin >= GPIO_PIN_COUNT || modes_[pin] != Mode::Output) return false;
    digitalWriteFast(pin, level == GpioLevel::High ? HIGH : LOW);
    return true;
}

}  // namespace hal
