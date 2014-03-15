//
//  RNValue.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VALUE_H__
#define __RAYNE_VALUE_H__

#include "RNBase.h"
#include "RNObject.h"

#include "RNVector.h"
#include "RNColor.h"
#include "RNMatrixQuaternion.h"
#include "RNAny.h"

namespace RN
{
	class Value : public Object
	{
	public:
		template<class T>
		Value(const T &value) :
			_any(value)
		{}
		
		RNAPI Value(const Value *other);
		
		RNAPI static Value *WithVector2(const Vector2& vector);
		RNAPI static Value *WithVector3(const Vector3& vector);
		RNAPI static Value *WithVector4(const Vector4& vector);
		
		RNAPI static Value *WithColor(const Color& color);
		
		RNAPI static Value *WithQuaternion(const Quaternion& quaternion);
		RNAPI static Value *WithMatrix(const Matrix& matrix);
		
		template<class T>
		T GetValue() const
		{
			RN_ASSERT(typeid(T) == GetTypeInfo(), "");
			
			return stl::any_cast<T>(_any);
		}
		
		RNAPI const std::type_info &GetTypeInfo() const { return _any.type(); }
		
	private:
		stl::any _any;
		
		RNDeclareMeta(Value)
	};
}

#endif /* __RAYNE_VALUE_H__ */
