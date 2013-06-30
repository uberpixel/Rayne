//
//  RNAttributedString.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAttributedString.h"

namespace RN
{
	RNDeclareMeta(AttributedString)
	
	AttributedString::AttributedString(class String *string)
	{
		_string = string->Retain();
		_editing = false;
	}
	AttributedString::~AttributedString()
	{
		_string->Release();
	}
	
	
	void AttributedString::BeginEditing()
	{
		_editing = true;
	}
	
	void AttributedString::EndEditing()
	{
		_editing = false;
		ApplyUpdates();
	}
	
	void AttributedString::AddAttribute(class String *key, Object *value, const Range& range)
	{
		_queuedAttributes.emplace_back(stl::interval_tree<Attribute>::interval(range, Attribute(key, value)));
		
		if(!_editing)
			ApplyUpdates();
	}
	
	void AttributedString::AddAttributes(Dictionary *attributes, const Range& range)
	{
		attributes->Enumerate([&](Object *value, Object *key, bool *stop) {
			if(key->IsKindOfClass(RN::String::MetaClass()))
			{
				class String *sKey = static_cast<class String *>(key);
				_queuedAttributes.emplace_back(stl::interval_tree<Attribute>::interval(range, Attribute(sKey, value)));
			}
		});
		
		if(!_editing)
			ApplyUpdates();
	}
	
	void AttributedString::ApplyUpdates()
	{
		if(!_queuedAttributes.empty())
		{
			_attributes.find_overlapping(Range(_attributes.min(), (_attributes.max() - _attributes.min())), _queuedAttributes);
			_attributes = stl::interval_tree<Attribute>(std::move(_queuedAttributes));
			
			_queuedAttributes = std::vector<stl::interval_tree<Attribute>::interval>();
		}
	}
	
	Dictionary *AttributedString::AttributesAtIndex(size_t index)
	{
		std::vector<stl::interval_tree<Attribute>::interval> overlapping;
		_attributes.find_overlapping(Range(index, 1), overlapping);
		
		if(overlapping.empty())
			return (new Dictionary())->Autorelease();
		
		Dictionary *attributes = new Dictionary(overlapping.size());
		for(auto i=overlapping.begin(); i!=overlapping.end(); i++)
		{
			attributes->SetObjectForKey(i->value.value, i->value.key);
		}
		
		return attributes->Autorelease();
	}
}
