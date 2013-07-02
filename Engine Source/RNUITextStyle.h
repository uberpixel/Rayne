//
//  RNUITextStyle.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UITEXTSTYLE_H__
#define __RAYNE_UITEXTSTYLE_H__

#include "RNBase.h"
#include "RNVector.h"

namespace RN
{
	namespace UI
	{
		enum class LineBreakMode
		{
			WordWrapping,
			CharacterWrapping,
			TruncateHead,
			TruncateTail,
			TruncateMiddle
		};
		
		enum class TextAlignment
		{
			Left,
			Right,
			Center
		};
		
		enum class TextTruncation
		{
			Start,
			End,
			Middle
		};
	}
}

#endif /* __RAYNE_UITEXTSTYLE_H__ */
