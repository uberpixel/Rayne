//
//  RNSerialization.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSerialization.h"

namespace RN
{
	void Serializer::EncodeBytes(void *data, size_t size)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeObject(Archivable *object)
	{
		throw ErrorException(0);
	}
	
	void Serializer::EncodeBool(bool value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeDouble(double value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeFloat(float value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeInt32(int32 value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeInt64(int64 value)
	{
		throw ErrorException(0);
	}
	
	void Serializer::EncodeVector2(const Vector2& value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeVector3(const Vector3& value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeVector4(const Vector4& value)
	{
		throw ErrorException(0);
	}
	
	void Serializer::EncodeMatrix(const Matrix& value)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeQuarternion(const Quaternion& value)
	{
		throw ErrorException(0);
	}
	
	void *Serializer::DecodeBytes(size_t *length)
	{
		throw ErrorException(0);
	}
	Archivable *Serializer::DecodeObject()
	{
		throw ErrorException(0);
	}
	
	bool Serializer::DecodeBool()
	{
		throw ErrorException(0);
	}
	double Serializer::DecodeDouble()
	{
		throw ErrorException(0);
	}
	float Serializer::DecodeFloat()
	{
		throw ErrorException(0);
	}
	int32 Serializer::DecodeInt32()
	{
		throw ErrorException(0);
	}
	int64 Serializer::DecodeInt64()
	{
		throw ErrorException(0);
	}
	
	Vector2 Serializer::DecodeVector2()
	{
		throw ErrorException(0);
	}
	Vector3 Serializer::DecodeVector3()
	{
		throw ErrorException(0);
	}
	Vector4 Serializer::DecodeVector4()
	{
		throw ErrorException(0);
	}
	
	Matrix Serializer::DecodeMatrix()
	{
		throw ErrorException(0);
	}
	Quaternion Serializer::DecodeQuaternion()
	{
		throw ErrorException(0);
	}
	
	uint32 Serializer::AppVersion() const
	{
		throw ErrorException(0);
	}
}
