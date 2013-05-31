//
//  RNWrappingObject.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WRAPPINGOBJECT_H__
#define __RAYNE_WRAPPINGOBJECT_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	template<class T>
	class WrappingObject : public Object
	{
	public:
		typedef std::function<void (WrappingObject<T> *)> DeallocCallback;
		
		WrappingObject(const T& data) :
			_data(data)
		{}
		
		WrappingObject(T&& data) :
			_data(std::move(data))
		{}
		
		~WrappingObject() override
		{
			if(_dealloc)
				_dealloc(this);
		}
		
		T& Data() const { return _data; }
		
		void SetDealloc(DeallocCallback callback)
		{
			_dealloc = std::move(callback);
		}
		
	private:
		T _data;
		DeallocCallback _dealloc;
	};
}

#endif /* __RAYNE_WRAPPINGOBJECT_H__ */
