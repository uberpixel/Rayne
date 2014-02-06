//
//  RNArray.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ARRAY_H__
#define __RAYNE_ARRAY_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Set;
	class Array : public Object
	{
	public:
		RNAPI Array();
		RNAPI Array(size_t size);
		RNAPI Array(const Array *other);
		RNAPI Array(const Set *set);
		
		RNAPI ~Array() override;
		
		RNAPI static Array *WithArray(const Array *other);
		RNAPI static Array *WithSet(const Set *set);
		RNAPI static Array *WithObjects(Object *first, ...);
		
		
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
			
			for(size_t i = 0; i < _count; i ++)
			{
				callback(_data[i], i, stop);
				
				if(stop)
					break;
			}
		}
		
		template<class T>
		void Enumerate(const std::function<void (T *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			
			for(size_t i = 0; i < _count; i ++)
			{
				callback(static_cast<T *>(_data[i]), i, stop);
				
				if(stop)
					break;
			}
		}
		
		
		void AddObject(Object *object)
		{
			UpdateSizeIfNeeded(_count + 1);
			_data[_count ++] = object->Retain();
		}
		
		void AddObjectsFromArray(const Array *other)
		{
			UpdateSizeIfNeeded(_count + other->_count);
			
			for(size_t i = 0; i < other->_count; i ++)
			{
				_data[_count ++] = other->_data[i]->Retain();
			}
		}
		
		void InsertObjectAtIndex(Object *object, size_t index)
		{
			UpdateSizeIfNeeded(_count + 1);
			
			auto begin = _data + index;
			std::move(begin, _data + _count, begin + 1);
			
			_data[index] = object->Retain();
			_count ++;
		}
		
		void InsertObjectsAtIndex(const Array *other, size_t index)
		{
			UpdateSizeIfNeeded(_count + other->_count);
			
			auto begin = _data + index;
			std::move(begin, _data + _count, begin + other->_count);
			
			std::copy(other->_data, other->_data + other->_count, begin);
			_count += other->_count;
			
			for(size_t i=0; i < other->_count; i++)
			{
				_data[index + i]->Retain();
			}
		}
		
		void ReplaceObjectAtIndex(size_t index, Object *object)
		{
			_data[index]->Release();
			_data[index] = object->Retain();
		}
		
		
		void RemoveObject(Object *object)
		{
			for(size_t i = 0; i < _count; i++)
			{
				if(object->IsEqual(_data[i]))
				{
					RemoveObjectAtIndex(i);
					return;
				}
			}
		}
		
		void RemoveObjectAtIndex(size_t index)
		{
			_data[index]->Release();
			std::move(_data + (index + 1), _data + _count, _data + index);
			
			UpdateSizeIfNeeded(_count - 1);
			_count --;
		}
		
		void RemoveAllObjects()
		{
			for(size_t i = 0; i < _count; i ++)
			{
				_data[i]->Release();
			}
			
			_count = 0;
		}
		
		
		size_t GetIndexOfObject(Object *object) const
		{
			for(size_t i = 0; i < _count; i ++)
			{
				if(object->IsEqual(_data[i]))
					return i;
			}
			
			return kRNNotFound;
		}
		
		bool ContainsObject(Object *object) const
		{
			for(size_t i = 0; i < _count; i ++)
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
		T *GetFirstObject() const
		{
			if(_count == 0)
				return nullptr;
			
			return _data[0]->Downcast<T>();
		}
		
		template<class T=Object>
		T *GetLastObject() const
		{
			if(_count == 0)
				return nullptr;
			
			return _data[_count - 1]->Downcast<T>();
		}
		
		
		
		size_t GetCount() const
		{
			return _count;
		}
		
		size_t GetCapacity() const
		{
			return _size;
		}
		
		template<class T=Object>
		const T **GetData() const
		{
			return _data;
		}
		
		RNAPI void ShrinkToFit();
		
		template<class T=Object>
		void Sort(const std::function<ComparisonResult (const T *left, const T *right)>& function)
		{
			std::sort(_data, _data + _count, [&](const Object *left, const Object *right) -> bool {
				ComparisonResult result = function(static_cast<const T *>(left), static_cast<const T *>(right));
				return (result == ComparisonResult::LessThan);
			});
		}
		
	private:
		RNAPI void UpdateSizeIfNeeded(size_t required);
		
		Object **_data;
		size_t _count;
		size_t _size;
		
		RNDeclareMetaWithTraits(Array, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_ARRAY_H__ */
