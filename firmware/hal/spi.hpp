#pragma once
#include <stddef.h>
#include <stdint.h>

namespace hal {

// Clock polarity / phase. Named Mode0..Mode3 (not SPI_MODE0..3: Arduino defines those macros).
enum class SpiMode : uint8_t { Mode0 = 0, Mode1, Mode2, Mode3 };

// Settings of ONE device on the bus. Bits are always sent MSB first (all our chips do).
// No default values here: the max clock and the mode come from each chip's datasheet and
// live as named constants in its driver, with the requirement ID.
struct SpiConfig {
    uint32_t clock_hz;
    SpiMode mode;
};

// Upper bound of one transaction. SPI is synchronous (it blocks while the bytes shift out),
// but it never waits on an external event: 32 bytes at 10 MHz is about 26 us. The cap keeps
// that worst case explicit (AGENTS.md: nothing blocks the critical path).
constexpr size_t SPI_MAX_TRANSFER_BYTES = 32;

// One ISpi = ONE DEVICE on a bus (bus + chip-select + settings), not the bus itself.
// The chips have different modes and clocks, and binding them to the object means a driver
// cannot use the wrong settings or leave CS low (which would corrupt the next device's frame).
// Never call from an ISR (AGENTS.md).
class ISpi {
public:
    virtual ~ISpi() = default;

    // ONE chip-select-framed, full-duplex transaction: CS low, `len` bytes, CS high.
    //   tx == nullptr -> sends 0x00 bytes (pure read)
    //   rx == nullptr -> received bytes are discarded (pure write)
    // Returns false, sends nothing and leaves `rx` untouched if: len == 0, len >
    // SPI_MAX_TRANSFER_BYTES, tx and rx are both null, or the device is not ready (begin()
    // not called). The caller must treat false as invalid data, i.e. out of bounds (SUR-04).
    // Register framing (e.g. address | 0x80 to read on the ICM-42688) belongs to the drivers.
    virtual bool transfer(const uint8_t* tx, uint8_t* rx, size_t len) = 0;
};

}  // namespace hal
