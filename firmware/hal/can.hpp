#pragma once

#include <stddef.h>
#include <stdint.h>

namespace hal {

constexpr uint32_t CAN_BITRATE_BPS = 1000000;  // TST-01.PM-1: PM08-CAN default.
constexpr uint16_t PM08_NODE_STATUS_TYPE_ID = 341;
constexpr uint16_t PM08_CIRCUIT_STATUS_TYPE_ID = 1091;
constexpr uint16_t PM08_BATTERY_INFO_TYPE_ID = 1092;

constexpr size_t CAN_MAX_DATA_LENGTH = 8;
constexpr size_t CAN_RX_QUEUE_CAPACITY = 32;
constexpr size_t CAN_MAX_FILTERS = 8;

enum class CanIdFormat : uint8_t { Standard = 0, Extended = 1 };

struct CanFrame {
	uint32_t id = 0;
	CanIdFormat format = CanIdFormat::Standard;
	uint8_t length = 0;
	uint8_t data[CAN_MAX_DATA_LENGTH] = {};
	uint32_t received_at_us = 0;
};

struct CanFilter {
	uint32_t id = 0;
	CanIdFormat format = CanIdFormat::Standard;
};

enum class CanBusState : uint8_t { ErrorActive = 0, ErrorPassive, BusOff };

class ICan {
public:
	virtual ~ICan() = default;
	virtual bool begin() = 0;
	virtual bool send(const CanFrame& frame) = 0;
	virtual bool receive(CanFrame& frame) = 0;
	virtual bool addFilter(const CanFilter& filter) = 0;
	virtual void clearFilters() = 0;
	virtual CanBusState busState() const = 0;
	virtual bool rxOverflowed() const = 0;
	virtual void clearRxOverflow() = 0;
};

}  // namespace hal
