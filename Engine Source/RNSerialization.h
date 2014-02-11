//
//  RNSerialization.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SERIALIZATION_H__
#define __RAYNE_SERIALIZATION_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNVector.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"

namespace RN
{
	class Data;
	class Dictionary;
	class String;
	
	class Serializer
	{
	public:
		RNAPI virtual void EncodeBytes(void *data, size_t size) = 0;
		RNAPI virtual void EncodeObject(Object *object) = 0;
		RNAPI virtual void EncodeRootObject(Object *object) = 0;
		RNAPI virtual void EncodeConditionalObject(Object *object) = 0;
		
		RNAPI virtual void EncodeBool(bool value) = 0;
		RNAPI virtual void EncodeDouble(double value) = 0;
		RNAPI virtual void EncodeFloat(float value) = 0;
		RNAPI virtual void EncodeInt32(int32 value) = 0;
		RNAPI virtual void EncodeInt64(int64 value) = 0;
		
		RNAPI virtual void EncodeVector2(const Vector2& value) = 0;
		RNAPI virtual void EncodeVector3(const Vector3& value) = 0;
		RNAPI virtual void EncodeVector4(const Vector4& value) = 0;
		
		RNAPI virtual void EncodeMatrix(const Matrix& value) = 0;
		RNAPI virtual void EncodeQuarternion(const Quaternion& value) = 0;
		
		RNAPI virtual Data *GetSerializedData() const = 0;
		
	protected:
		RNAPI virtual ~Serializer();
	};
	
	class Deserializer
	{
	public:
		RNAPI virtual void *DecodeBytes(size_t *length) = 0;
		RNAPI virtual Object *DecodeObject() = 0;
		RNAPI virtual Object *DecodeConditionalObject() = 0;
		
		RNAPI virtual bool DecodeBool() = 0;
		RNAPI virtual double DecodeDouble() = 0;
		RNAPI virtual float DecodeFloat() = 0;
		RNAPI virtual int32 DecodeInt32() = 0;
		RNAPI virtual int64 DecodeInt64() = 0;
		
		RNAPI virtual Vector2 DecodeVector2() = 0;
		RNAPI virtual Vector3 DecodeVector3() = 0;
		RNAPI virtual Vector4 DecodeVector4() = 0;
		
		RNAPI virtual Matrix DecodeMatrix() = 0;
		RNAPI virtual Quaternion DecodeQuaternion() = 0;
		
	protected:
		RNAPI virtual ~Deserializer();
	};
	
	
	
	class FlatSerializer : public Serializer
	{
	public:		
		RNAPI FlatSerializer();
		RNAPI ~FlatSerializer() override;
		
		RNAPI void EncodeBytes(void *data, size_t size) override;
		RNAPI void EncodeObject(Object *object) override;
		RNAPI void EncodeRootObject(Object *object) override;
		RNAPI void EncodeConditionalObject(Object *object) override;
		
		RNAPI void EncodeBool(bool value) override;
		RNAPI void EncodeDouble(double value) override;
		RNAPI void EncodeFloat(float value) override;
		RNAPI void EncodeInt32(int32 value) override;
		RNAPI void EncodeInt64(int64 value) override;
		
		RNAPI void EncodeVector2(const Vector2& value) override;
		RNAPI void EncodeVector3(const Vector3& value) override;
		RNAPI void EncodeVector4(const Vector4& value) override;
		
		RNAPI void EncodeMatrix(const Matrix& value) override;
		RNAPI void EncodeQuarternion(const Quaternion& value) override;
		
		RNAPI Data *GetSerializedData() const override;
		
	private:
		void EncodeData(char type, size_t size, const void *data);
		uint32 EncodeClassName(String *name);
		
		Data *_data;
		Dictionary *_nametable;
		size_t _index;
	};
	
	class FlatDeserializer : public Deserializer
	{
	public:
		RNAPI FlatDeserializer(Data *data);
		RNAPI ~FlatDeserializer() override;
		
		RNAPI void *DecodeBytes(size_t *length) override;
		RNAPI Object *DecodeObject() override;
		RNAPI Object *DecodeConditionalObject() override;
		
		RNAPI bool DecodeBool() override;
		RNAPI double DecodeDouble() override;
		RNAPI float DecodeFloat() override;
		RNAPI int32 DecodeInt32() override;
		RNAPI int64 DecodeInt64() override;
		
		RNAPI Vector2 DecodeVector2() override;
		RNAPI Vector3 DecodeVector3() override;
		RNAPI Vector4 DecodeVector4() override;
		
		RNAPI Matrix DecodeMatrix() override;
		RNAPI Quaternion DecodeQuaternion() override;
		
	private:
		void AssertType(char expected, size_t *size);
		void PeekHeader(char *type, size_t *size);
		void DecodeHeader(char *type, size_t *size);
		void DecodeData(char expected, void *buffer, size_t size);
		
		Data *_data;
		Dictionary *_nametable;
		size_t _index;
	};
}

#endif /* __RAYNE_SERIALIZATION_H__ */
