#pragma once
// Mock IGpio for native tests: no hardware, lets a test inject input levels and inspect
// everything the code under test did to the outputs. Not built into the Teensy firmware.
#include <stddef.h>
#include <stdint.h>

#include "communication/gpio.hpp"

namespace hal {

class MockGpio : public IGpio {
public:
    // One output level, in order. configureOutput()'s initial level counts as the first
    // event of that pin, so a test can prove "the contactor was open from the start".
    struct OutputEvent {
        uint8_t pin;
        GpioLevel level;
    };
    static constexpr size_t HISTORY_MAX = 64;

    bool configureInput(uint8_t pin, bool pullup) override {
        if (pin >= GPIO_PIN_COUNT) return false;
        modes_[pin] = Mode::Input;
        // Like the hardware: a pulled-up input with nothing connected reads High.
        // A floating input is undefined on real hardware; the mock reads Low.
        levels_[pin] = pullup ? GpioLevel::High : GpioLevel::Low;
        return true;
    }

    bool configureOutput(uint8_t pin, GpioLevel initial_level) override {
        if (pin >= GPIO_PIN_COUNT) return false;
        modes_[pin] = Mode::Output;
        levels_[pin] = initial_level;
        record(pin, initial_level);
        return true;
    }

    bool read(uint8_t pin, GpioLevel& level) const override {
        if (pin >= GPIO_PIN_COUNT || modes_[pin] != Mode::Input) return false;
        level = levels_[pin];
        return true;
    }

    bool write(uint8_t pin, GpioLevel level) override {
        if (pin >= GPIO_PIN_COUNT || modes_[pin] != Mode::Output) return false;
        levels_[pin] = level;
        record(pin, level);
        return true;
    }

    // --- Test helpers (not part of IGpio) ---

    // Simulates the outside world driving an input pin (e.g. the E-stop being pressed).
    void setInputLevel(uint8_t pin, GpioLevel level) {
        if (pin < GPIO_PIN_COUNT && modes_[pin] == Mode::Input) levels_[pin] = level;
    }

    // Current level of an output pin; Low if the pin is not an output.
    GpioLevel outputLevel(uint8_t pin) const {
        if (pin >= GPIO_PIN_COUNT || modes_[pin] != Mode::Output) return GpioLevel::Low;
        return levels_[pin];
    }

    size_t historySize() const { return history_size_; }
    OutputEvent historyAt(size_t index) const { return history_[index]; }
    bool historyOverflowed() const { return overflowed_; }

private:
    enum class Mode : uint8_t { Unconfigured = 0, Input, Output };

    void record(uint8_t pin, GpioLevel level) {
        if (history_size_ < HISTORY_MAX) {
            history_[history_size_++] = {pin, level};
        } else {
            overflowed_ = true;
        }
    }

    Mode modes_[GPIO_PIN_COUNT] = {};
    GpioLevel levels_[GPIO_PIN_COUNT] = {};
    OutputEvent history_[HISTORY_MAX] = {};
    size_t history_size_ = 0;
    bool overflowed_ = false;
};

}  // namespace hal
