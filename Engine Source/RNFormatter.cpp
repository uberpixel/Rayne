//
//  RNFormatter.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFormatter.h"
#include "RNNumber.h"

namespace RN
{
	RNDefineMeta(Formatter)
	
	AttributedString *Formatter::GetAttributedStringForObject(Object *object, Dictionary *defaultAttributes)
	{
		AttributedString *string = new AttributedString(GetStringForObject(object));
		string->SetAttributes(defaultAttributes, Range(0, string->GetLength()));
		
		return string->Autorelease();
	}
	
	
	
	String *NumberFormatter::GetStringForObject(Object *object)
	{
		Number *number = object->Downcast<Number>();
		switch(number->GetType())
		{
			case Number::Type::Int8:
			case Number::Type::Int16:
			case Number::Type::Int32:
			case Number::Type::Int64:
			{
				int64 value = number->GetInt64Value();
				return RNSTR("%lli", value);
			}
				
			case Number::Type::Uint8:
			case Number::Type::Uint16:
			case Number::Type::Uint32:
			case Number::Type::Uint64:
			{
				uint64 value = number->GetUint64Value();
				return RNSTR("%llu", value);
			}
				
			case Number::Type::Float32:
			case Number::Type::Float64:
			{
				double value = number->GetDoubleValue();
				return RNSTR("%f", value);
			}
				
			case Number::Type::Boolean:
			{
				bool value = number->GetBoolValue();
				return RNSTR("%s", value ? "true" : "false");
			}
		}

		return RNCSTR("??");
	}
	
	Object *NumberFormatter::GetObjectForString(String *string)
	{
		if(string->IsEqual(RNCSTR("true")))
			return Number::WithBool(true);
		
		if(string->IsEqual(RNCSTR("false")))
			return Number::WithBool(false);
		
		Range range = string->GetRangeOfString(RNCSTR("."));
		bool isFloat = (range.origin != k::NotFound);
		
		if(isFloat)
		{
			double value = atof(string->GetUTF8String());
			return Number::WithDouble(value);
		}
		else
		{
			const char *utf8String = string->GetUTF8String();
			
			int32 value32 = atoi(utf8String);
			int64 value64 = atoll(utf8String);
			
			if(value64 == value32)
				return Number::WithInt32(value32);
			
			return Number::WithInt64(value64);
		}
	}
}
