//
//  RNRingbuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RINGBUFFER_H__
#define __RAYNE_RINGBUFFER_H__

#include "../Base/RNBase.h"

namespace RN
{
	template<class T, size_t Size>
	class AtomicRingBuffer
	{
	public:
		AtomicRingBuffer() :
			_head(0),
			_tail(0)
		{}

		bool Push(const T &value)
		{
			size_t tail = _tail.load(std::memory_order_relaxed);
			size_t next = Advance(tail);

			if(next != _head.load(std::memory_order_acquire))
			{
				_buffer[tail] = value;
				_tail.store(next, std::memory_order_release);

				return true;
			}

			return false;
		}
		bool Push(T &&value)
		{
			size_t tail = _tail.load(std::memory_order_relaxed);
			size_t next = Advance(tail);

			if(next != _head.load(std::memory_order_acquire))
			{
				_buffer[tail] = std::move(value);
				_tail.store(next, std::memory_order_release);

				return true;
			}

			return false;
		}


		bool Pop(T &value)
		{
			size_t head = _head.load(std::memory_order_relaxed);

			if(head != _tail.load(std::memory_order_acquire))
			{
				value = std::move(_buffer[head]);
				_head.store(Advance(head), std::memory_order_release);

				return true;
			}

			return false;
		}


		bool WasEmpty() const
		{
			return (_head.load(std::memory_order_acquire) == _tail.load(std::memory_order_acquire));
		}

		bool IsLockFree() const
		{
			return (_head.is_lock_free() && _tail.is_lock_free());
		}

	private:
		RN_CONSTEXPR size_t Capacity = Size + 1;

		size_t Advance(size_t index) const
		{
			return (index + 1) % Capacity;
		}

		std::atomic<size_t> _head;
		std::atomic<size_t> _tail;

		std::array<T, Capacity> _buffer;
	};
}

#endif /* __RAYNE_RINGBUFFER_H__ */
