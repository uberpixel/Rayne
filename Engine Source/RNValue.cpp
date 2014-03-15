//
//  RNValue.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNValue.h"

namespace RN
{
	RNDefineMeta(Value, Object)
	
	Value::Value(const Value *other) :
		_any(other->_any)
	{}
	
	
	Value *Value::WithVector2(const Vector2& vector)
	{
		Value *value = new Value(vector);
		return value->Autorelease();
	}
	
	Value *Value::WithVector3(const Vector3& vector)
	{
		Value *value = new Value(vector);
		return value->Autorelease();
	}
	
	Value *Value::WithVector4(const Vector4& vector)
	{
		Value *value = new Value(vector);
		return value->Autorelease();
	}
	
	Value *Value::WithColor(const Color &color)
	{
		Value *value = new Value(color);
		return value->Autorelease();
	}
	
	Value *Value::WithQuaternion(const Quaternion& quaternion)
	{
		Value *value = new Value(quaternion);
		return value->Autorelease();
	}
	
	Value *Value::WithMatrix(const Matrix& matrix)
	{
		Value *value = new Value(matrix);
		return value->Autorelease();
	}
}
