//
//  RNKVO.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVO_H__
#define __RAYNE_KVO_H__

#include "../Base/RNBase.h"
#include "../Base/RNSignal.h"
#include "../Base/RNTypeTranslator.h"
#include "../Math/RNMatrixQuaternion.h"
#include "../Math/RNVector.h"
#include "../Math/RNColor.h"

#define kRNObservableNewValueKey RNCSTR("kRNObservableNewValueKey")
#define kRNObservableOldValueKey RNCSTR("kRNObservableOldValueKey")

#if RN_PLATFORM_WINDOWS
#pragma push_macro("GetObject")
#undef GetObject
#endif

namespace RN
{
	class Object;
	class Dictionary;
	class MetaClass;
	
	class ObservableProperty
	{
	public:
		friend class Object;
		
		RNAPI virtual ~ObservableProperty();
	
		std::string GetName() const { return _name; }
		char GetType() const { return _type; }
		
		RNAPI virtual void SetValue(Object *value) = 0;
		RNAPI virtual Object *GetValue() const = 0;
		RNAPI Object *GetObject() const { return _object; }

#if RN_PLATFORM_WINDOWS
		Object *GetObjectA() const { return _object; }
		Object *GetObjectW() const { return _object; }
#endif
		
		RNAPI void SetWritable(bool writable);
		bool IsWritable() const { return _flags & (1 << 8); }
		
		RNAPI virtual MetaClass *GetMetaClass() const { return nullptr; }
		
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

#if RN_PLATFORM_WINDOWS
#pragma pop_macro("GetObject")
#endif

#endif /* __RAYNE_KVO_H__ */
