#pragma once
// Binary RF messages of the test bench. Implements docs/messages.md (sections 2 to 4).
// Header-only, depends only on <stddef.h> / <stdint.h>: usable by every module and
// buildable on a PC for native tests.
//
// Design choices (AGENTS.md: "Document implementation choices")
//
// 1. The `type` and `state` fields are stored as raw uint8_t, not as an `enum class`.
//    Why: the wire size is guaranteed, and a byte received out of range is not an
//    invalid enum value (undefined behavior when compared). The receiver validates the
//    byte before converting it (SUR-04: invalid data counts as out of bounds).
//    Rejected: `enum class X : uint8_t` as the field type (size is fine, but an unknown
//    value received from the link becomes an undeclared enum value).
//
// 2. Commands hold a `CmdHeader hdr` member (composition), not a base class.
//    Why: the struct stays standard-layout, so `offsetof` is valid and the static_asserts
//    can check the byte tables of messages.md.
//    Rejected: inheriting from CmdHeader (offsetof is not defined on derived classes).
//
// 3. Little-endian, `packed`, memcpy from / to the nRF24 buffer.
//    Why: native to the Teensy 4.1, no conversion needed, identical format on both ends
//    (same architecture). Checked by the static_assert below.
//    Rejected: field-by-field serialization (more code and more risk of error, with no
//    gain while both boards are Teensys).
//    32-bit fields may be unaligned: read them with memcpy from the RF buffer, never
//    through a reference or pointer to a member.
//
// 4. Everything lives in `namespace protocol`: `SystemState` already exists in NRF24Link.h
//    (old code, to be replaced, see messages.md section 7) and both must coexist during
//    the migration.
//
// About "setpoint" (CMD_POWER, CMD_ARM, TlmStatus::setpoint_applied)
//
//    The setpoint is the requested motor power, as a fraction of full power, in units
//    of 0.1 %: 0 = motor stopped, 1000 = 100.0 % (CMD-03). 400 means 40.0 %.
//    It is an absolute value, not a delta, and it is not a PWM pulse width: the sensor
//    Teensy turns the applied setpoint into the ESC signal (the "pwm" field of the `tlm`
//    JSON message, in microseconds).
//    Path of the value:
//      - the control Teensy sends the operator's setpoint in CMD_POWER, at 20 Hz;
//      - the sensor Teensy rejects it if seq is stale (ACK-02) or if it is > 1000
//        (CMD-03), then limits its rate of change (SAF-04);
//      - the value actually sent to the ESC is reported back as setpoint_applied, so it
//        can differ from the requested one while the ramp limit is still catching up.
//    CMD_ARM carries a setpoint that must be 0 (ARM-03): the motor cannot be armed
//    while a non-zero power is requested.
//    CMD_KILL and CMD_START carry no setpoint.

#include <stddef.h>
#include <stdint.h>

namespace protocol {

static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
              "messages.md requires little-endian structures");

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

constexpr size_t RF_PAYLOAD_MAX_BYTES = 32;  // nRF24L01 limit (TX-03)

// "Protocol and firmware version" (messages.md 4.2). To be confirmed: no versioning
// scheme is defined yet, so the two boards only compare it for equality.
constexpr uint8_t PROTOCOL_VERSION = 1;

constexpr uint16_t SETPOINT_MAX = 1000;  // CMD-03: setpoint range is 0-1000 (0.1 % units)

// Encoding scales (messages.md section 8: to be validated against the bench's real ranges).
constexpr float SETPOINT_LSB_PCT = 0.1f;  // setpoint, setpoint_applied
constexpr float VOLTAGE_LSB_V = 0.01f;    // voltage
constexpr float CURRENT_LSB_A = 0.1f;     // current
constexpr float THRUST_LSB_N = 0.01f;     // thrust
// vib_rms is in mg and uptime_ms in ms: no scale.

// ---------------------------------------------------------------------------
// 2.1 RF message types
// ---------------------------------------------------------------------------

enum class MsgType : uint8_t {
    CMD_POWER = 0x01,   // Control -> Sensor
    CMD_ARM = 0x02,
    CMD_KILL = 0x03,
    CMD_START = 0x04,
    TLM_STATUS = 0x81,  // Sensor -> Control (ack payload)
    HSK_RESP = 0x82,
};

// The top bit gives the direction: 0x0_ = command, 0x8_ = response.
constexpr uint8_t MSG_DIRECTION_BIT = 0x80;
constexpr bool isResponseType(uint8_t type) { return (type & MSG_DIRECTION_BIT) != 0; }
constexpr bool isCommandType(uint8_t type) { return (type & MSG_DIRECTION_BIT) == 0; }

// ---------------------------------------------------------------------------
// 2.2 Sensor Teensy states
// ---------------------------------------------------------------------------

enum class SystemState : uint8_t {
    BOOT = 0,
    IDLE = 1,       // PRE-01, PRE-02
    SELFTEST = 2,   // TST-01 to TST-05
    LINK = 3,       // HSK-03
    PRECHARGE = 4,  // PWR-02 to PWR-04
    POWERED = 5,
    ARMED = 6,
    DEGRADED = 7,   // LNK-03
    SHUTDOWN = 8,   // ARR-04
};
constexpr uint8_t SYSTEM_STATE_COUNT = 9;
constexpr bool isValidSystemState(uint8_t raw) { return raw < SYSTEM_STATE_COUNT; }

// ---------------------------------------------------------------------------
// 2.3 Error codes: high byte = category, low byte = requirement number.
// ---------------------------------------------------------------------------

enum class ErrorCode : uint16_t {
    NONE = 0x0000,

    // 0x01xx: communication
    COM_TIMEOUT = 0x0101,              // COM-01 · controlled stop
    COM_KILL = 0x0102,                 // COM-02 · emergency stop
    COM_ARMING_SWITCH_OFF = 0x0103,    // COM-03 · controlled stop
    COM_INVALID_COMMANDS = 0x0104,     // COM-04 · controlled stop

    // 0x02xx: electrical
    ELE_OVERCURRENT = 0x0201,          // ELE-01 · emergency stop
    ELE_UNDERVOLTAGE = 0x0202,         // ELE-02 · controlled stop
    ELE_OVERVOLTAGE = 0x0203,          // ELE-03 · controlled stop
    ELE_ESC_VOLTAGE_DROP = 0x0204,     // ELE-04 · emergency stop
    ELE_PM_NO_DATA = 0x0205,           // ELE-05 · controlled stop

    // 0x03xx: mechanical
    MEC_LOAD_CELL_OVERLOAD = 0x0301,   // MEC-01 · controlled stop
    MEC_LOAD_CELL_FAULT = 0x0302,      // MEC-02 · controlled stop
    MEC_VIBRATION = 0x0303,            // MEC-03 · emergency stop
    MEC_IMU_FAULT = 0x0304,            // MEC-04 · controlled stop

    // 0x04xx: hardware
    MAT_ESTOP_OR_INTERLOCK = 0x0401,   // MAT-01 · emergency stop
    MAT_RESET = 0x0402,                // MAT-02 · emergency stop (de facto)
    MAT_CYCLE_OVERRUN = 0x0403,        // MAT-03 · controlled stop

    // 0x05xx: startup sequence abort
    ABT_STEP_TIMEOUT = 0x0501,         // ABT-01
    ABT_SELFTEST_FAILED = 0x0502,      // TST-05
    ABT_LINK_UNSTABLE = 0x0503,        // HSK-03
    ABT_PRECHARGE_THRESHOLD = 0x0504,  // PWR-04
    ABT_CONTACTOR_NOT_CLOSED = 0x0505, // PWR-06
    ABT_LAUNCH_REFUSED = 0x0506,       // PRE-04 / PRE-05
};

// ---------------------------------------------------------------------------
// 2.4 Self-test bits (selftest_fail): bit set = check failed.
// ---------------------------------------------------------------------------

namespace selftest_fail {
constexpr uint16_t ADC_ABSENT = 1u << 0;        // TST-01.ADC1-1 / ADC2-1
constexpr uint16_t ADC_INIT_OR_CAL = 1u << 1;   // TST-01.ADC1-2 to 4 / ADC2-2 to 4
constexpr uint16_t ADC_SATURATED = 1u << 2;     // TST-01.ADC1-5 / ADC2-5
constexpr uint16_t IMU_WHO_AM_I = 1u << 3;      // TST-01.IMU-1
constexpr uint16_t IMU_RESET = 1u << 4;         // TST-01.IMU-2
constexpr uint16_t IMU_FROZEN = 1u << 5;        // TST-01.IMU-3
constexpr uint16_t IMU_IMPLAUSIBLE = 1u << 6;   // TST-01.IMU-4
constexpr uint16_t CAN_BUS = 1u << 7;           // TST-01.PM-1
constexpr uint16_t PM_NODE_STATUS = 1u << 8;    // TST-01.PM-2
constexpr uint16_t PM_NO_MEASUREMENT = 1u << 9; // TST-01.PM-3
constexpr uint16_t BATTERY_VOLTAGE = 1u << 10;  // TST-03
constexpr uint16_t WDOG_INACTIVE = 1u << 11;    // TST-04
constexpr uint16_t TARE_FAILED = 1u << 12;      // TST-02
constexpr uint16_t RESERVED_MASK = 0xE000;      // bits 13-15
}  // namespace selftest_fail

// ---------------------------------------------------------------------------
// 3.1 RF commands: common header (6 bytes)
// ---------------------------------------------------------------------------

namespace cmd_flags {
constexpr uint8_t ARMING_SWITCH_ON = 1u << 0;  // a state, sent in EVERY command (3.1)
constexpr uint8_t RESERVED_MASK = 0xFE;        // bits 1-7: must be 0
}  // namespace cmd_flags

struct __attribute__((packed)) CmdHeader {
    uint8_t type;    // MsgType
    uint8_t flags;   // cmd_flags
    uint32_t seq;    // CMD-02
};
static_assert(sizeof(CmdHeader) == 6, "messages.md 3.1");
static_assert(offsetof(CmdHeader, type) == 0 && offsetof(CmdHeader, flags) == 1 &&
                  offsetof(CmdHeader, seq) == 2,
              "messages.md 3.1");

// 3.2 CMD_POWER (0x01): power setpoint, also the 20 Hz heartbeat (TX-01)
struct __attribute__((packed)) CmdPower {
    CmdHeader hdr;
    uint16_t setpoint;  // requested power, 0.1 % units (0-1000), see "About setpoint" above
};
static_assert(sizeof(CmdPower) == 8, "messages.md 2.1 / 3.2");
static_assert(offsetof(CmdPower, setpoint) == 6, "messages.md 3.2");

// 3.3 CMD_ARM (0x02): arming; setpoint must be 0 (ARM-03)
struct __attribute__((packed)) CmdArm {
    CmdHeader hdr;
    uint16_t setpoint;
};
static_assert(sizeof(CmdArm) == 8, "messages.md 2.1 / 3.3");
static_assert(offsetof(CmdArm, setpoint) == 6, "messages.md 3.3");

// 3.4 CMD_KILL (0x03): immediate stop, never filtered (SAF-03, ARR-03)
struct __attribute__((packed)) CmdKill {
    CmdHeader hdr;
};
static_assert(sizeof(CmdKill) == 6, "messages.md 2.1 / 3.4");

// 3.5 CMD_START (0x04): starts the startup sequence; seq = 0 (HSK-02)
struct __attribute__((packed)) CmdStart {
    CmdHeader hdr;
};
static_assert(sizeof(CmdStart) == 6, "messages.md 2.1 / 3.5");

// ---------------------------------------------------------------------------
// 4.1 TLM_STATUS (0x81): status and application-level ack, 20 Hz, in the ack payload
// ---------------------------------------------------------------------------

namespace sensor_valid {
constexpr uint8_t LOAD_CELL = 1u << 0;
constexpr uint8_t IMU = 1u << 1;
constexpr uint8_t POWER_MODULE = 1u << 2;
}  // namespace sensor_valid

struct __attribute__((packed)) TlmStatus {
    uint8_t type;               // MsgType::TLM_STATUS
    uint8_t state;              // SystemState
    uint16_t error_code;        // ErrorCode
    uint32_t last_applied_seq;  // ACK-04
    uint16_t setpoint_applied;  // 0.1 % units, after the ramp limit (SAF-04)
    uint16_t voltage;           // 0.01 V
    int16_t current;            // 0.1 A
    int16_t thrust;             // 0.01 N
    uint16_t vib_rms;           // mg
    uint8_t sensor_valid;       // sensor_valid::*
    uint8_t link_quality;       // % of commands received during the last second
    uint32_t uptime_ms;         // ms since boot (going backwards signals a reset)
};
static_assert(sizeof(TlmStatus) == 24, "messages.md 2.1 / 4.1");
static_assert(offsetof(TlmStatus, state) == 1 && offsetof(TlmStatus, error_code) == 2 &&
                  offsetof(TlmStatus, last_applied_seq) == 4 &&
                  offsetof(TlmStatus, setpoint_applied) == 8 &&
                  offsetof(TlmStatus, voltage) == 10 && offsetof(TlmStatus, current) == 12 &&
                  offsetof(TlmStatus, thrust) == 14 && offsetof(TlmStatus, vib_rms) == 16 &&
                  offsetof(TlmStatus, sensor_valid) == 18 &&
                  offsetof(TlmStatus, link_quality) == 19 &&
                  offsetof(TlmStatus, uptime_ms) == 20,
              "messages.md 4.1");

// ---------------------------------------------------------------------------
// 4.2 HSK_RESP (0x82): reply to CMD_START once the self-test is done
// ---------------------------------------------------------------------------

struct __attribute__((packed)) HskResp {
    uint8_t type;            // MsgType::HSK_RESP
    uint8_t state;           // LINK if the self-test passed, otherwise SHUTDOWN
    uint16_t error_code;     // NONE, ABT_SELFTEST_FAILED or ABT_LAUNCH_REFUSED
    uint16_t selftest_fail;  // selftest_fail::*
    uint8_t cells;           // configured number of battery cells
    uint8_t fw_version;      // PROTOCOL_VERSION
    uint16_t voltage;        // 0.01 V
};
static_assert(sizeof(HskResp) == 10, "messages.md 2.1 / 4.2");
static_assert(offsetof(HskResp, state) == 1 && offsetof(HskResp, error_code) == 2 &&
                  offsetof(HskResp, selftest_fail) == 4 && offsetof(HskResp, cells) == 6 &&
                  offsetof(HskResp, fw_version) == 7 && offsetof(HskResp, voltage) == 8,
              "messages.md 4.2");

// Every message must fit in one nRF24L01 payload.
static_assert(sizeof(CmdPower) <= RF_PAYLOAD_MAX_BYTES, "nRF24 payload > 32 bytes");
static_assert(sizeof(CmdArm) <= RF_PAYLOAD_MAX_BYTES, "nRF24 payload > 32 bytes");
static_assert(sizeof(TlmStatus) <= RF_PAYLOAD_MAX_BYTES, "nRF24 payload > 32 bytes");
static_assert(sizeof(HskResp) <= RF_PAYLOAD_MAX_BYTES, "nRF24 payload > 32 bytes");

}  // namespace protocol
