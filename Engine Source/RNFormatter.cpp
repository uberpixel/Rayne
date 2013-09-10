//
//  RNFormatter.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFormatter.h"

namespace RN
{
	RNDeclareMeta(Formatter)
	
	AttributedString *Formatter::GetAttributedStringForObject(Object *object, Dictionary *defaultAttributes)
	{
		AttributedString *string = new AttributedString(GetStringForObject(object));
		string->SetAttributes(defaultAttributes, Range(0, string->GetLength()));
		
		return string->Autorelease();
	}
}
