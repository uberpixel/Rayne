//
//  RNValue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNValue.h"
#include "RNSerialization.h"

namespace RN
{
	RNDefineMeta(Value, Object)
	
	Value::Value(const Value *other) :
		_type(other->_type),
		_size(other->_size)
	{
		_storage = new uint8[_size];
		std::copy(other->_storage, other->_storage + _size, _storage);
	}
	
	Value::Value(Deserializer *deserializer)
	{
		_type = static_cast<char>(deserializer->DecodeInt32());
		
		uint8 *source = static_cast<uint8 *>(deserializer->DecodeBytes(&_size));
		
		_storage = new uint8[_size];
		std::copy(source, source + _size, _storage);
	}
	
	Value::~Value()
	{
		delete [] _storage;
	}
	
	
	void Value::Serialize(Serializer *serializer)
	{
		serializer->EncodeInt32(_type);
		serializer->EncodeBytes(_storage, _size);
	}
	
	
	Value *Value::WithVector2(const Vector2 &vector)
	{
		Value *value = new Value(vector);
		return value->Autorelease();
	}
	
	Value *Value::WithVector3(const Vector3 &vector)
	{
		Value *value = new Value(vector);
		return value->Autorelease();
	}
	
	Value *Value::WithVector4(const Vector4 &vector)
	{
		Value *value = new Value(vector);
		return value->Autorelease();
	}
	
	Value *Value::WithColor(const Color &color)
	{
		Value *value = new Value(color);
		return value->Autorelease();
	}
	
	Value *Value::WithQuaternion(const Quaternion &quaternion)
	{
		Value *value = new Value(quaternion);
		return value->Autorelease();
	}
	
	Value *Value::WithMatrix(const Matrix &matrix)
	{
		Value *value = new Value(matrix);
		return value->Autorelease();
	}
	
	
	size_t Value::GetHash() const
	{
		size_t hash = _size ^ _type;
		
		for(size_t i = 0; i < _size - 1; i ++)
			hash ^= _storage[i] | (static_cast<uint32>(_storage[i + 1]) << 16);
		
		return hash;
	}
	
	bool Value::IsEqual(const Object *tother) const
	{
		const Value *other = tother->Downcast<Value>();
		if(!other)
			return false;
		
		if(_type == other->_type && _size == other->_size)
			return (memcpy(_storage, other->_storage, _size) == 0);
		
		return false;
	}
}
