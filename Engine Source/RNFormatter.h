//
//  RNFormatter.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FORMATTER_H__
#define __RAYNE_FORMATTER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNString.h"
#include "RNAttributedString.h"

namespace RN
{
	class Formatter : public Object
	{
	public:
		virtual String *GetStringForObject(Object *object) = 0;
		virtual Object *GetObjectForString(String *string) = 0;
		virtual AttributedString *GetAttributedStringForObject(Object *object, Dictionary *defaultAttributes);
		
		RNDefineMeta(Formatter, Object)
	};
	
	class NumberFormatter : public Formatter
	{
	public:
		String *GetStringForObject(Object *object) override;
		Object *GetObjectForString(String *string) override;
	};
}

#endif /* __RAYNE_FORMATTER_H__ */
