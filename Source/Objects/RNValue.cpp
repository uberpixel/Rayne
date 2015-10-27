//
//  RNValue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNValue.h"
#include "RNSerialization.h"
#include "RNString.h"

namespace RN
{
	RNDefineMeta(Value, Object)
	
	Value::Value(const Value *other) :
		_type(other->_type),
		_size(other->_size),
		_alignment(other->_alignment)
	{
		_storage = reinterpret_cast<uint8 *>(Memory::AllocateAligned(_size, _alignment));
		std::copy(other->_storage, other->_storage + _size, _storage);
	}
	
	Value::Value(Deserializer *deserializer)
	{
		_type = static_cast<char>(deserializer->DecodeInt32());
		
		uint8 *source = static_cast<uint8 *>(deserializer->DecodeBytes(&_size));
		
		_storage = reinterpret_cast<uint8 *>(Memory::AllocateAligned(_size, _alignment));
		std::copy(source, source + _size, _storage);
	}
	
	Value::~Value()
	{
		Memory::FreeAligned(_storage);
	}
	
	
	void Value::Serialize(Serializer *serializer) const
	{
		serializer->EncodeInt32(_type);
		serializer->EncodeBytes(_storage, _size);
	}

	const String *Value::GetDescription() const
	{
		switch(_type)
		{
			case TypeTranslator<Vector2>::value:
			{
				Vector2 value(GetValue<Vector2>());
				return RNSTRF("<Vector2(%f, %f)>", value.x, value.y);
			}
			case TypeTranslator<Vector3>::value:
			{
				Vector3 value(GetValue<Vector3>());
				return RNSTRF("<Vector3(%f, %f)>", value.x, value.y, value.z);
			}
			case TypeTranslator<Vector4>::value:
			{
				Vector4 value(GetValue<Vector4>());
				return RNSTRF("<Vector4(%f, %f, %f, %f)>", value.x, value.y, value.z, value.w);
			}

			case TypeTranslator<Color>::value:
			{
				Color value(GetValue<Color>());
				return RNSTRF("<Color(%f, %f, %f, %f)>", value.r, value.g, value.b, value.a);
			}

			case TypeTranslator<Matrix>::value:
			{
				Matrix value(GetValue<Matrix>());
				return RNSTRF("<Matrix(\n\t%f, %f, %f, %f\n\t%f, %f, %f, %f\n\t%f, %f, %f, %f\n\t%f, %f, %f, %f)>",
							 value.m[0], value.m[1], value.m[2], value.m[3],
							 value.m[4], value.m[5], value.m[6], value.m[7],
							 value.m[8], value.m[9], value.m[10], value.m[11],
							 value.m[12], value.m[13], value.m[14], value.m[15]);
			}
			case TypeTranslator<Quaternion>::value:
			{
				Quaternion value(GetValue<Quaternion>());
				return RNSTRF("<Quaternion(%f, %f, %f, %f)>", value.x, value.y, value.z, value.w);
			}

			default:
				break;
		}

		return RNSTRF("<Value: type: %c, size: %d, alignment: %d>", _type, (int)_size, (int)_alignment);
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
