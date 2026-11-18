#include <unity.h>

#include "hal/mock/mock_can.hpp"

using namespace hal;

namespace
{
    CanFrame makeFrame(uint32_t id, CanIdFormat format, uint8_t length)
    {
        CanFrame frame{};
        frame.id = id;
        frame.format = format;
        frame.length = length;
        frame.received_at_us = 123456;
        for (uint8_t index = 0; index < length; ++index)
        {
            frame.data[index] = static_cast<uint8_t>(index + 1);
        }
        return frame;
    }
} // namespace

void setUp() {}
void tearDown() {}

void test_receive_is_non_blocking_when_queue_is_empty()
{
    MockCan can;
    CanFrame frame{};

    TEST_ASSERT_FALSE(can.receive(frame));
}

void test_received_frame_is_preserved()
{
    MockCan can;
    const CanFrame expected = makeFrame(0x12345678, CanIdFormat::Extended, 8);
    CanFrame actual{};

    TEST_ASSERT_TRUE(can.injectReceivedFrame(expected));
    TEST_ASSERT_TRUE(can.receive(actual));
    TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, sizeof(CanFrame));
    TEST_ASSERT_FALSE(can.receive(actual));
}

void test_filter_matches_id_and_format()
{
    MockCan can;
    const CanFilter filter{0x12345678, CanIdFormat::Extended};
    const CanFrame wrong_id = makeFrame(0x12345679, CanIdFormat::Extended, 1);
    const CanFrame wrong_format = makeFrame(0x12345678, CanIdFormat::Standard, 1);
    const CanFrame expected = makeFrame(0x12345678, CanIdFormat::Extended, 1);
    CanFrame actual{};

    TEST_ASSERT_TRUE(can.addFilter(filter));
    TEST_ASSERT_FALSE(can.injectReceivedFrame(wrong_id));
    TEST_ASSERT_FALSE(can.injectReceivedFrame(wrong_format));
    TEST_ASSERT_TRUE(can.injectReceivedFrame(expected));
    TEST_ASSERT_TRUE(can.receive(actual));
    TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, sizeof(CanFrame));
}

void test_receive_queue_reports_overflow()
{
    MockCan can;
    const CanFrame frame = makeFrame(341, CanIdFormat::Extended, 1);

    for (size_t index = 0; index < CAN_RX_QUEUE_CAPACITY - 1; ++index)
    {
        TEST_ASSERT_TRUE(can.injectReceivedFrame(frame));
    }
    TEST_ASSERT_FALSE(can.injectReceivedFrame(frame));
    TEST_ASSERT_TRUE(can.rxOverflowed());

    can.clearRxOverflow();
    TEST_ASSERT_FALSE(can.rxOverflowed());
}

void test_send_records_frame_and_rejects_oversized_payload()
{
    MockCan can;
    const CanFrame expected = makeFrame(1092, CanIdFormat::Extended, 4);
    CanFrame oversized = expected;
    oversized.length = CAN_MAX_DATA_LENGTH + 1;

    TEST_ASSERT_TRUE(can.send(expected));
    TEST_ASSERT_EQUAL_UINT(1, can.sentCount());
    TEST_ASSERT_EQUAL_MEMORY(&expected, &can.sentAt(0), sizeof(CanFrame));
    TEST_ASSERT_FALSE(can.send(oversized));
    TEST_ASSERT_EQUAL_UINT(1, can.sentCount());
}

void test_bus_state_is_reported()
{
    MockCan can;

    can.setBusState(CanBusState::ErrorActive);
    TEST_ASSERT_EQUAL(CanBusState::ErrorActive, can.busState());
    can.setBusState(CanBusState::ErrorPassive);
    TEST_ASSERT_EQUAL(CanBusState::ErrorPassive, can.busState());
    can.setBusState(CanBusState::BusOff); // ELE-05: CAN bus-off is a safety event.
    TEST_ASSERT_EQUAL(CanBusState::BusOff, can.busState());
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_receive_is_non_blocking_when_queue_is_empty);
    RUN_TEST(test_received_frame_is_preserved);
    RUN_TEST(test_filter_matches_id_and_format);
    RUN_TEST(test_receive_queue_reports_overflow);
    RUN_TEST(test_send_records_frame_and_rejects_oversized_payload);
    RUN_TEST(test_bus_state_is_reported);
    return UNITY_END();
}