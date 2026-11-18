#include "hal/teensy/teensy_can.hpp"

namespace hal {

TeensyCan* TeensyCan::active_instance_ = nullptr;

bool TeensyCan::begin() {
    active_instance_ = this;
    can_.begin();
    can_.setBaudRate(CAN_BITRATE_BPS);
    can_.onReceive(receiveCallback);
    can_.enableFIFO();
    can_.enableFIFOInterrupt();
    return true;
}

bool TeensyCan::send(const CanFrame& frame) {
    if (frame.length > CAN_MAX_DATA_LENGTH) return false;
    CAN_message_t message = {};
    message.id = frame.id;
    message.len = frame.length;
    message.flags.extended = frame.format == CanIdFormat::Extended;
    for (uint8_t index = 0; index < frame.length; ++index) message.buf[index] = frame.data[index];
    return can_.write(message);
}

bool TeensyCan::receive(CanFrame& frame) { return pop(frame); }

bool TeensyCan::addFilter(const CanFilter& filter) {
    if (filter_count_ >= CAN_MAX_FILTERS) return false;
    filters_[filter_count_++] = filter;
    return true;
}

void TeensyCan::clearFilters() { filter_count_ = 0; }

CanBusState TeensyCan::busState() const {
    // FlexCAN ESR1 FLTCONF: 0 = active, 1 = passive, 2 = bus-off.
    const uint32_t fault_state = (FLEXCAN1_ESR1 >> 4) & 0x03U;
    if (fault_state == 2U) return CanBusState::BusOff;
    if (fault_state == 1U) return CanBusState::ErrorPassive;
    return CanBusState::ErrorActive;
}

void TeensyCan::receiveCallback(const CAN_message_t& message) {
    if (active_instance_ == nullptr) return;
    CanFrame frame;
    frame.id = message.id;
    frame.format = message.flags.extended ? CanIdFormat::Extended : CanIdFormat::Standard;
    frame.length = message.len > CAN_MAX_DATA_LENGTH ? CAN_MAX_DATA_LENGTH : message.len;
    frame.received_at_us = micros();
    for (uint8_t index = 0; index < frame.length; ++index) frame.data[index] = message.buf[index];
    active_instance_->push(frame);
}

bool TeensyCan::accepted(const CanFrame& frame) {
    if (active_instance_->filter_count_ == 0) return true;
    for (size_t index = 0; index < active_instance_->filter_count_; ++index) {
        const CanFilter& filter = active_instance_->filters_[index];
        if (filter.id == frame.id && filter.format == frame.format) return true;
    }
    return false;
}

bool TeensyCan::push(const CanFrame& frame) {
    if (!accepted(frame)) return false;
    const size_t next = (write_index_ + 1) % CAN_RX_QUEUE_CAPACITY;
    if (next == read_index_) { rx_overflowed_ = true; return false; }
    queue_[write_index_] = frame;
    write_index_ = next;
    return true;
}

bool TeensyCan::pop(CanFrame& frame) {
    noInterrupts();
    if (read_index_ == write_index_) {
        interrupts();
        return false;
    }
    frame = queue_[read_index_];
    read_index_ = (read_index_ + 1) % CAN_RX_QUEUE_CAPACITY;
    interrupts();
    return true;
}

}  // namespace hal