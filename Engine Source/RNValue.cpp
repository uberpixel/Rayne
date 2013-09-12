//
//  RNValue.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNValue.h"

namespace RN
{
	RNDeclareMeta(Value)
	
	Value::Value(const void *ptr, size_t size, const std::type_info& typeinfo)
	{
		_typeinfo = &typeinfo;
		_size     = size;
		_buffer   = new uint8[size];
		
		uint8 *temp = const_cast<uint8 *>(reinterpret_cast<const uint8 *>(ptr));
		std::copy(temp, temp + size, _buffer);
	}
	
	Value::Value(const void *ptr) :
		Value(&ptr, sizeof(const void *), typeid(const void *))
	{}
	
	Value::~Value()
	{
		delete [] _buffer;
	}
	
	
	Value *Value::WithVector2(const Vector2& vector)
	{
		Value *value = new Value(&vector, sizeof(Vector2), typeid(Vector2));
		return value->Autorelease();
	}
	
	Value *Value::WithVector3(const Vector3& vector)
	{
		Value *value = new Value(&vector, sizeof(Vector3), typeid(Vector3));
		return value->Autorelease();
	}
	
	Value *Value::WithVector4(const Vector4& vector)
	{
		Value *value = new Value(&vector, sizeof(Vector4), typeid(Vector4));
		return value->Autorelease();
	}
	
	Value *Value::WithColor(const Color& color)
	{
		Value *value = new Value(&color, sizeof(Color), typeid(Color));
		return value->Autorelease();
	}
	
	Value *Value::WithQuaternion(const Quaternion& quaternion)
	{
		Value *value = new Value(&quaternion, sizeof(Quaternion), typeid(Quaternion));
		return value->Autorelease();
	}
	
	Value *Value::WithMatrix(const Matrix& matrix)
	{
		Value *value = new Value(&matrix, sizeof(Matrix), typeid(Matrix));
		return value->Autorelease();
	}
	
	
	void Value::GetValue(void *ptr) const
	{
		uint8 *temp = reinterpret_cast<uint8 *>(ptr);
		std::copy(_buffer, _buffer + _size, temp);
	}
	
	void *Value::GetPointerValue() const
	{
		void *ptr;
		GetValue(&ptr);
		
		return ptr;
	}
}
