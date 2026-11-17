#include <stdint.h>
#include "hal/uart.hpp"

namespace hal {
  class MockUart : public IUart {
    public :
      static constexpr size_t CAPTURE_MAX_BYTES = 2048;

      bool tryWrite(const uint8_t* data, const size_t len) override {
        if (len == 0) {
          return true;
        }
        if (data == nullptr) {
          rejected_count++;
          return false;
        }
        if (len > free_bytes || len > CAPTURE_MAX_BYTES - captured_size_) {
          rejected_count++;
          return false;
        }
        memcpy(&captured_[captured_size_], data, len);
        captured_size_ += len;
        free_bytes_ -= len;
        return true;
      }

    // --- Test controls ---

    void setFreeBytes(size_t free_bytes) { free_bytes_ = free_bytes; }

    void clearCapture() {
        captured_size_ = 0;
        rejected_count_ = 0;
    }

    // --- Test inspection ---

    const uint8_t* captured() const { return captured_; }
    size_t capturedSize() const { return captured_size_; }
    size_t freeBytes() const { return free_bytes_; }
    unsigned rejectedCount() const { return rejected_count_; }

    private :
      // real
      uint8_t captured_[CAPTURE_MAX_BYTES] = {};
      size_t captured_size_ = 0;
      // mock
      size_t free_bytes_ = CAPTURE_MAX_BYTES;
      unsigned rejected_count_ = 0;
  }
}
