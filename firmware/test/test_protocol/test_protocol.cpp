// Checks that the structs in protocol/messages.hpp produce exactly the bytes described in
// docs/messages.md (little-endian, no padding).
#include <string.h>
#include <unity.h>

#include "protocol/messages.hpp"

using namespace protocol;

void setUp() {}
void tearDown() {}

void test_cmd_power_bytes() {
    CmdPower cmd{};
    cmd.hdr.type = static_cast<uint8_t>(MsgType::CMD_POWER);
    cmd.hdr.flags = cmd_flags::ARMING_SWITCH_ON;
    cmd.hdr.seq = 0x04030201;
    cmd.setpoint = 0x1234;

    const uint8_t expected[8] = {0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x34, 0x12};
    TEST_ASSERT_EQUAL_MEMORY(expected, &cmd, sizeof(expected));
}

void test_tlm_status_bytes() {
    TlmStatus t{};
    t.type = static_cast<uint8_t>(MsgType::TLM_STATUS);
    t.state = static_cast<uint8_t>(SystemState::ARMED);
    t.error_code = static_cast<uint16_t>(ErrorCode::ELE_OVERCURRENT);
    t.last_applied_seq = 0x0A0B0C0D;
    t.setpoint_applied = 0x0102;
    t.voltage = 0x0304;
    t.current = -2;  // 0xFFFE
    t.thrust = 0x0506;
    t.vib_rms = 0x0708;
    t.sensor_valid = sensor_valid::LOAD_CELL | sensor_valid::POWER_MODULE;
    t.link_quality = 100;
    t.uptime_ms = 0x11223344;

    const uint8_t expected[24] = {
        0x81,                    // type
        0x06,                    // state ARMED
        0x01, 0x02,              // error_code 0x0201
        0x0D, 0x0C, 0x0B, 0x0A,  // last_applied_seq
        0x02, 0x01,              // setpoint_applied
        0x04, 0x03,              // voltage
        0xFE, 0xFF,              // current
        0x06, 0x05,              // thrust
        0x08, 0x07,              // vib_rms
        0x05,                    // sensor_valid
        100,                     // link_quality
        0x44, 0x33, 0x22, 0x11,  // uptime_ms
    };
    TEST_ASSERT_EQUAL_MEMORY(expected, &t, sizeof(expected));
}

void test_hsk_resp_bytes() {
    HskResp h{};
    h.type = static_cast<uint8_t>(MsgType::HSK_RESP);
    h.state = static_cast<uint8_t>(SystemState::SHUTDOWN);
    h.error_code = static_cast<uint16_t>(ErrorCode::ABT_SELFTEST_FAILED);
    h.selftest_fail = selftest_fail::IMU_WHO_AM_I | selftest_fail::TARE_FAILED;
    h.cells = 6;
    h.fw_version = PROTOCOL_VERSION;
    h.voltage = 0x0A0B;

    const uint8_t expected[10] = {0x82, 0x08, 0x02, 0x05, 0x08, 0x10,
                                  6,    PROTOCOL_VERSION, 0x0B, 0x0A};
    TEST_ASSERT_EQUAL_MEMORY(expected, &h, sizeof(expected));
}

void test_message_direction() {
    TEST_ASSERT_TRUE(isCommandType(static_cast<uint8_t>(MsgType::CMD_POWER)));
    TEST_ASSERT_TRUE(isCommandType(static_cast<uint8_t>(MsgType::CMD_START)));
    TEST_ASSERT_TRUE(isResponseType(static_cast<uint8_t>(MsgType::TLM_STATUS)));
    TEST_ASSERT_TRUE(isResponseType(static_cast<uint8_t>(MsgType::HSK_RESP)));
    TEST_ASSERT_FALSE(isCommandType(static_cast<uint8_t>(MsgType::HSK_RESP)));
}

void test_state_validation() {
    TEST_ASSERT_TRUE(isValidSystemState(static_cast<uint8_t>(SystemState::SHUTDOWN)));
    TEST_ASSERT_FALSE(isValidSystemState(9));
    TEST_ASSERT_FALSE(isValidSystemState(0xFF));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cmd_power_bytes);
    RUN_TEST(test_tlm_status_bytes);
    RUN_TEST(test_hsk_resp_bytes);
    RUN_TEST(test_message_direction);
    RUN_TEST(test_state_validation);
    return UNITY_END();
}
