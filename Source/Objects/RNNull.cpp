//
//  RNNull.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNull.h"

namespace RN
{
	RNDefineMeta(Null, Object)
	
	Null::Null()
	{}
	
	Null::~Null()
	{}
	
	
	Null::Null(Deserializer *deserializer)
	{}
	void Null::Serialize(Serializer *serializer) const
	{}
	
	
	Null *Null::GetNull()
	{
		static std::once_flag flag;
		static Null *null = nullptr;
		
		std::call_once(flag, [&]() {
			null = new Null();
		});
		
		return null;
	}
}
