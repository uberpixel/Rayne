//
//  RNCharacterSet.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CHARACTERSET_H__
#define __RAYNE_CHARACTERSET_H__

#include "../Base/RNBase.h"
#include "../Base/RNUnicode.h"
#include "RNObject.h"

namespace RN
{
	class String;
	class CharacterSet : public Object
	{
	public:
		RNAPI CharacterSet();
		RNAPI CharacterSet(const String *string);
		RNAPI CharacterSet(const char *string);

		RNAPI bool CharacterIsMember(UniChar character) const;

	private:
		void AddCharacter(UniChar character);

		uint8 _bitmap[8192];
	};
}

#endif /* __RAYNE_CHARACTERSET_H__ */
