//
//  RNArray.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ARRAY_H__
#define __RAYNE_ARRAY_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	template <class T, bool B>
	class __ArrayCore : public Object
	{
	};
	
	
	template <class T>
	class __ArrayCore<T, false> : public Object
	{
	public:
		__ArrayCore()
		{
			_size  = 5;
			_count = 0;
			
			_data = (T *)malloc(_size * sizeof(T));
		}
		
		~__ArrayCore()
		{
			free(_data);
		}
		
		
		T& operator[](int index) const
		{
			return _data[index];
		}
		
		
		void AddObject(T object)
		{
			_data[_count ++] = object;
			UpdateSizeIfNeeded();
		}
		
		void InsertObjectAtIndex(T object, machine_uint index)
		{
			// Well, fuck
			throw ErrorException(0);
		}
		
		void ReplaceObjectAtIndex(machine_uint index, T object)
		{
			_data[index] = object;
		}
		
		void RemoveObject(T object)
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object == _data[i])
				{
					RemoveObjectAtIndex(i);
					return;
				}
			}
		}
		
		void RemoveObjectAtIndex(machine_uint index)
		{
			for(; index<_count - 1; index++)
			{
				_data[index] = _data[index + 1];
			}
			
			_count --;
			UpdateSizeIfNeeded();
		}
		
		void RemoveLastObject()
		{
			RemoveObjectAtIndex(_count - 1);
		}
		
		void RemoveAllObjects()
		{
			_count = 0;
			
			T *tdata = (T *)realloc(_data, 5 * sizeof(T));
			if(tdata)
			{
				_data = tdata;
				_size = 5;
			}
		}
		
		T& ObjectAtIndex(machine_uint index) const
		{
			return _data[index];
		}
		
		T& FirstObject() const
		{
			return _data[0];
		}
		
		T& LastObject() const
		{
			return _data[_count - 1];
		}
		
		machine_uint IndexOfObject(T object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object == _data[i])
					return i;
			}
			
			return RN_NOT_FOUND;
		}
		
		bool ContainsObject(T object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object == _data[i])
					return true;
			}
			
			return false;
		}
		
		machine_uint Count() const
		{
			return _count;
		}
			
		void ShrinkToFit()
		{
			T *temp = (T *)realloc(_data, _count * sizeof(T));
			if(temp)
			{
				_data = temp;
				_size = _count;
			}
		}
		
	private:
		void UpdateSizeIfNeeded()
		{
			if(_count >= _size)
			{
				machine_uint tsize = _size * 2;
				T *tdata = (T *)realloc(_data, tsize * sizeof(T));
				
				if(tdata)
				{
					_size = tsize;
					_data = tdata;
				}
				
				return;
			}
			
			machine_uint tsize = _size / 2;
			
			if(tsize >= _count && tsize > 5)
			{
				T *tdata = (T *)realloc(_data, tsize * sizeof(T));
				
				if(tdata)
				{
					_size = tsize;
					_data = tdata;
				}
			}
		}
		
		machine_uint _size;
		machine_uint _count;
		T *_data;
	};
	
	
	template <class T>
	class __ArrayCore<T, true> : public Object
	{
	public:
		__ArrayCore()
		{
			_size  = 5;
			_count = 0;
			
			_data = (T **)malloc(_size * sizeof(T *));
		}
		
		~__ArrayCore()
		{
			for(machine_uint i=0; i<_count; i++)
				_data[i]->Release();
			
			free(_data);
		}
		
		
		T& operator[](int index) const
		{
			return _data[index];
		}
		
		
		void AddObject(T *object)
		{
			_data[_count ++] = object->template Retain<T>();
			UpdateSizeIfNeeded();
		}
		
		void InsertObjectAtIndex(T object, machine_uint index)
		{
			// Well, fuck
			throw ErrorException(0);
		}
		
		void ReplaceObjectAtIndex(machine_uint index, T *object)
		{
			_data[index]->Release();
			_data[index] = object->template Retain<T>();
		}
		
		void RemoveObject(T *object)
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object == _data[i])
				{
					RemoveObjectAtIndex(i);
					return;
				}
			}
		}
		
		void RemoveObjectAtIndex(machine_uint index)
		{
			_data[index]->Release();
			
			for(; index<_count - 1; index++)
				_data[index] = _data[index + 1];
			
			_count --;
			UpdateSizeIfNeeded();
		}
		
		void RemoveLastObject()
		{
			RN_ASSERT0(_count > 0);
			RemoveObjectAtIndex(_count - 1);
		}
		
		void RemoveAllObjects()
		{
			for(machine_uint i=0; i<_count; i++)
				_data[i]->Release();
			
			_count = 0;
			
			T **tdata = (T **)realloc(_data, 5 * sizeof(T *));
			if(tdata)
			{
				_data = tdata;
				_size = 5;
			}
		}
		
		T* ObjectAtIndex(machine_uint index) const
		{
			if(index >= _count)
				return 0;
				
			return _data[index];
		}
		
		T* FirstObject() const
		{
			if(_count == 0)
				return 0;
			
			return _data[0];
		}
		
		T* LastObject() const
		{
			if(_count == 0)
				return 0;
			
			return _data[_count - 1];
		}
		
		machine_uint IndexOfObject(T *object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object->IsEqual(_data[i]))
					return i;
			}
			
			return RN_NOT_FOUND;
		}
		
		bool ContainsObject(T *object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				if(object->IsEqual(_data[i]))
					return true;
			}
			
			return false;
		}
		
		machine_uint Count() const
		{
			return _count;
		}
		
		void ShrinkToFit()
		{
			T **temp = (T **)realloc(_data, _count * sizeof(T *));
			if(temp)
			{
				_data = temp;
				_size = _count;
			}
		}
		
	protected:
		void UpdateSizeIfNeeded()
		{
			if(_count >= _size)
			{
				machine_uint tsize = _size * 2;
				T **tdata = (T **)realloc(_data, tsize * sizeof(T *));
				
				if(tdata)
				{
					_size = tsize;
					_data = tdata;
				}
				
				return;
			}
			
			machine_uint tsize = _size / 2;
			
			if(tsize >= _count && tsize > 5)
			{
				T **tdata = (T **)realloc(_data, tsize * sizeof(T *));
				
				if(tdata)
				{
					_size = tsize;
					_data = tdata;
				}
			}
		}
		
		machine_uint _size;
		machine_uint _count;
		T **_data;
	};
	
	template <typename T>
	using Array = __ArrayCore<T, std::is_base_of<Object, T>::value>;
	
	static_assert(std::is_base_of<Object, Object>::value, "WTF?!");
}

#endif /* __RAYNE_ARRAY_H__ */
