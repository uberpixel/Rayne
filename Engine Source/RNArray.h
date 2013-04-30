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
	template <typename T, bool B>
	class __ArrayCore : public Object
	{
	};
	
	template <typename T>
	class __ArrayStore
	{
	public:
		machine_uint Count() const
		{
			return _count;
		}
		
		machine_uint Capacity() const
		{
			return _size;
		}
		
		T *Data() const
		{
			return _data;
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
		
		void UpdateSizeIfNeeded(size_t required)
		{
			if(required >= _size)
			{
				machine_uint tsize = MAX(required, _size * 2);
				T *tdata = new T[tsize];
				
				if(tdata)
				{
					for(machine_uint i=0; i<_count; i++)
						tdata[i] = std::move(_data[i]);
					
					delete [] _data;
					
					_size = tsize;
					_data = tdata;
				}
				
				return;
			}
			
			machine_uint tsize = _size / 2;
			
			if(tsize >= required && tsize > 5)
			{
				T *tdata = new T[tsize];
				
				if(tdata)
				{
					for(machine_uint i=0; i<_count; i++)
						tdata[i] = std::move(_data[i]);
					
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
	
	
	template <typename T>
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
			std::copy(other._data, other._data + other._count, this->_data);
		}
		
		T& operator[](int index) const
		{
			return this->_data[index];
		}
		
		__ArrayCore<T, false>& operator =(const __ArrayCore<T, false>& other)
		{
			if(other._size > this->_size)
			{
				delete [] this->_data;
				this->_data = new T[(other._size)];
				
				this->_size = other._size;
			}
			
			std::copy(other._data, other._data + other._count, this->_data);
			this->_count = other._count;
			
			return *this;
		}
		
		void AddObject(const T& object)
		{
			this->UpdateSizeIfNeeded(this->_count + 1);
			this->_data[this->_count ++] = object;
		}
		
		void InsertObjectsAtIndex(const __ArrayCore<T, false>& other, machine_uint index)
		{
			this->UpdateSizeIfNeeded(this->_count + other._count);
			auto begin = this->_data + index;
			
			std::move(begin, this->_data + this->_count, begin + other._count);
			std::copy(other._data, other._data + other._count, begin);
			
			this->_count += other._count;
		}
		
		void InsertObjectAtIndex(const T& object, machine_uint index)
		{
			this->UpdateSizeIfNeeded(this->_count + 1);
			
			auto begin = this->_data + index;
			
			std::move(begin, this->_data + this->_count, begin + 1);
			this->_data[index] = object;
			this->_count ++;
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
			std::move(this->_data + (index + 1), this->_data + this->_count, this->_data + index);
			this->UpdateSizeIfNeeded(this->_count - 1);
			this->_count --;
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
			
			return kRNNotFound;
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
	
	
	template <typename T>
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
				this->_data[i] = other._data[i]->Retain();
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
		
		__ArrayCore<T, true>& operator =(const __ArrayCore<T, true>& other)
		{
			for(machine_uint i=0; i<this->_count; i++)
			{
				this->_data[i]->Release();
			}
			
			if(other._size > this->_size)
			{
				delete [] this->_data;
				this->_data = new T *[(other._size)];
				
				this->_size = other._size;
			}
			
			std::copy(other._data, other._data + other._count, this->_data);
			this->_count = other._count;
			
			for(machine_uint i=0; i<this->_count; i++)
			{
				this->_data[i]->Retain();
			}
			
			return *this;
		}
		
		
		void AddObject(T *object)
		{
			this->UpdateSizeIfNeeded(this->_count + 1);
			this->_data[this->_count ++] = object->Retain();
		}
		
		void InsertObjectsAtIndex(const __ArrayCore<T, true>& other, machine_uint index)
		{
			this->UpdateSizeIfNeeded(this->_count + other._count);
			auto begin = this->_data + index;
			
			std::move(begin, this->_data + this->_count, begin + other._count);
			std::copy(other._data, other._data + other._count, begin);
			
			this->_count += other._count;
			
			for(machine_uint i=0; i<other._count; i++)
			{
				this->_data[index + i]->Retain();
			}
		}
		
		void InsertObjectAtIndex(T *object, machine_uint index)
		{
			this->UpdateSizeIfNeeded(this->_count + 1);
			
			auto begin = this->_data + index;
			
			std::move(begin, this->_data + this->_count, begin + 1);
			this->_data[index] = object->Retain();
			this->_count ++;
		}
		
		void ReplaceObjectAtIndex(machine_uint index, T *object)
		{
			this->_data[index]->Release();
			this->_data[index] = object->Retain();
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
			std::move(this->_data + (index + 1), this->_data + this->_count, this->_data + index);
			
			this->UpdateSizeIfNeeded(this->_count - 1);
			this->_count --;
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
			
			return kRNNotFound;
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
