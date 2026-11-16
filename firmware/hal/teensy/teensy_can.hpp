#pragma once

#include <FlexCAN_T4.h>

#include "hal/can.hpp"

namespace hal {

class TeensyCan : public ICan {
public:
    bool begin() override;
    bool send(const CanFrame& frame) override;
    bool receive(CanFrame& frame) override;
    bool addFilter(const CanFilter& filter) override;
    void clearFilters() override;
    CanBusState busState() const override;
    bool rxOverflowed() const override { return rx_overflowed_; }
    void clearRxOverflow() override { rx_overflowed_ = false; }

private:
    static void receiveCallback(const CAN_message_t& message);
    static bool accepted(const CanFrame& frame);
    bool push(const CanFrame& frame);
    bool pop(CanFrame& frame);

    static TeensyCan* active_instance_;
    FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can_;
    CanFrame queue_[CAN_RX_QUEUE_CAPACITY] = {};
    CanFilter filters_[CAN_MAX_FILTERS] = {};
    size_t read_index_ = 0;
    size_t write_index_ = 0;
    size_t filter_count_ = 0;
    volatile bool rx_overflowed_ = false;
};

}  // namespace hal