// Placeholder entry point of the sensor Teensy.
//
// The legacy main (Serial JSON loop on top of the old firmware/lib/ drivers) was removed with
// those drivers. The real entry point will live in modules/sensor_teensy/ and wire the HAL,
// the drivers and the safety logic together; this file is then replaced.
//
// Until then the board does nothing on purpose: no pin is configured, so no output is driven
// (fail-safe: motor off, contactor open, see ST-01 / PRE-02). Nothing here blocks or
// touches hardware.
#include <Arduino.h>

void setup() {}

void loop() {}
