// Tests the IGpio contract through MockGpio. The Teensy implementation cannot run on a
// PC; it follows the same contract and is checked by compiling it for the Teensy.
#include <unity.h>

#include "communication/mock/mock_gpio.hpp"

using namespace hal;

namespace {
constexpr uint8_t PIN_CONTACTOR = 5;
constexpr uint8_t PIN_PRECHARGE = 6;
constexpr uint8_t PIN_ESTOP = 7;
}  // namespace

void setUp() {}
void tearDown() {}

void test_output_starts_at_initial_level() {
    MockGpio gpio;
    TEST_ASSERT_TRUE(gpio.configureOutput(PIN_CONTACTOR, GpioLevel::Low));

    TEST_ASSERT_EQUAL(GpioLevel::Low, gpio.outputLevel(PIN_CONTACTOR));
    // The very first thing the pin ever drove is the initial level (PRE-02).
    TEST_ASSERT_EQUAL_UINT(1, gpio.historySize());
    TEST_ASSERT_EQUAL_UINT8(PIN_CONTACTOR, gpio.historyAt(0).pin);
    TEST_ASSERT_EQUAL(GpioLevel::Low, gpio.historyAt(0).level);
}

void test_write_changes_output_and_keeps_order() {
    MockGpio gpio;
    gpio.configureOutput(PIN_PRECHARGE, GpioLevel::Low);
    gpio.configureOutput(PIN_CONTACTOR, GpioLevel::Low);

    TEST_ASSERT_TRUE(gpio.write(PIN_PRECHARGE, GpioLevel::High));  // PWR-02: precharge first
    TEST_ASSERT_TRUE(gpio.write(PIN_CONTACTOR, GpioLevel::High));
    TEST_ASSERT_TRUE(gpio.write(PIN_PRECHARGE, GpioLevel::Low));   // PWR-05: then cut it

    TEST_ASSERT_EQUAL(GpioLevel::High, gpio.outputLevel(PIN_CONTACTOR));
    TEST_ASSERT_EQUAL(GpioLevel::Low, gpio.outputLevel(PIN_PRECHARGE));

    // History after the two initial events: precharge on, contactor on, precharge off.
    TEST_ASSERT_EQUAL_UINT(5, gpio.historySize());
    TEST_ASSERT_EQUAL_UINT8(PIN_PRECHARGE, gpio.historyAt(2).pin);
    TEST_ASSERT_EQUAL(GpioLevel::High, gpio.historyAt(2).level);
    TEST_ASSERT_EQUAL_UINT8(PIN_CONTACTOR, gpio.historyAt(3).pin);
    TEST_ASSERT_EQUAL_UINT8(PIN_PRECHARGE, gpio.historyAt(4).pin);
    TEST_ASSERT_EQUAL(GpioLevel::Low, gpio.historyAt(4).level);
}

void test_input_reads_injected_level() {
    MockGpio gpio;
    gpio.configureInput(PIN_ESTOP, true);

    GpioLevel level = GpioLevel::Low;
    TEST_ASSERT_TRUE(gpio.read(PIN_ESTOP, level));
    TEST_ASSERT_EQUAL(GpioLevel::High, level);  // pulled up, nothing connected

    gpio.setInputLevel(PIN_ESTOP, GpioLevel::Low);  // wire pulled to ground
    TEST_ASSERT_TRUE(gpio.read(PIN_ESTOP, level));
    TEST_ASSERT_EQUAL(GpioLevel::Low, level);
}

void test_unconfigured_pin_is_refused() {
    MockGpio gpio;
    GpioLevel level = GpioLevel::High;

    TEST_ASSERT_FALSE(gpio.read(PIN_ESTOP, level));
    TEST_ASSERT_EQUAL(GpioLevel::High, level);  // untouched on failure
    TEST_ASSERT_FALSE(gpio.write(PIN_CONTACTOR, GpioLevel::High));
    TEST_ASSERT_EQUAL_UINT(0, gpio.historySize());
}

void test_wrong_direction_is_refused() {
    MockGpio gpio;
    gpio.configureOutput(PIN_CONTACTOR, GpioLevel::Low);
    gpio.configureInput(PIN_ESTOP, true);

    GpioLevel level = GpioLevel::High;
    TEST_ASSERT_FALSE(gpio.read(PIN_CONTACTOR, level));         // reading an output
    TEST_ASSERT_FALSE(gpio.write(PIN_ESTOP, GpioLevel::Low));   // writing an input
    TEST_ASSERT_EQUAL_UINT(1, gpio.historySize());              // only the initial event
}

void test_out_of_range_pin_is_refused() {
    MockGpio gpio;
    GpioLevel level = GpioLevel::Low;

    TEST_ASSERT_FALSE(gpio.configureInput(GPIO_PIN_COUNT, false));
    TEST_ASSERT_FALSE(gpio.configureOutput(255, GpioLevel::Low));
    TEST_ASSERT_FALSE(gpio.read(GPIO_PIN_COUNT, level));
    TEST_ASSERT_FALSE(gpio.write(GPIO_PIN_COUNT, GpioLevel::High));
}

void test_works_through_the_interface() {
    // Code under test only ever sees IGpio, so a mock can replace the Teensy.
    MockGpio mock;
    IGpio& gpio = mock;
    TEST_ASSERT_TRUE(gpio.configureOutput(PIN_CONTACTOR, GpioLevel::Low));
    TEST_ASSERT_TRUE(gpio.write(PIN_CONTACTOR, GpioLevel::High));
    TEST_ASSERT_EQUAL(GpioLevel::High, mock.outputLevel(PIN_CONTACTOR));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_output_starts_at_initial_level);
    RUN_TEST(test_write_changes_output_and_keeps_order);
    RUN_TEST(test_input_reads_injected_level);
    RUN_TEST(test_unconfigured_pin_is_refused);
    RUN_TEST(test_wrong_direction_is_refused);
    RUN_TEST(test_out_of_range_pin_is_refused);
    RUN_TEST(test_works_through_the_interface);
    return UNITY_END();
}
