//
//  RNUIAttributedString.cpp
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNUIAttributedString.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(AttributedString, String)
	
		void TextAttributes::SetFont(Font *font)
		{
			SafeRelease(_font);
			_font = SafeRetain(font);
		}

		void TextAttributes::SetFontSize(float fontSize)
		{
			_fontSize = fontSize;
		}

		void TextAttributes::SetColor(const Color &color)
		{
			_color = color;
		}

		void TextAttributes::SetAlignment(TextAlignment alignment)
		{
			_alignment = alignment;
		}

		void TextAttributes::SetWrapMode(TextWrapMode wrapMode)
		{
			_wrapMode = wrapMode;
		}
	

		AttributedString::AttributedString(const String *string) : String(string)
		{
			
		}
	
		AttributedString::AttributedString(const AttributedString *string) : String(string), _attributes(string->_attributes)
		{
			
		}
	
		AttributedString::~AttributedString()
		{
			
		}
	
		void AttributedString::SetAttributes(const TextAttributes &attributes, const RN::Range &range)
		{
			size_t newStart = range.origin;
			size_t newEnd = range.origin + range.length;
			
			for(int64 i = _attributes.size()-1; i >= 0; i--)
			{
				size_t oldStart = _attributes[i]._range.origin;
				size_t oldEnd = _attributes[i]._range.origin + _attributes[i]._range.length;
				
				//No overlap
				if(oldStart > newEnd)
				{
					continue;
				}
				
				//Reached point to insert new attributes
				if(oldEnd < newStart)
				{
					_attributes.insert(_attributes.begin() + i + 1, attributes);
					_attributes[i+1]._range = range;
					return;
				}
				
				//new range contains old range
				if(oldStart >= newStart && oldEnd <= newEnd)
				{
					_attributes.erase(_attributes.begin() + i);
				}
				
				//New range is inside old range
				if(oldStart < newStart && oldEnd > newEnd)
				{
					_attributes.insert(_attributes.begin() + i + 1, attributes);
					_attributes.insert(_attributes.begin() + i + 2, _attributes[i]);
					_attributes[i]._range.origin = oldStart;
					_attributes[i]._range.length = newStart - oldStart;
					
					_attributes[i+1]._range = range;
					
					_attributes[i+2]._range.origin = newEnd;
					_attributes[i+2]._range.length = oldEnd - newEnd;
					return;
				}
				
				//New range overlaps end of old range
				if(newStart <= oldEnd && oldStart < newStart)
				{
					_attributes[i]._range.origin = oldStart;
					_attributes[i]._range.length = newStart - oldStart;
					
					_attributes.insert(_attributes.begin() + i + 1, attributes);
					_attributes[i+1]._range = range;
					
					return;
				}
				
				//New range overlaps beginning of old range
				if(oldStart <= newEnd && newStart < oldStart)
				{
					_attributes.insert(_attributes.begin() + i, attributes);
					_attributes[i]._range = range;
					
					_attributes[i+1]._range.origin = newEnd;
					_attributes[i+1]._range.length = oldEnd - newEnd;
					
					return;
				}
			}
			
			_attributes.insert(_attributes.begin(), attributes);
			_attributes[0]._range = range;
		}
	
		const TextAttributes *AttributedString::GetAttributesAtIndex(size_t index) const
		{
			for(const TextAttributes &attributes : _attributes)
			{
				if(attributes._range.Contains(RN::Range(index, 1)))
				{
					return &attributes;
				}
			}
			
			return nullptr;
		}
	}
}
