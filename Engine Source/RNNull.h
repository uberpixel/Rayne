//
//  RNNull.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NULL_H__
#define __RAYNE_NULL_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Null : public Object
	{
	public:
		Null();
		~Null() override;
		
		static Null *GetNull();
		
		RNDefineMeta(Null, Object)
	};
}

#endif /* __RAYNE_NULL_H__ */
