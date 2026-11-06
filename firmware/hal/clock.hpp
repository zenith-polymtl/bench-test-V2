#pragma once
#include <stdint.h>

namespace hal {
  class IClock {
    public:
      virtual ~IClock() = default;
      virtual uint32_t nowMs() const = 0;
      virtual uint32_t nowUs();
  }
}
