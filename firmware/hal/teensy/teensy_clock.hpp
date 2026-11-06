#include "hal/clock.hpp"

namespace hal {
  Class TeensyClock : public IClock {
    public :
      uint32_t nowMs() const override {
        return millis();
      }
      uint32_t nowUs() const override {
        return micros();
      }
  }
}
