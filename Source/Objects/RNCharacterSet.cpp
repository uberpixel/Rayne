//
//  RNCharacterSet.cpp
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
			AddCharacter(string->GetCharacterAtIndex(i));
	}
	CharacterSet::CharacterSet(const char *string) :
		CharacterSet()
	{
		size_t length = strlen(string);

		for(size_t i = 0; i < length; i ++)
			AddCharacter(string[i]);
	}

	bool CharacterSet::CharacterIsMember(UniChar character) const
	{
		uint32 plane = (character >> 16);

		if(RN_EXPECT_FALSE(plane != 0))
			throw InconsistencyException("CharacterSet only supports plane 0 characters");

		uint16 n = static_cast<uint16>(character);
		return (_bitmap[n >> 3] & (static_cast<uint32>(1) << (n  & 7)));
	}

	void CharacterSet::AddCharacter(UniChar character)
	{
		uint32 plane = (character >> 16);

		if(RN_EXPECT_FALSE(plane != 0))
			throw InconsistencyException("CharacterSet only supports plane 0 characters");

		uint16 n = static_cast<uint16>(character);
		_bitmap[(n) >> 3] |= (static_cast<uint32>(1) << (n & 7));
	}
}
