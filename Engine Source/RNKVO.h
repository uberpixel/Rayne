//
//  RNKVO.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVO_H__
#define __RAYNE_KVO_H__

#include "RNBase.h"
#include "RNSignal.h"

#define kRNObservableNewValueKey RNCSTR("kRNObservableNewValueKey")
#define kRNObservableOldValueKey RNCSTR("kRNObservableOldValueKey")

namespace RN
{
	class Object;
	class Dictionary;
	
	enum class ObservableType
	{
		Bool,
		
		Int8,
		Int16,
		Int32,
		Int64,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		
		Float,
		Double,
		
		Vector2,
		Vector3,
		Vector4,
		Color,
		Matrix,
		Quaternion,
		
		Object
	};
	
	class ObservableProperty
	{
	public:
		friend class Object;
		RNAPI virtual ~ObservableProperty();
	
		const std::string &GetName() const { return _name; }
		ObservableType GetType() const { return _type; }
		
		RNAPI virtual void SetValue(Object *value) = 0;
		RNAPI virtual Object *GetValue() const = 0;
		
		void SetWritable(bool writable) { _writable = writable; }
		bool IsWritable() const { return _writable; }
		
		RNAPI void WillChangeValue();
		RNAPI void DidChangeValue();
		
	protected:
		RNAPI ObservableProperty(const std::string &name, ObservableType type);
		
	private:
		std::string _name;
		ObservableType _type;
		Signal<void (Object *, const std::string &, Dictionary *)> _signal;
		
		Object *_object;
		Dictionary *_changeSet;
		
		bool _writable;
		char _recursion;
	};
}

#endif /* __RAYNE_KVO_H__ */
