//
//  RNCharacterSet.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCharacterSet.h"
#include "RNString.h"

namespace RN
{
	CharacterSet::CharacterSet()
	{
		std::memset(_bitmap, 0, 8192);
	}
	CharacterSet::CharacterSet(const String *string) :
		CharacterSet()
	{
		size_t length = string->GetLength();
		for(size_t i = 0; i < length; i ++)
		{
			uint16 character = static_cast<uint16>(string->GetCharacterAtIndex(i));
			_bitmap[(character) >> 3] |= (static_cast<uint32>(1) << (character & 7));
		}
	}
	CharacterSet::CharacterSet(const char *string) :
		CharacterSet()
	{
		size_t length = strlen(string);
		for(size_t i = 0; i < length; i ++)
		{
			uint16 character = static_cast<uint16>(string[i]);
			_bitmap[(character) >> 3] |= (static_cast<uint32>(1) << (character & 7));
		}
	}

	bool CharacterSet::CharacterIsMember(UniChar character) const
	{
		uint16 n = static_cast<uint16>(character);
		return (_bitmap[n >> 3] & (static_cast<uint32>(1) << (n  & 7)));
	}
}
