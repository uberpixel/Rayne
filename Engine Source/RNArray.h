//
//  RNArray.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ARRAY_H__
#define __RAYNE_ARRAY_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Array : public Object
	{
	public:
		Array()
		{
			_size  = 5;
			_count = 0;
			
			_data = new Object *[_size];
		}
		
		Array(machine_uint size)
		{
			_size  = size > 5 ? size : 5;
			_count = 0;
			
			_data = new Object *[_size];
		}
		
		Array(const Array& other)
		{
			_size  = other._size;
			_count = other._count;
			
			_data = new Object *[_size];
			
			for(machine_uint i=0; i<_count; i++)
			{
				_data[i] = other._data[i]->Retain();
			}
		}
		
		~Array() override
		{
			for(machine_uint i=0; i<_count; i++)
			{
				_data[i]->Release();
			}
			
			delete [] _data;
		}
		
		
		Object *operator [](int index) const
		{
			return _data[index];
		}
		
		Object *operator [](machine_uint index) const
		{
			return _data[index];
		}
		
		
		
		void AddObject(Object *object)
		{
			UpdateSizeIfNeeded(_count + 1);
			_data[_count ++] = object->Retain();
		}
		
		void InsertObjectAtIndex(Object *object, machine_uint index)
		{
			UpdateSizeIfNeeded(_count + 1);
			
			auto begin = _data + index;
			std::move(begin, _data + _count, begin + 1);
			
			_data[index] = object->Retain();
			_count ++;
		}
		
		void InsertObjectsAtIndex(const Array& other, machine_uint index)
		{
			UpdateSizeIfNeeded(_count + other._count);
			
			auto begin = _data + index;
			std::move(begin, _data + _count, begin + other._count);
			
			std::copy(other._data, other._data + other._count, begin);
			_count += other._count;
			
			for(machine_uint i=0; i<other._count; i++)
			{
				_data[index + i]->Retain();
			}
		}
		
		void ReplaceObjectAtIndex(machine_uint index, Object *object)
		{
			_data[index]->Release();
			_data[index] = object->Retain();
		}
		
		
		void RemoveObject(Object *object)
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object->IsEqual(_data[i]))
				{
					RemoveObjectAtIndex(i);
					return;
				}
			}
		}
		
		void RemoveObjectAtIndex(machine_uint index)
		{
			_data[index]->Release();
			std::move(_data + (index + 1), _data + _count, _data + index);
			
			UpdateSizeIfNeeded(_count - 1);
			_count --;
		}
		
		void RemoveAllObjects()
		{
			for(machine_uint i=0; i<_count; i++)
			{
				_data[i]->Release();
			}
			
			_count = 0;
		}
		
		
		machine_uint IndexOfObject(Object *object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object->IsEqual(_data[i]))
					return i;
			}
			
			return kRNNotFound;
		}
		
		bool ContainsObject(Object *object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object->IsEqual(_data[i]))
					return true;
			}
			
			return false;
		}
		
		
		template<typename T=Object>
		T *ObjectAtIndex(machine_uint index) const
		{
			return static_cast<T *>(_data[index]);
		}
		
		template<typename T=Object>
		T* FirstObject() const
		{
			if(this->_count == 0)
				return 0;
			
			return static_cast<T *>(this->_data[0]);
		}
		
		template<typename T=Object>
		T* LastObject() const
		{
			if(this->_count == 0)
				return 0;
			
			return static_cast<T *>(_data[_count - 1]);
		}
		
		
		
		machine_uint Count() const
		{
			return _count;
		}
		
		machine_uint Capacity() const
		{
			return _size;
		}
		
		template<typename T=Object>
		const T *Data() const
		{
			return _data;
		}
		
		void ShrinkToFit()
		{
			Object **tdata = new Object *[(_count + 1)];
			
			if(tdata)
			{
				std::move(_data, _data + _count, tdata);
				delete [] _data;
				
				_data = tdata;
				_size = _count + 1;
			}
		}
		
	private:
		void UpdateSizeIfNeeded(size_t required)
		{
			if(required >= _size)
			{
				machine_uint tsize = MAX(required, _size * 2);
				Object **tdata = new Object *[tsize];
				
				if(tdata)
				{
					std::move(_data, _data + _count, tdata);
					delete [] _data;
					
					_size = tsize;
					_data = tdata;
				}
				
				return;
			}
			
			machine_uint tsize = _size >> 1;
			
			if(tsize >= required && tsize > 5)
			{
				Object **tdata = new Object *[tsize];
				
				if(tdata)
				{
					std::move(_data, _data + _count, tdata);
					delete [] _data;
					
					_size = tsize;
					_data = tdata;
				}
			}
		}

		
		Object **_data;
		machine_uint _count;
		machine_uint _size;
		
		RNDefineMeta(Array, Object)
	};
}

#endif /* __RAYNE_ARRAY_H__ */
