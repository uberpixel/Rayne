//
//  RNNull.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		RNAPI Null();
		RNAPI Null(Deserializer *deserializer);
		RNAPI ~Null() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI static Null *GetNull();
		
		RNDeclareMeta(Null)
	};
	
	RNObjectClass(Null)
}

#endif /* __RAYNE_NULL_H__ */
