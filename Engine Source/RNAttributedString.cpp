//
//  RNAttributedString.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAttributedString.h"
#include "RNWrappingObject.h"

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
	
	void AttributedString::RemoveAttribute(class String *key, const Range& range)
	{
		if(_attributes.size() > 0)
		{
			_attributes.find_overlapping(Range(_attributes.min(), (_attributes.max() - _attributes.min())), _queuedAttributes);
			_attributes = stl::interval_tree<Attribute>();
		}
		
		std::vector<stl::interval_tree<Attribute>::interval> additions;
		
		for(auto i = _queuedAttributes.begin(); i != _queuedAttributes.end();)
		{
			if(range.Overlaps(i->range))
			{
				if(i->value == key)
				{
					if(i->range.origin < range.origin)
					{
						Range temp;
						temp.origin = i->range.origin;
						temp.length = range.origin - temp.origin;
						
						additions.emplace_back(stl::interval_tree<Attribute>::interval(temp, i->value));
					}
					
					if(i->range.End() > range.End())
					{
						Range temp;
						temp.origin = range.End();
						temp.length = i->range.End() - temp.origin;
						
						additions.emplace_back(stl::interval_tree<Attribute>::interval(temp, i->value));
					}
					
					i = _queuedAttributes.erase(i);
					continue;
				}
			}
			
			i ++;
		}
		
		_queuedAttributes.insert(_queuedAttributes.end(), additions.begin(), additions.end());
		
		if(!_editing)
			ApplyUpdates();
	}
	
	void AttributedString::MergeAttributes()
	{
		typedef stl::interval_tree<Attribute>::interval Interval;
		typedef WrappingObject<std::deque<stl::interval_tree<Attribute>::interval>> Wrapper;
		
		Dictionary *temp = new Dictionary(_queuedAttributes.size());
		
		for(auto i = _queuedAttributes.begin(); i != _queuedAttributes.end(); i ++)
		{
			Wrapper *object = temp->ObjectForKey<Wrapper>(i->value.key);
			
			if(!object)
			{
				object = new Wrapper();
				temp->SetObjectForKey(object, i->value.key);
			}
			
			object->Data().push_back(std::move(*i));
		}
		
		_queuedAttributes.clear();
		
		temp->Enumerate([&](Object *value, Object *key, bool *stop) {
			Wrapper *object = static_cast<Wrapper *>(value);
			auto data = object->Data();
			
			std::sort(data.begin(), data.end(), [](const Interval& left, const Interval& right) {
				return (left.range.origin < right.range.origin);
			});
			
			while(!data.empty())
			{
				auto interval = data.at(0);
				
				for(auto i = data.begin() + 1; i != data.end();)
				{
					if(!i->value.value->IsEqual(interval.value.value))
						continue;
					
					bool subsequent = false;
					
					if(!subsequent)
						subsequent = (i->range.origin == interval.range.End() + 1);
					
					if(!subsequent)
						subsequent = (interval.range.origin == i->range.End() + 1);
					
					if(subsequent || interval.range.Overlaps(i->range))
					{
						size_t origin = std::min(i->range.origin, interval.range.origin);
						size_t length = std::max(i->range.End(), interval.range.End()) - origin;
						
						interval.range.origin = origin;
						interval.range.length = length;
						
						i = data.erase(i);
						continue;
					}
					
					break;
				}
				
				_queuedAttributes.push_back(std::move(data.at(0)));
				data.erase(data.begin());
			}
		});
		
		temp->Release();
	}
	
	void AttributedString::ApplyUpdates()
	{
		if(!_queuedAttributes.empty())
		{
			_attributes.find_overlapping(Range(_attributes.min(), (_attributes.max() - _attributes.min())), _queuedAttributes);
			MergeAttributes();
			
			_attributes = stl::interval_tree<Attribute>(std::move(_queuedAttributes));
			_queuedAttributes.clear();
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
