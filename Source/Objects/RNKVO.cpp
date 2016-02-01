//
//  RNKVO.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
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
		_owner(nullptr),
		_type(type),
		_flags(1 << 4),
		_signal(nullptr),
		_changeSet(nullptr)
	{
		RN_ASSERT(type != '?', "ObservableProperty with invalid type!");
		
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
			_signal = new Signal<void (Object *, const char *, const Dictionary *)>();
	}
	
	
	void ObservableProperty::SetWritable(bool writable)
	{
		writable ? (_flags |= (1 << 4)) : (_flags &= ~(1 << 4));
	}
	
	void ObservableProperty::WillChangeValue()
	{
		RN_ASSERT(_owner, "Observable<> must be added to an Object before they can be used!");

		uint8 recursion = (_flags & 0xf);

		if((recursion ++) == 0 && _signal)
		{
			if(_signal->GetCount() > 0)
			{
				Object *value = GetValue();
				
				_changeSet = new Dictionary();
				_changeSet->SetObjectForKey(value ? value : Null::GetNull(), kRNObservableOldValueKey);
			}
		}

		_flags &= ~0xf;
		_flags |= recursion;
	}
	
	void ObservableProperty::DidChangeValue()
	{
		RN_ASSERT(_owner, "Observable<> must be added to an Object before they can be used!");

		uint8 recursion = (_flags & 0xf);
		
		if((-- recursion) == 0)
		{
			if(_changeSet)
			{
				// Write the recursion counter back immediately
				_flags &= ~0xf;
				_flags |= recursion;

				Object *value = GetValue();
				_changeSet->SetObjectForKey(value ? value : Null::GetNull(), kRNObservableNewValueKey);
				
				_signal->Emit(std::move(_owner), _name, std::move(_changeSet));
				
				_changeSet->Release();
				_changeSet = nullptr;

				return;
			}
		}

		_flags &= ~0xf;
		_flags |= recursion;
	}
}
