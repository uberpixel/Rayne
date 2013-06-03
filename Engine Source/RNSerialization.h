//
//  RNSerialization.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SERIALIZATION_H__
#define __RAYNE_SERIALIZATION_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNData.h"
#include "RNVector.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"

namespace RN
{
	class Serializer;
	
	class Archivable : public Object
	{
	public:
		virtual void Deserialize(Serializer *deserializer);
		virtual void Serialize(Serializer *serializer) const;
	};
	
	
	class Serializer
	{
	public:
		virtual void EncodeBytes(void *data, size_t size);
		virtual void EncodeObject(Archivable *object);
		
		virtual void EncodeBool(bool value);
		virtual void EncodeDouble(double value);
		virtual void EncodeFloat(float value);
		virtual void EncodeInt32(int32 value);
		virtual void EncodeInt64(int64 value);
		
		virtual void EncodeVector2(const Vector2& value);
		virtual void EncodeVector3(const Vector3& value);
		virtual void EncodeVector4(const Vector4& value);
		
		virtual void EncodeMatrix(const Matrix& value);
		virtual void EncodeQuarternion(const Quaternion& value);
		
		virtual void *DecodeBytes(size_t *length);
		virtual Archivable *DecodeObject();
		
		virtual bool DecodeBool();
		virtual double DecodeDouble();
		virtual float DecodeFloat();
		virtual int32 DecodeInt32();
		virtual int64 DecodeInt64();
		
		virtual Vector2 DecodeVector2();
		virtual Vector3 DecodeVector3();
		virtual Vector4 DecodeVector4();
		
		virtual Matrix DecodeMatrix();
		virtual Quaternion DecodeQuaternion();
		
		virtual uint32 AppVersion() const;
	};
}

#endif /* __RAYNE_SERIALIZATION_H__ */
