//
//  RNAttributedString.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ATTRIBUTEDSTRING_H__
#define __RAYNE_ATTRIBUTEDSTRING_H__

#include "RNBase.h"
#include "RNString.h"
#include "RNDictionary.h"
#include "RNIntervalTree.h"

namespace RN
{
	class AttributedString : public Object
	{
	public:
		RNAPI AttributedString(String *string);
		RNAPI ~AttributedString() override;
		
		RNAPI void BeginEditing();
		RNAPI void EndEditing();
		
		RNAPI void AddAttribute(String *key, Object *value, const Range& range);
		RNAPI void AddAttributes(Dictionary *attributes, const Range& range);
		RNAPI void RemoveAttribute(String *key, const Range& range);
		RNAPI Dictionary *AttributesAtIndex(size_t index);
		
		size_t Length() const { return _string->Length(); }
		String *String() const { return _string; }
		
	private:
		class Attribute
		{
		public:
			Attribute(class String *tkey, Object *tvalue)
			{
				key = tkey->Retain();
				value = tvalue->Retain();
			}
			
			Attribute(const Attribute& other)
			{
				key = other.key->Retain();
				value = other.value->Retain();
			}
			
			~Attribute()
			{
				key->Release();
				value->Release();
			}
			
			bool operator== (class String *string)
			{
				return key->IsEqual(string);
			}
			
			class String *key;
			Object *value;
		};
		
		void ApplyUpdates();
		
		class String *_string;
		stl::interval_tree<Attribute> _attributes;
		
		bool _editing;
		std::vector<stl::interval_tree<Attribute>::interval> _queuedAttributes;
		
		RNDefineMeta(AttributedString, Object)
	};
}

#endif /* __RAYNE_ATTRIBUTEDSTRING_H__ */
