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
		enum class Mode
		{
			Serialize,
			Deserialize
		};
		
		RNAPI virtual void EncodeBytes(void *data, size_t size);
		RNAPI virtual void EncodeObject(Object *object);
		RNAPI virtual void EncodeRootObject(Object *object);
		
		RNAPI virtual void EncodeBool(bool value);
		RNAPI virtual void EncodeDouble(double value);
		RNAPI virtual void EncodeFloat(float value);
		RNAPI virtual void EncodeInt32(int32 value);
		RNAPI virtual void EncodeInt64(int64 value);
		
		RNAPI virtual void EncodeVector2(const Vector2& value);
		RNAPI virtual void EncodeVector3(const Vector3& value);
		RNAPI virtual void EncodeVector4(const Vector4& value);
		
		RNAPI virtual void EncodeMatrix(const Matrix& value);
		RNAPI virtual void EncodeQuarternion(const Quaternion& value);
		
		RNAPI virtual void *DecodeBytes(size_t *length);
		RNAPI virtual Object *DecodeObject();
		
		RNAPI virtual bool DecodeBool();
		RNAPI virtual double DecodeDouble();
		RNAPI virtual float DecodeFloat();
		RNAPI virtual int32 DecodeInt32();
		RNAPI virtual int64 DecodeInt64();
		
		RNAPI virtual Vector2 DecodeVector2();
		RNAPI virtual Vector3 DecodeVector3();
		RNAPI virtual Vector4 DecodeVector4();
		
		RNAPI virtual Matrix DecodeMatrix();
		RNAPI virtual Quaternion DecodeQuaternion();
		
		RNAPI virtual uint32 AppVersion() const;
		
		RNAPI Mode SerializerMode() const { return _mode; }
		
	protected:
		RNAPI Serializer(Mode mode);
		RNAPI virtual ~Serializer();
		
	private:
		Mode _mode;
	};
	
	class FlatSerializer : public Serializer
	{
	public:		
		RNAPI FlatSerializer();
		RNAPI FlatSerializer(Data *data);
		RNAPI ~FlatSerializer() override;
		
		RNAPI void EncodeBytes(void *data, size_t size) override;
		RNAPI void EncodeObject(Object *object) override;
		RNAPI void EncodeRootObject(Object *object) override;
		
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
		
		RNAPI void *DecodeBytes(size_t *length) override;
		RNAPI Object *DecodeObject() override;
		
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
		
		RNAPI Data *SerializedData() const;
		
	private:
		void AssertType(char expected, size_t *size);
		void PeekHeader(char *type, size_t *size);
		void DecodeHeader(char *type, size_t *size);
		void DecodeData(char expected, void *buffer, size_t size);
		
		void EncodeData(char type, size_t size, const void *data);
		
		uint32 EncodeClassName(String *name);
		
		Data *_data;
		Dictionary *_nametable;
		size_t _index;
	};
}

#endif /* __RAYNE_SERIALIZATION_H__ */
