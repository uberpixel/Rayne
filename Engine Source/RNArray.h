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
			for(machine_uint i=_count; i>index; i--)
			{
				_data[i] = _data[i - 1];
			}
			
			_count ++;
			UpdateSizeIfNeeded();
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
		
		template<typename F>
		bool IsSorted(F&& func)
		{
			for(machine_uint i=0; i<_count - 1; i++)
			{
				if(func(_data[i], _data[i + 1]) < kRNCompareEqualTo)
					return false;
			}
			
			return true;
		}
		
		template<typename F>
		bool IsSorted(machine_uint begin, machine_uint end, F&& func)
		{
			for(machine_uint i=begin; i<end - 1; i++)
			{
				if(func(_data[i], _data[i + 1]) < kRNCompareEqualTo)
					return false;
			}
			
			return true;
		}
		
		template<typename F>
		void SortUsingFunction(F&& func)
		{
			QuickSort(0, _count, logf(_count) * 2, func);
			InsertionSort(func);
		}
		
	private:
		template<typename F>
		machine_uint Partition(machine_uint begin, machine_uint end, F&& func)
		{
			machine_uint pivot = begin;
			machine_uint middle = (begin + end) / 2;
			
			if(func(_data[middle], _data[begin]) >= kRNCompareEqualTo)
				pivot = middle;
			
			if(func(_data[pivot], _data[end]) >= kRNCompareEqualTo)
				pivot = end;
			
			std::swap(_data[pivot], _data[begin]);
			pivot = begin;
			
			while(begin < end)
			{
				if(func(_data[begin], _data[end]) <= kRNCompareEqualTo)
				{
					std::swap(_data[pivot], _data[begin]);
					pivot ++;
				}
				
				begin ++;
			}
			
			std::swap(_data[pivot], _data[end]);
			return pivot;
		}
		
		template<typename F>
		void QuickSort(machine_uint begin, machine_uint end, uint32 depth, F&& func)
		{
			if(begin < end)
			{
				if(depth > 0)
				{
					if(IsSorted(begin, end, func))
						return;
					
					machine_uint pivot = Partition(begin, end, func);
					if(pivot == 0)
						return;
					
					QuickSort(begin, pivot - 1, depth - 1, func);
					QuickSort(pivot + 1, end, depth - 1, func);
				}
			}
		}
		
		template<typename F>
		void InsertionSort(F&& func)
		{
			for(machine_uint i=1; i<_count; i++)
			{
				if(func(_data[i-1], _data[i]) <= kRNCompareEqualTo)
					continue;
				
				T value = _data[i];
				std::swap(_data[i], _data[i-1]);
				
				machine_uint j;
				for(j=i-1; j>=1; j--)
				{
					if(func(_data[j-1], value) <= kRNCompareEqualTo)
						break;
					
					std::swap(_data[j], _data[j-1]);
				}
				
				_data[j] = value;
			}
		}
		
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
		
		void InsertObjectAtIndex(T *object, machine_uint index)
		{
			for(machine_uint i=_count; i>index; i--)
			{
				_data[i] = _data[i - 1];
			}
			
			_data[index] = object->template Retain<T>();
			_count ++;
			
			UpdateSizeIfNeeded();
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
		
	private:
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
}

#endif /* __RAYNE_ARRAY_H__ */
