//
//  RNRingbuffer.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RINGBUFFER_H__
#define __RAYNE_RINGBUFFER_H__

#include "RNBase.h"

namespace RN
{
	namespace stl
	{
		template<class T, class Alloc=std::allocator<T>>
		class ring_buffer
		{
		class __iterator;
		friend class __iterator;
		public:
			typedef Alloc allocator_type;
			typedef typename Alloc::value_type value_type;
			typedef typename Alloc::pointer pointer;
			typedef typename Alloc::const_pointer const_pointer;
			typedef typename Alloc::reference reference;
			typedef typename Alloc::const_reference const_reference;
			typedef typename Alloc::difference_type difference_type;
			typedef typename Alloc::size_type size_type;
			typedef __iterator iterator;
			typedef const __iterator const_iterator;
			
			ring_buffer(size_type capacity) :
				_content(new T[capacity])
			{
				_size      = 0;
				_capacity  = capacity;
				_rotations = 0;
				
				_begin = 0;
				_end   = 0;
			}
			
			~ring_buffer()
			{
				delete [] _content;
			}
			
			
			size_type capacity() const
			{
				return _capacity;
			}
			
			size_type size() const
			{
				return _size;
			}
			
			
			void push(const_reference value)
			{
				_content[_end] = value;
				increment();
			}
			
			void push(value_type&& value)
			{
				_content[_end] = std::move(value);
				increment();
			}
			
			void pop()
			{
				if(_size > 0)
				{
					_begin = (_begin + 1) % _capacity;
					_size --;
					
					return;
				}
				
				throw std::underflow_error("_size");
			}
			
			
			reference operator[](size_type index)
			{
				return at(index);
			}
			const_reference operator[](size_type index) const
			{
				return at(index);
			}
			
			reference at(size_type index)
			{
				index = (_begin + index) % _size;
				return _content[index];
			}
			const_reference at(size_type index) const
			{
				index = (_begin + index) % _size;
				return _content[index];
			}
			
			
			reference front()
			{
				return _content[_begin];
			}
			const_reference front() const
			{
				return _content[_begin];
			}
			
			reference back()
			{
				return _content[last_index()];
			}
			const_reference back() const
			{
				return _content[last_index()];
			}
			
			
			
			iterator begin()
			{
				if(_size == 0)
					return end();
				
				return iterator(*this, begin_marker());
			}
			const_iterator begin() const
			{
				if(_size == 0)
					return end();
				
				return iterator(*this, begin_marker());
			}
			
			iterator end()
			{
				return iterator(*this, end_marker());
			}
			const_iterator end() const
			{
				return iterator(*this, end_marker());
			}
			
		private:
			typedef std::tuple<size_type, size_type> marker;
			
			struct marker_type
			{
				marker_type(size_type tindex, size_type trotation)
				{
					index = tindex;
					rotation = trotation;
				}
				
				bool operator==(const marker_type &other) const
				{
					return (index == other.index && rotation == other.rotation);
				}
				bool operator!=(const marker_type &other) const
				{
					return (index != other.index || rotation != other.rotation);
				}
				
				size_type index;
				size_type rotation;
			};
			
			
			class __iterator : public std::iterator<std::bidirectional_iterator_tag, T>
			{
				friend class ring_buffer;
			public:
				bool operator== (const __iterator &other) const
				{
					return (_marker == other._marker);
				}
				bool operator!= (const __iterator &other) const
				{
					return (_marker != other._marker);
				}
				
				
				__iterator &operator-- ()
				{
					_buffer.decrement_marker(_marker);
					return *this;
				}
				__iterator operator-- (int)
				{
					iterator temp(*this);
					operator--();
					return temp;
				}
				
				__iterator &operator++ ()
				{
					_buffer.increment_marker(_marker);
					return *this;
				}
				__iterator operator++ (int)
				{
					iterator temp(*this);
					operator++();
					return temp;
				}
				
				
				const_reference operator* () const
				{
					return _buffer._content[_marker.index];
				}
				pointer operator-> () const
				{
					return &_buffer._content[_marker.index];
				}
				
			private:
				__iterator(const ring_buffer &buffer, marker_type marker) :
					_buffer(buffer),
					_marker(marker)
				{}
				
				marker_type _marker;
				const ring_buffer &_buffer;
			};
			
			size_type last_index() const
			{
				size_type index = (_end != 0) ? _end - 1 : _size - 1;
				return index;
			}
			
			
			marker_type begin_marker() const
			{
				return marker_type(_begin, _rotations);
			}
			marker_type end_marker() const
			{
				size_type rotation = _rotations;
				if(_begin >= _end)
					rotation ++;
				
				return marker_type(_end, rotation);
			}
			
			void increment_marker(marker_type &marker) const
			{
				if(marker == end_marker())
				{
					marker = begin_marker();
					return;
				}
				
				if(_size != _capacity)
				{
					marker.index = marker.index % _size;
					marker.index ++;
				}
				else
				{
					marker.index = (marker.index + 1) % _capacity;
				}
				
				if(marker.index == 0)
					marker.rotation ++;
			}
			void decrement_marker(marker_type &marker) const
			{
				if(marker == begin_marker())
				{
					marker = end_marker();
					return;
				}
				
				if(marker.index == 0)
				{
					marker.index = _size - 1;
					marker.rotation --;
				}
				else
				{
					marker.index --;
				}
			}
			
			
			
			void increment()
			{
				_end ++;
				_end %= _capacity;
				
				if(_end == 0)
					_rotations ++;
				
				if(RN_EXPECT_TRUE(_size == _capacity))
					_begin = _end;
				else
					_size ++;
			}
			
			size_type _begin;
			size_type _end;
			
			size_type _size;
			size_type _capacity;
			size_type _rotations;
			
			T *_content;
		};
		
		
		template<class T, size_t Size>
		class lock_free_ring_buffer
		{
		public:
			lock_free_ring_buffer() :
				_head(0),
				_tail(0)
			{}
			
			~lock_free_ring_buffer()
			{}
			
			
			bool push(const T &val)
			{
				size_t tail = _tail.load(std::memory_order_relaxed);
				size_t next = advance(tail);
				
				if(next != _head.load(std::memory_order_acquire))
				{
					_buffer[tail] = val;
					_tail.store(next, std::memory_order_release);
					
					return true;
				}
				
				return false;
			}
			
			bool push(T&& val)
			{
				size_t tail = _tail.load(std::memory_order_relaxed);
				size_t next = advance(tail);
				
				if(next != _head.load(std::memory_order_acquire))
				{
					_buffer[tail] = std::move(val);
					_tail.store(next, std::memory_order_release);
					
					return true;
				}
				
				return false;
			}
			
			bool pop(T &val)
			{
				size_t head = _head.load(std::memory_order_relaxed);
				
				if(head != _tail.load(std::memory_order_acquire))
				{
					val = std::move(_buffer[head]);
					_head.store(advance(head), std::memory_order_release);
					
					return true;
				}
				
				return false;
			}
			
			bool was_empty() const
			{
				return (_head.load() == _tail.load());
			}
			
			bool is_lock_free() const
			{
				return (_head.is_lock_free() && _tail.is_lock_free());
			}
			
		private:
			enum { capacity = Size + 1 };
			
			size_t advance(size_t index) const
			{
				return (index + 1) % capacity;
			}
			
			std::atomic<size_t> _head;
			std::atomic<size_t> _tail;
			
			std::array<T, capacity> _buffer;
		};
	}
}

#endif /* __RAYNE_RINGBUFFER_H__ */
