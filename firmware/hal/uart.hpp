#pragma once
#include <stddef.h>
#include <stdint.h>

namespace hal {

// Baud rate of the UART link to the Raspberry Pi.
// TO CONFIRM (telemetrie.md section 5): 115 200 or 460 800. One `tlm` line is ~400 bytes
// at 20 Hz = 8 kB/s, i.e. 70 % of 115 200 baud. Kept at 115 200 until the team decides.
constexpr uint32_t PI_UART_BAUD_BPS = 115200;

class IUart {
public:
    virtual ~IUart() = default;

    // Queues the WHOLE buffer for transmission and returns true, or queues nothing and
    // returns false (null data, or not enough free space in the TX buffer). Never blocks:
    // the caller skips the send instead of waiting (telemetrie.md section 5, AGENTS.md
    // safety rules). A zero-length write is a successful no-op.
    virtual bool tryWrite(const uint8_t* data, size_t len) = 0;
};

}  // namespace hal
