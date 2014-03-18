//
//  RNKVO.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVO_H__
#define __RAYNE_KVO_H__

#include "RNBase.h"
#include "RNSignal.h"
#include "RNMatrixQuaternion.h"
#include "RNVector.h"
#include "RNColor.h"
#include "RNTypeTranslator.h"

#define kRNObservableNewValueKey RNCSTR("kRNObservableNewValueKey")
#define kRNObservableOldValueKey RNCSTR("kRNObservableOldValueKey")

namespace RN
{
	class Object;
	class Dictionary;
	class MetaClassBase;
	
	class ObservableProperty
	{
	public:
		friend class Object;
		
		RNAPI virtual ~ObservableProperty();
	
		std::string GetName() const { return _name; }
		char GetType() const { return _type; }
		
		RNAPI virtual void SetValue(Object *value) = 0;
		RNAPI virtual Object *GetValue() const = 0;
		RNAPI Object *GetObject() { return _object; }
		
		RNAPI void SetWritable(bool writable);
		bool IsWritable() const { return _flags & (1 << 8); }
		
		RNAPI virtual MetaClassBase *GetMetaClass() const { return nullptr; }
		
		RNAPI void WillChangeValue();
		RNAPI void DidChangeValue();
		
	protected:
		RNAPI ObservableProperty(const char *name, char type);
		
		Object *_object;
		
	private:
		RNAPI void AssertSignal();
		
		char _type;
		char _name[33];
		uint8 _flags;
		
		Signal<void (Object *, const std::string &, Dictionary *)> *_signal;
		Dictionary *_changeSet;
		void *_opaque;
	};
}

#endif /* __RAYNE_KVO_H__ */
