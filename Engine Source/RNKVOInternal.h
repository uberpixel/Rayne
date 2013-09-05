//
//  RNKVOInternal.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVOINTERNALH__
#define __RAYNE_KVOINTERNALH__

#include "RNBase.h"
#include "RNKVO.h"
#include "RNObject.h"
#include "RNNumber.h"

namespace RN
{
	class ObservableInt : public ObservableVariable<int32>
	{
	public:
		ObservableInt(int32 *ptr, const char *name) :
			ObservableVariable(ptr, name, ObservableType::Int32)
		{}
		
		void SetValue(Object *value) override
		{
			RN_ASSERT(value->IsKindOfClass(Number::MetaClass()), "");
			Number *number = static_cast<Number *>(value);
		
			WillChangeValue();
			*_value = number->GetInt32Value();
			DiDchangeValue();
		}
	
		Object *GetValue() const override
		{
			return Number::WithInt32(*_value);
		}
	};
}

#endif /* __RAYNE_KVOINTERNALH__ */
