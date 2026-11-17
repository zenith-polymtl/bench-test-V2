#pragma once

#include "hal/can.hpp"

namespace hal
{

	class MockCan : public ICan
	{
	public:
		bool begin() override
		{
			started_ = true;
			return true;
		}
		bool send(const CanFrame &frame) override
		{
			if (frame.length > CAN_MAX_DATA_LENGTH)
				return false;
			if (sent_count_ >= CAN_RX_QUEUE_CAPACITY)
				return false;
			sent_[sent_count_++] = frame;
			return true;
		}
		bool receive(CanFrame &frame) override { return pop(frame); }
		bool addFilter(const CanFilter &filter) override
		{
			if (filter_count_ >= CAN_MAX_FILTERS)
				return false;
			filters_[filter_count_++] = filter;
			return true;
		}
		void clearFilters() override { filter_count_ = 0; }
		CanBusState busState() const override { return bus_state_; }
		bool rxOverflowed() const override { return rx_overflowed_; }
		void clearRxOverflow() override { rx_overflowed_ = false; }

		bool injectReceivedFrame(const CanFrame &frame) { return push(frame); }
		void setBusState(CanBusState state) { bus_state_ = state; }
		bool started() const { return started_; }
		size_t sentCount() const { return sent_count_; }
		const CanFrame &sentAt(size_t index) const { return sent_[index]; }

	private:
		bool accepted(const CanFrame &frame) const
		{
			if (filter_count_ == 0)
				return true;
			for (size_t index = 0; index < filter_count_; ++index)
			{
				if (filters_[index].id == frame.id && filters_[index].format == frame.format)
					return true;
			}
			return false;
		}
		bool push(const CanFrame &frame)
		{
			if (frame.length > CAN_MAX_DATA_LENGTH || !accepted(frame))
				return false;
			const size_t next = (write_index_ + 1) % CAN_RX_QUEUE_CAPACITY;
			if (next == read_index_)
			{
				rx_overflowed_ = true;
				return false;
			}
			queue_[write_index_] = frame;
			write_index_ = next;
			return true;
		}
		bool pop(CanFrame &frame)
		{
			if (read_index_ == write_index_)
				return false;
			frame = queue_[read_index_];
			read_index_ = (read_index_ + 1) % CAN_RX_QUEUE_CAPACITY;
			return true;
		}

		CanFrame queue_[CAN_RX_QUEUE_CAPACITY] = {};
		CanFrame sent_[CAN_RX_QUEUE_CAPACITY] = {};
		CanFilter filters_[CAN_MAX_FILTERS] = {};
		size_t read_index_ = 0;
		size_t write_index_ = 0;
		size_t sent_count_ = 0;
		size_t filter_count_ = 0;
		CanBusState bus_state_ = CanBusState::ErrorActive;
		bool rx_overflowed_ = false;
		bool started_ = false;
	};

} // namespace hal
