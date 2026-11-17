#pragma once
// Mock ISpi for native tests: no hardware. Records every transaction and lets a test script
// what the "chip" answers. Not built into the Teensy firmware.
// It does not model timing or the chip-select wire; it only proves that one transfer() is
// exactly one frame with these bytes.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hal/spi.hpp"

namespace hal {

class MockSpi : public ISpi {
public:
    struct Transaction {
        uint8_t tx[SPI_MAX_TRANSFER_BYTES];  // what was sent (zeros if tx was null)
        size_t len;
    };
    // Called for each transfer when set: fills rx[0..len) from tx[0..len). Lets a driver test
    // plug in a small fake chip (register map). Function pointer, not std::function: no heap.
    using Responder = void (*)(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len);

    static constexpr size_t HISTORY_MAX = 32;
    static constexpr size_t QUEUE_MAX = 8;

    bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) override {
        if (!begun_ || len == 0 || len > SPI_MAX_TRANSFER_BYTES) return false;
        if (tx == nullptr && rx == nullptr) return false;
        if (fail_count_ > 0) {  // injected bus fault: nothing happened on the "wire"
            fail_count_--;
            return false;
        }

        uint8_t sent[SPI_MAX_TRANSFER_BYTES] = {};
        if (tx != nullptr) memcpy(sent, tx, len);
        record(sent, len);

        // Response priority: scripted queue first, then the responder, else zeros
        // (a MISO line pulled low).
        uint8_t answer[SPI_MAX_TRANSFER_BYTES] = {};
        if (queue_size_ > 0) {
            const Response& r = queue_[queue_head_];
            memcpy(answer, r.bytes, r.len < len ? r.len : len);
            queue_head_ = (queue_head_ + 1) % QUEUE_MAX;
            queue_size_--;
        } else if (responder_ != nullptr) {
            responder_(responder_ctx_, sent, answer, len);
        }
        if (rx != nullptr) memcpy(rx, answer, len);
        return true;
    }

    // --- Test controls (not part of ISpi) ---

    // Next transfer() returns `len` bytes from `bytes` (zero-padded if shorter). FIFO, one
    // entry per transfer. Returns false if the queue is full or the input is invalid.
    bool queueResponse(const uint8_t* bytes, size_t len) {
        if (bytes == nullptr || len == 0 || len > SPI_MAX_TRANSFER_BYTES) return false;
        if (queue_size_ >= QUEUE_MAX) return false;
        Response& r = queue_[(queue_head_ + queue_size_) % QUEUE_MAX];
        memset(r.bytes, 0, sizeof(r.bytes));
        memcpy(r.bytes, bytes, len);
        r.len = len;
        queue_size_++;
        return true;
    }

    void setResponder(Responder fn, void* ctx) {
        responder_ = fn;
        responder_ctx_ = ctx;
    }

    // The next `count` transfers return false (bus fault), to test the SUR-04 path.
    void failNextTransfers(unsigned count) { fail_count_ = count; }

    // false = device not begun: every transfer is refused.
    void setBegun(bool begun) { begun_ = begun; }

    void clearHistory() {
        history_size_ = 0;
        overflowed_ = false;
    }

    // --- Test inspection ---

    size_t transactionCount() const { return history_size_; }
    const Transaction& transactionAt(size_t index) const { return history_[index]; }
    bool historyOverflowed() const { return overflowed_; }

private:
    struct Response {
        uint8_t bytes[SPI_MAX_TRANSFER_BYTES];
        size_t len;
    };

    void record(const uint8_t* sent, size_t len) {
        if (history_size_ < HISTORY_MAX) {
            Transaction& t = history_[history_size_++];
            memcpy(t.tx, sent, len);
            t.len = len;
        } else {
            overflowed_ = true;
        }
    }

    bool begun_ = true;  // a mock is ready by default; setBegun(false) simulates the failure
    Transaction history_[HISTORY_MAX] = {};
    size_t history_size_ = 0;
    bool overflowed_ = false;
    Response queue_[QUEUE_MAX] = {};
    size_t queue_head_ = 0;
    size_t queue_size_ = 0;
    Responder responder_ = nullptr;
    void* responder_ctx_ = nullptr;
    unsigned fail_count_ = 0;
};

}  // namespace hal
