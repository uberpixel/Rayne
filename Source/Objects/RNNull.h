//
//  RNNull.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NULL_H__
#define __RAYNE_NULL_H__

#include "../Base/RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Null : public Object
	{
	public:
		RNAPI Null();
		RNAPI Null(Deserializer *deserializer);
		RNAPI ~Null() override;
		
		RNAPI void Serialize(Serializer *serializer) const override;
		
		RNAPI static Null *GetNull();
		
		__RNDeclareMetaInternal(Null)
	};
	
	RNObjectClass(Null)
}

#endif /* __RAYNE_NULL_H__ */
