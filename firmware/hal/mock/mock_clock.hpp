#include <stdint.h>

#include "hal/clock.hpp"

namespace hal {
  class MockClock : public IClock {
    public :
      uint32_t nowMs() const override {
        return now_us / 1000;
      }
      uint32_t nowUs() const override {
        return now_us;
      }
      // Test helper
      void advanceMs(uint32_t ms) {
        now_us += ms * 1000;
      }
    
    private :
      uint32_t now_us = 0;
  }
}
