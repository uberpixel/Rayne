//
//  RNRingBuffer.h
//  Rayne
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RINGBUFFER_H__
#define __RAYNE_RINGBUFFER_H__

#include "../Base/RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Set;
	class RingBuffer : public Object
	{
	public:
		RNAPI RingBuffer(size_t size);
		RNAPI RingBuffer(const RingBuffer *other);
		RNAPI RingBuffer(Deserializer *deserializer);
		RNAPI ~RingBuffer() override;
		
		RNAPI static RingBuffer *WithRingBuffer(const RingBuffer *other);
		RNAPI static RingBuffer *WithObjects(const std::initializer_list<Object *> objects);
		
		RNAPI void Serialize(Serializer *serializer) const override;
		RNAPI const String *GetDescription() const override;
		
		
		Object *operator [](int index) const
		{
			return _data[index];
		}
		
		Object *operator [](size_t index) const
		{
			return _data[index];
		}
		
		
		void Enumerate(const std::function<void (Object *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			
			for(size_t i = _head; i != _tail; i = GetNextIndex(i))
			{
				callback(_data[i], i, stop);
				
				if(stop)
					break;
			}
		}
		
		void EnumerateReverse(const std::function<void (Object *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			if(GetCount() == 0) return;
			
			for(size_t i = GetPreviousIndex(_tail); ; i = GetPreviousIndex(i))
			{
				callback(_data[i], i, stop);
				
				if(stop || i == _head)
					break;
			}
		}
		
		template<class T>
		void Enumerate(const std::function<void (T *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			
			for(size_t i = _head; i != _tail; i = GetNextIndex(i))
			{
				callback(static_cast<T *>(_data[i]), i, stop);
				
				if(stop)
					break;
			}
		}
		
		template<class T>
		void EnumerateReverse(const std::function<void (T *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			if(GetCount() == 0) return;
			
			for(size_t i = GetPreviousIndex(_tail); ; i = GetPreviousIndex(i))
			{
				callback(static_cast<T *>(_data[i]), i, stop);
				
				if(stop || i == _head)
					break;
			}
		}
		
		
		bool PushObject(Object *object)
		{
			//This will only add the new object if the ringbuffer isn't already full
			size_t next = GetNextIndex(_tail);

			if(next != _head)
			{
				_data[_tail] = object->Retain();
				_tail = next;

				return true;
			}

			return false;
		}
		
		void ForcePushObject(Object *object)
		{
			//This will overwrite the head with the new object if the ringbuffer is full
			size_t next = GetNextIndex(_tail);

			if(next == _head)
			{
				_data[_head]->Release();
				_head = GetNextIndex(_head);
			}
			
			_data[_tail] = object->Retain();
			_tail = GetNextIndex(_tail);
		}
		
		void ReplaceObjectAtIndex(size_t index, Object *object)
		{
			_data[index]->Release();
			_data[index] = object->Retain();
		}
		
		void RemoveAllObjects()
		{
			for(size_t i = _head; i != _tail; i = GetNextIndex(i))
			{
				_data[i]->Release();
			}
			
			_head = _tail = 0;
		}
		
		template<class T=Object>
		T *PopHead()
		{
			if(_head == _tail) return;
			
			T *object = _data[_head]->Downcast<T>();
			_head = GetNextIndex(_head);
			
			return object->Autorelease();
		}
		
		template<class T=Object>
		T *PopTail()
		{
			if(_head == _tail) return;
			
			_tail = GetPreviousIndex(_tail);
			T *object = _data[_tail]->Downcast<T>();
			
			return object->Autorelease();
		}
		
		void TruncateHead(size_t index)
		{
			//Remove all objects from head to index (not including index)
			while(_head != _tail && _head != index)
			{
				_data[_head]->Release();
				_head = GetNextIndex(_head);
			}
		}
		
		void TruncateTail(size_t index)
		{
			//Remove all objects from tail to index (not including index)
			index = GetNextIndex(index); //Otherwise the object at index would also be removed
			while(_head != _tail && _tail != index)
			{
				_tail = GetPreviousIndex(_tail);
				_data[_tail]->Release();
			}
		}
		
		
		size_t GetIndexOfObject(Object *object) const
		{
			for(size_t i = _head; i != _tail; i = GetNextIndex(i))
			{
				if(object->IsEqual(_data[i]))
					return i;
			}
			
			return kRNNotFound;
		}
		
		bool ContainsObject(Object *object) const
		{
			for(size_t i = _head; i != _tail; i = GetNextIndex(i))
			{
				if(object->IsEqual(_data[i]))
					return true;
			}
			
			return false;
		}
		
		
		template<class T=Object>
		T *GetObjectAtIndex(size_t index) const
		{
			return _data[index]->Downcast<T>();
		}
		
		template<class T=Object>
		T *GetHeadObject() const
		{
			if(_head == _tail)
				return nullptr;
			
			return _data[_head]->Downcast<T>();
		}
		
		template<class T=Object>
		T *GetTailObject() const
		{
			if(_head == _tail)
				return nullptr;
			
			return _data[GetPreviousIndex(_tail)]->Downcast<T>();
		}
		
		size_t GetHeadIndex() const
		{
			return _head;
		}
		
		size_t GetTailIndex() const
		{
			return _tail;
		}
		
		size_t GetCount() const
		{
			if(_tail >= _head) return _tail - _head;
			return _size - _head + _tail;
		}
		
		size_t GetCapacity() const
		{
			return _size;
		}
		
		const Object **GetData() const
		{
			return const_cast<const Object **>(_data);
		}
		
		size_t GetNextIndex(size_t index) const
		{
			return (index + 1) % _size;
		}
		
		size_t GetPreviousIndex(size_t index) const
		{
			if(index == 0) index = _size;
			return index - 1;
		}

	private:
		size_t _size;
		size_t _head;
		size_t _tail;
		Object **_data;
		
		__RNDeclareMetaInternal(RingBuffer)
	};
	
	RNObjectClass(RingBuffer)
}

#endif /* __RAYNE_RINGBUFFER_H__ */
