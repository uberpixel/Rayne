//
//  RNArray.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ARRAY_H__
#define __RAYNE_ARRAY_H__

#include "../Base/RNBase.h"
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
		RNAPI Array(Deserializer *deserializer);
		RNAPI ~Array() override;
		
		RNAPI static Array *WithArray(const Array *other);
		RNAPI static Array *WithSet(const Set *set);
		RNAPI static Array *WithObjects(const std::initializer_list<Object *> objects);
		
		RNAPI void Serialize(Serializer *serializer) const override;
		RNAPI const String *GetDescription() const override;
		
		Array *GetObjectsPassingTest(const std::function<bool (Object *, bool &)> &callback) const
		{
			bool stop = false;
			Array *subarray = new Array();
			
			for(size_t i = 0; i < _count; i ++)
			{
				if(callback(_data[i], stop))
					subarray->AddObject(_data[i]);
				
				if(stop)
					break;
			}
			
			return subarray->Autorelease();
		}
		
		template<class T>
		Array *GetObjectsPassingTest(const std::function<bool (T *, bool &)> &callback) const
		{
			bool stop = false;
			Array *subarray = new Array();
			
			for(size_t i = 0; i < _count; i ++)
			{
				if(callback(static_cast<T *>(_data[i]), stop))
					subarray->AddObject(_data[i]);
				
				if(stop)
					break;
			}
			
			return subarray->Autorelease();
		}
		
		
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
		
		void EnumerateReverse(const std::function<void (Object *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			if(_count == 0) return;
			
			for(size_t i = _count - 1; ; i --)
			{
				callback(_data[i], i, stop);
				
				if(stop || i == 0)
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
		
		template<class T>
		void EnumerateReverse(const std::function<void (T *, size_t, bool &)>& callback) const
		{
			bool stop = false;
			if(_count == 0) return;
			
			for(size_t i = _count - 1; ; i --)
			{
				callback(static_cast<T *>(_data[i]), i, stop);
				
				if(stop || i == 0)
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
		
		bool ContainsObject(const Object *object) const
		{
			for(size_t i = 0; i < _count; i ++)
			{
				if(object->IsEqual(_data[i]))
					return true;
			}
			
			return false;
		}
		
		bool IsEqual(const Object *temp) const override
		{
			if(!temp)
				return false;
			
			const Array *other = temp->Downcast<Array>();
			if(!other)
				return false;

			if(GetCount() != other->GetCount())
				return false;
			
			for(size_t i = 0; i < _count; i++)
			{
				if(!_data[i]->IsEqual(other->_data[i])) return false;
			}
			
			return true;
		}

		bool IsEqualLite(const Array *temp) const //Only checks if objects are the same (same address), not if they are equal. Also reduced a little bit of other overhead.
		{
			if(GetCount() != temp->GetCount())
				return false;

			for(size_t i = 0; i < _count; i++)
			{
				if(_data[i] != temp->_data[i]) return false;
			}

			return true;
		}

		Object *GetObjectAtIndex(size_t index) const
		{
			return _data[index];
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
		
		const Object **GetData() const
		{
			return const_cast<const Object **>(_data);
		}
		
		RNAPI void ShrinkToFit();
		
		template<class T=Object>
		void Sort(const std::function<bool (const T *left, const T *right)> &function)
		{
			if(_count <= 1) return;
			
			std::sort(_data, _data + _count, [&](const Object *left, const Object *right) -> bool {
				return function(static_cast<const T *>(left), static_cast<const T *>(right));
			});
		}

		RNAPI String *GetComponentsJoinedByString(const String *separator) const;

	private:
		RNAPI void UpdateSizeIfNeeded(size_t required);
		
		Object **_data;
		size_t _count;
		size_t _size;
		
		__RNDeclareMetaInternal(Array)
	};
	
	RNObjectClass(Array)
}

#endif /* __RAYNE_ARRAY_H__ */
