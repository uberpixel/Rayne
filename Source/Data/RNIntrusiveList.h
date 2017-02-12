//
//  RNIntrusiveList.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_INTRUSIVELIST_H_
#define __RAYNE_INTRUSIVELIST_H_

#include "../Base/RNBase.h"

namespace RN
{
	template<class T>
	class IntrusiveList
	{
	public:
		class Member
		{
		public:
			friend class IntrusiveList;

			Member(T *value) :
				_next(nullptr),
				_prev(nullptr),
				_value(value)
			{}

			T *Get() const { return _value; }
			Member *GetNext() const { return _next; }
			Member *GetPrevious() const { return _prev; }

		private:

			Member *_next;
			Member *_prev;
			T *_value;
		};

		IntrusiveList() :
			_head(0),
			_tail(0),
			_count(0)
		{}

		~IntrusiveList()
		{
			Member *value = _head;
			while(value)
			{
				Member *temp = value->_next;

				value->_next = value->_prev = nullptr;

				value = temp;
			}
		}

		void PushBack(Member &value)
		{
			value._prev = _tail;

			if(_tail)
				_tail->_next = &value;
			if(RN_EXPECT_FALSE(!_head))
				_head = &value;

			_tail = &value;
			_count ++;
		}
		void PushFront(Member &value)
		{
			value._next = _head;

			if(_head)
				_head->_prev = &value;
			if(RN_EXPECT_FALSE(!_tail))
				_tail = &value;

			_head = &value;
			_count ++;
		}

		Member *Erase(Member &value, bool returnNext = true)
		{
			return Erase(&value, returnNext);
		}

		Member *Erase(Member *value, bool returnNext = true)
		{
			Member *result = returnNext ? value->_next : value->_prev;

			if(value == _tail)
				_tail = value->_prev;
			if(value == _head)
				_head = value->_next;

			if(value->_next)
				value->_next->_prev = value->_prev;
			if(value->_prev)
				value->_prev->_next = value->_next;

			value->_next = nullptr;
			value->_prev = nullptr;

			_count --;

			return result;
		}

		size_t GetCount() const { return _count; }

		Member *GetHead() const { return _head; }
		Member *GetTail() const { return _tail; }

	private:
		Member *_head;
		Member *_tail;
		size_t _count;
	};
}


#endif /* __RAYNE_INTRUSIVELIST_H_ */
