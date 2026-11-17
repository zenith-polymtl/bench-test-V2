// Tests the ISpi contract through MockSpi. The Teensy implementation cannot run on a PC;
// it follows the same contract and is checked by compiling it for the Teensy.
#include <unity.h>

#include "hal/mock/mock_spi.hpp"

using namespace hal;

namespace {
// ICM-42688-P: WHO_AM_I register, read = address | 0x80 (TST-01.IMU-1).
constexpr uint8_t REG_WHO_AM_I = 0x75;
constexpr uint8_t SPI_READ_FLAG = 0x80;
constexpr uint8_t WHO_AM_I_ICM42688 = 0x47;

// Minimal fake ICM-42688: answers a WHO_AM_I read, zeros for the rest.
// Byte 0 is the address phase (rx is don't-care), byte 1 is the data phase.
void fakeImu(void*, const uint8_t* tx, uint8_t* rx, size_t len) {
    if (len == 2 && tx[0] == (REG_WHO_AM_I | SPI_READ_FLAG)) rx[1] = WHO_AM_I_ICM42688;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_transfer_records_exactly_one_transaction() {
    MockSpi spi;
    const uint8_t tx[] = {0xA5, 0x01, 0x02};

    TEST_ASSERT_TRUE(spi.transfer(tx, nullptr, sizeof(tx)));

    TEST_ASSERT_EQUAL_UINT(1, spi.transactionCount());
    TEST_ASSERT_EQUAL_UINT(3, spi.transactionAt(0).len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(tx, spi.transactionAt(0).tx, sizeof(tx));
}

void test_queued_response_comes_back_in_rx_in_order() {
    MockSpi spi;
    const uint8_t first[] = {0x11, 0x22};
    const uint8_t second[] = {0x33, 0x44};
    spi.queueResponse(first, sizeof(first));
    spi.queueResponse(second, sizeof(second));

    uint8_t rx[2] = {};
    TEST_ASSERT_TRUE(spi.transfer(nullptr, rx, 2));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(first, rx, 2);
    TEST_ASSERT_TRUE(spi.transfer(nullptr, rx, 2));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(second, rx, 2);
}

void test_empty_queue_reads_zeros() {
    MockSpi spi;
    uint8_t rx[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    TEST_ASSERT_TRUE(spi.transfer(nullptr, rx, sizeof(rx)));

    const uint8_t zeros[4] = {};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(zeros, rx, sizeof(rx));
}

void test_null_tx_sends_zeros_and_null_rx_is_accepted() {
    MockSpi spi;
    uint8_t rx[3] = {};
    TEST_ASSERT_TRUE(spi.transfer(nullptr, rx, 3));  // pure read
    const uint8_t zeros[3] = {};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(zeros, spi.transactionAt(0).tx, 3);

    const uint8_t tx[] = {0x7E};
    TEST_ASSERT_TRUE(spi.transfer(tx, nullptr, 1));  // pure write
    TEST_ASSERT_EQUAL_UINT(2, spi.transactionCount());
}

void test_invalid_calls_are_refused_and_leave_no_trace() {
    MockSpi spi;
    const uint8_t tx[SPI_MAX_TRANSFER_BYTES + 1] = {};
    uint8_t rx[SPI_MAX_TRANSFER_BYTES + 1];
    for (auto& b : rx) b = 0xEE;

    TEST_ASSERT_FALSE(spi.transfer(tx, rx, 0));                            // empty
    TEST_ASSERT_FALSE(spi.transfer(tx, rx, SPI_MAX_TRANSFER_BYTES + 1));   // too long
    TEST_ASSERT_FALSE(spi.transfer(nullptr, nullptr, 4));                  // nothing to do

    TEST_ASSERT_EQUAL_UINT(0, spi.transactionCount());
    TEST_ASSERT_EQUAL_UINT8(0xEE, rx[0]);  // rx untouched on failure
}

void test_max_length_transfer_is_accepted() {
    MockSpi spi;
    uint8_t tx[SPI_MAX_TRANSFER_BYTES] = {};
    TEST_ASSERT_TRUE(spi.transfer(tx, nullptr, SPI_MAX_TRANSFER_BYTES));
}

void test_not_begun_device_refuses_everything() {
    MockSpi spi;
    spi.setBegun(false);
    uint8_t rx[1];

    TEST_ASSERT_FALSE(spi.transfer(nullptr, rx, 1));
    TEST_ASSERT_EQUAL_UINT(0, spi.transactionCount());
}

void test_history_overflow_is_flagged() {
    MockSpi spi;
    const uint8_t tx[] = {0x00};
    for (size_t i = 0; i < MockSpi::HISTORY_MAX; i++) spi.transfer(tx, nullptr, 1);
    TEST_ASSERT_FALSE(spi.historyOverflowed());

    spi.transfer(tx, nullptr, 1);

    TEST_ASSERT_TRUE(spi.historyOverflowed());
    TEST_ASSERT_EQUAL_UINT(MockSpi::HISTORY_MAX, spi.transactionCount());
}

void test_responder_serves_a_fake_chip_who_am_i() {
    // TST-01.IMU-1: the chip ID read must return 0x47.
    MockSpi spi;
    spi.setResponder(fakeImu, nullptr);
    const uint8_t tx[2] = {REG_WHO_AM_I | SPI_READ_FLAG, 0x00};
    uint8_t rx[2] = {};

    TEST_ASSERT_TRUE(spi.transfer(tx, rx, 2));

    TEST_ASSERT_EQUAL_UINT8(WHO_AM_I_ICM42688, rx[1]);
}

void test_queued_response_has_priority_over_responder() {
    MockSpi spi;
    spi.setResponder(fakeImu, nullptr);
    const uint8_t wrong_id[] = {0x00, 0x99};
    spi.queueResponse(wrong_id, sizeof(wrong_id));
    const uint8_t tx[2] = {REG_WHO_AM_I | SPI_READ_FLAG, 0x00};
    uint8_t rx[2] = {};

    spi.transfer(tx, rx, 2);

    TEST_ASSERT_EQUAL_UINT8(0x99, rx[1]);  // a test can simulate a wrong chip
}

void test_injected_failure_returns_false_then_recovers() {
    MockSpi spi;
    uint8_t rx[1];
    spi.failNextTransfers(2);

    TEST_ASSERT_FALSE(spi.transfer(nullptr, rx, 1));
    TEST_ASSERT_FALSE(spi.transfer(nullptr, rx, 1));
    TEST_ASSERT_EQUAL_UINT(0, spi.transactionCount());  // a failed bus fault leaves no frame
    TEST_ASSERT_TRUE(spi.transfer(nullptr, rx, 1));
    TEST_ASSERT_EQUAL_UINT(1, spi.transactionCount());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_transfer_records_exactly_one_transaction);
    RUN_TEST(test_queued_response_comes_back_in_rx_in_order);
    RUN_TEST(test_empty_queue_reads_zeros);
    RUN_TEST(test_null_tx_sends_zeros_and_null_rx_is_accepted);
    RUN_TEST(test_invalid_calls_are_refused_and_leave_no_trace);
    RUN_TEST(test_max_length_transfer_is_accepted);
    RUN_TEST(test_not_begun_device_refuses_everything);
    RUN_TEST(test_history_overflow_is_flagged);
    RUN_TEST(test_responder_serves_a_fake_chip_who_am_i);
    RUN_TEST(test_queued_response_has_priority_over_responder);
    RUN_TEST(test_injected_failure_returns_false_then_recovers);
    return UNITY_END();
}
