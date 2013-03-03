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
	class __ArrayStore
	{
	public:
		machine_uint Count() const
		{
			return _count;
		}
		
		void RemoveAllObjects()
		{
			_count = 0;
		}
		
		void ShrinkToFit()
		{
			T *tdata = new T[(_count + 1)];
			
			if(tdata)
			{
				for(machine_uint i=0; i<_count; i++)
					std::swap(tdata[i], _data[i]);
				
				delete [] _data;
				
				_data = tdata;
				_size = _count + 1;
			}
		}
		
		template<typename F>
		bool IsSorted(F&& func)
		{
			for(machine_uint i=0; i<this->_count - 1; i++)
			{
				if(func(this->_data[i], this->_data[i + 1]) < kRNCompareEqualTo)
					return false;
			}
			
			return true;
		}
		
		template<typename F>
		bool IsSorted(machine_uint begin, machine_uint end, F&& func)
		{
			for(machine_uint i=begin; i<end - 1; i++)
			{
				if(func(this->_data[i], this->_data[i + 1]) < kRNCompareEqualTo)
					return false;
			}
			
			return true;
		}
		
		template<typename F>
		void SortUsingFunction(F&& func, machine_uint offset = 0)
		{
			//QuickSort(offset, this->_count, logf(this->_count) * 2, func);
			InsertionSort(func, offset);
		}
		
	protected:
		__ArrayStore()
		{
			_size  = 5;
			_count = 0;
			
			_data = new T[_size];
		}
		
		__ArrayStore(machine_uint capacity)
		{
			_size  = capacity > 5 ? capacity : 5;
			_count = 0;
			
			_data = new T[_size];
		}
		
		virtual ~__ArrayStore()
		{
			delete [] _data;
		}
		
		void UpdateSizeIfNeeded()
		{
			if(_count >= _size)
			{
				machine_uint tsize = _size * 2;
				T *tdata = new T[tsize];
				
				if(tdata)
				{
					for(machine_uint i=0; i<_count; i++)
						std::swap(tdata[i], _data[i]);
					
					delete [] _data;
					
					_size = tsize;
					_data = tdata;
				}
				
				return;
			}
			
			machine_uint tsize = _size / 2;
			
			if(tsize >= _count && tsize > 5)
			{
				T *tdata = new T[tsize];
				
				if(tdata)
				{
					for(machine_uint i=0; i<_count; i++)
						std::swap(tdata[i], _data[i]);
					
					delete [] _data;
					
					_size = tsize;
					_data = tdata;
				}
			}
		}
		
		template<typename F>
		machine_uint Partition(machine_uint begin, machine_uint end, F&& func)
		{
			machine_uint pivot = begin;
			machine_uint middle = (begin + end) / 2;
			
			if(func(this->_data[middle], this->_data[begin]) >= kRNCompareEqualTo)
				pivot = middle;
			
			if(func(this->_data[pivot], this->_data[end]) >= kRNCompareEqualTo)
				pivot = end;
			
			std::swap(this->_data[pivot], this->_data[begin]);
			pivot = begin;
			
			while(begin < end)
			{
				if(func(this->_data[begin], this->_data[end]) <= kRNCompareEqualTo)
				{
					std::swap(this->_data[pivot], this->_data[begin]);
					pivot ++;
				}
				
				begin ++;
			}
			
			std::swap(this->_data[pivot], this->_data[end]);
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
		void InsertionSort(F&& func, machine_uint offset)
		{
			offset ++;
			
			for(machine_uint i=offset; i<this->_count; i++)
			{
				if(func(this->_data[i-1], this->_data[i]) <= kRNCompareEqualTo)
					continue;
				
				T value = this->_data[i];
				std::swap(this->_data[i], this->_data[i-1]);
				
				machine_uint j;
				for(j=i-1; j>=offset; j--)
				{
					if(func(this->_data[j-1], this->_data[j]) <= kRNCompareEqualTo)
						break;
					
					std::swap(this->_data[j], this->_data[j-1]);
				}
				
				this->_data[j] = value;
			}
		}
		
		machine_uint _size;
		machine_uint _count;
		T *_data;
	};
	
	
	template <class T>
	class __ArrayCore<T, false> : public __ArrayStore<T>, public Object
	{
	public:
		__ArrayCore() :
			__ArrayStore<T>()
		{}
		
		__ArrayCore(machine_uint capacity) :
			__ArrayStore<T>(capacity)
		{}
		
		__ArrayCore(const __ArrayCore<T, false>& other) :
			__ArrayStore<T>(other._size)
		{
			this->_count = other._count;
			
			for(machine_uint i=0; i<this->_count; i++)
			{
				this->_data[i] = other._data[i];
			}
		}
		
		T& operator[](int index) const
		{
			return this->_data[index];
		}
		
		
		void AddObject(const T& object)
		{
			this->_data[this->_count ++] = object;
			this->UpdateSizeIfNeeded();
			
			std::vector<T> test;
			test.reserve(1024);
		}
		
		void InsertObjectAtIndex(const T& object, machine_uint index)
		{
			for(machine_uint i=this->_count; i>index; i--)
			{
				this->_data[i] = this->_data[i - 1];
			}
			
			this->_count ++;
			this->UpdateSizeIfNeeded();
		}
		
		void ReplaceObjectAtIndex(machine_uint index, const T& object)
		{
			this->_data[index] = object;
		}
		
		void RemoveObject(const T& object)
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				if(object == this->_data[i])
				{
					RemoveObjectAtIndex(i);
					return;
				}
			}
		}
		
		void RemoveObjectAtIndex(machine_uint index)
		{
			for(; index<this->_count - 1; index++)
			{
				this->_data[index] = this->_data[index + 1];
			}
			
			this->_count --;
			this->UpdateSizeIfNeeded();
		}
		
		void RemoveLastObject()
		{
			RemoveObjectAtIndex(this->_count - 1);
		}
		
		T& ObjectAtIndex(machine_uint index) const
		{
			return this->_data[index];
		}
		
		T& FirstObject() const
		{
			return this->_data[0];
		}
		
		T& LastObject() const
		{
			return this->_data[this->_count - 1];
		}
		
		machine_uint IndexOfObject(const T& object) const
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				if(object == this->_data[i])
					return i;
			}
			
			return RN_NOT_FOUND;
		}
		
		bool ContainsObject(const T& object) const
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				if(object == this->_data[i])
					return true;
			}
			
			return false;
		}
	};
	
	
	template <class T>
	class __ArrayCore<T, true> : public __ArrayStore<T *>, public Object
	{
	public:
		__ArrayCore() :
			__ArrayStore<T *>()
		{}
		
		__ArrayCore(machine_uint capacity) :
			__ArrayStore<T *>(capacity)
		{}
		
		__ArrayCore(const __ArrayCore<T, true>& other) :
			__ArrayStore<T *>(other._size)
		{
			this->_count = other._count;
			
			for(machine_uint i=0; i<this->_count; i++)
			{
				this->_data[i] = other._data[i]->template Retain<T>();
			}
		}
		
		virtual ~__ArrayCore()
		{
			for(machine_uint i=0; i<this->_count; i++)
				this->_data[i]->Release();
		}
		
		
		T* operator[](int index) const
		{
			return this->_data[index];
		}
		
		
		void AddObject(T *object)
		{
			this->_data[this->_count ++] = object->template Retain<T>();
			this->UpdateSizeIfNeeded();
		}
		
		void InsertObjectAtIndex(T *object, machine_uint index)
		{
			for(machine_uint i=this->_count; i>index; i--)
			{
				this->_data[i] = this->_data[i - 1];
			}
			
			this->_data[index] = object->template Retain<T>();
			this->_count ++;
			
			this->UpdateSizeIfNeeded();
		}
		
		void ReplaceObjectAtIndex(machine_uint index, T *object)
		{
			this->_data[index]->Release();
			this->_data[index] = object->template Retain<T>();
		}
		
		void RemoveObject(T *object)
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				if(object == this->_data[i])
				{
					RemoveObjectAtIndex(i);
					return;
				}
			}
		}
		
		void RemoveObjectAtIndex(machine_uint index)
		{
			this->_data[index]->Release();
			
			for(; index<this->_count - 1; index++)
				this->_data[index] = this->_data[index + 1];
			
			this->_count --;
			this->UpdateSizeIfNeeded();
		}
		
		void RemoveLastObject()
		{
			RN_ASSERT0(this->_count > 0);
			RemoveObjectAtIndex(this->_count - 1);
		}
		
		void RemoveAllObjects()
		{
			for(machine_uint i=0; i<this->_count; i++)
				this->_data[i]->Release();
			
			__ArrayStore<T *>::RemoveAllObjects();
		}
		
		T* ObjectAtIndex(machine_uint index) const
		{
			if(index >= this->_count)
				return 0;
				
			return this->_data[index];
		}
		
		T* FirstObject() const
		{
			if(this->_count == 0)
				return 0;
			
			return this->_data[0];
		}
		
		T* LastObject() const
		{
			if(this->_count == 0)
				return 0;
			
			return this->_data[this->_count - 1];
		}
		
		machine_uint IndexOfObject(T *object) const
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				if(object->IsEqual(this->_data[i]))
					return i;
			}
			
			return RN_NOT_FOUND;
		}
		
		bool ContainsObject(T *object) const
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				if(object->IsEqual(this->_data[i]))
					return true;
			}
			
			return false;
		}
	};
	
	template <typename T>
	using Array = __ArrayCore<T, std::is_base_of<Object, T>::value>;
}

#endif /* __RAYNE_ARRAY_H__ */
