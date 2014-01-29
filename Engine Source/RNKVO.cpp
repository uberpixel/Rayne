//
//  RNKVO.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKVO.h"
#include "RNKVOImplementation.h"
#include "RNDictionary.h"
#include "RNString.h"
#include "RNNull.h"

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: ObservableBase
	// ---------------------
	
	ObservableProperty::ObservableProperty(const char *name, char type) :
		_type(type),
		_changeSet(nullptr),
		_object(nullptr),
		_signal(nullptr),
		_flags(1 << 4)
	{
		strncpy(_name, name, 32);
		_name[32] = '\0';
	}
	
	ObservableProperty::~ObservableProperty()
	{
		RN_ASSERT(!_changeSet, "ChangeSet must be null upon destructor call. Smells like WillChange/DidChange imbalance");
		delete _signal;
	}
	
	void ObservableProperty::AssertSignal()
	{
		if(!_signal)
			_signal = new Signal<void (Object *, const std::string &, Dictionary *)>();
	}
	
	
	void ObservableProperty::SetWritable(bool writable)
	{
		writable ? (_flags |= (1 << 4)) : (_flags &= ~(1 << 4));
	}
	
	void ObservableProperty::WillChangeValue()
	{
		RN_ASSERT(_object, "Observable<> must be added to an Object before they can be used!");
		
		if(((_flags ++) & 0xf) == 0 && _signal)
		{
			if(_signal->GetCount() > 0)
			{
				Object *value = GetValue();
				
				_changeSet = new Dictionary();
				_changeSet->SetObjectForKey(value ? value : Null::GetNull(), kRNObservableOldValueKey);
			}
		}
	}
	
	void ObservableProperty::DidChangeValue()
	{
		RN_ASSERT(_object, "Observable<> must be added to an Object before they can be used!");
		
		if(((-- _flags) & 0xf) == 0)
		{
			if(_changeSet)
			{
				Object *value = GetValue();
				_changeSet->SetObjectForKey(value ? value : Null::GetNull(), kRNObservableNewValueKey);
				
				_signal->Emit(std::move(_object), _name, std::move(_changeSet));
				
				_changeSet->Release();
				_changeSet = nullptr;
			}
		}
	}
}
