//
//  RNKVO.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
	
	ObservableProperty::ObservableProperty(const std::string &name, ObservableType type) :
		_name(name),
		_type(type),
		_changeSet(nullptr),
		_object(nullptr),
		_recursion(0),
		_writable(true)
	{}
	
	ObservableProperty::~ObservableProperty()
	{
		RN_ASSERT(!_changeSet, "ChangeSet must be null upon destructor call. Smells like WillChange/DidChange imbalance");
	}
	
	void ObservableProperty::WillChangeValue()
	{
		RN_ASSERT(_object, "Observable<> must be added to an Object before they can be used!");
		
		if((_recursion ++) == 0)
		{
			Object *value = GetValue();
			
			_changeSet = new Dictionary();
			_changeSet->SetObjectForKey(value ? value : Null::GetNull(), kRNObservableOldValueKey);
		}
	}
	
	void ObservableProperty::DidChangeValue()
	{
		RN_ASSERT(_object, "Observable<> must be added to an Object before they can be used!");
		
		if((-- _recursion) == 0)
		{
			Object *value = GetValue();
			_changeSet->SetObjectForKey(value ? value : Null::GetNull(), kRNObservableNewValueKey);
			
			_signal.Emit(std::move(_object), _name, std::move(_changeSet));
			
			_changeSet->Release();
			_changeSet = nullptr;
		}
	}
}
