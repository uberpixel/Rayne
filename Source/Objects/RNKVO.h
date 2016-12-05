//
//  RNKVO.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
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

#define __kRNObservableFlagWritable 4 // Internal, do not use!

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
	
		const char *GetName() const { return _name; }
		char GetType() const { return _type; }
		
		RNAPI virtual void SetValue(Object *value) = 0;
		RNAPI virtual Object *GetValue() const = 0;
		Object *GetOwner() const { return _owner; }

		RNAPI void SetWritable(bool writable);
		bool IsWritable() const { return (_flags & (1 << __kRNObservableFlagWritable)); }
		
		virtual MetaClass *GetMetaClass() const { return nullptr; }
		
		RNAPI void WillChangeValue();
		RNAPI void DidChangeValue();
		
	protected:
		RNAPI ObservableProperty(const char *name, char type);
		
		Object *_owner;
		
	private:
		RNAPI void AssertSignal();
		
		char _type;
		char _name[33];
		uint8 _flags; // Lower 3 bits used as recursion Will/DidChange counter
		
		Signal<void (Object *, const char *, const Dictionary *)> *_signal;
		Dictionary *_changeSet;
		void *_opaque;
	};
}

#endif /* __RAYNE_KVO_H__ */
