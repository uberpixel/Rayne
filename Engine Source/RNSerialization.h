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
		
		virtual void EncodeBytes(void *data, size_t size);
		virtual void EncodeObject(Object *object);
		virtual void EncodeRootObject(Object *object);
		
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
		virtual Object *DecodeObject();
		
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
		
		Mode SerializerMode() const { return _mode; }
		
	protected:
		Serializer(Mode mode);
		virtual ~Serializer();
		
	private:
		Mode _mode;
	};
	
	class FlatSerializer : public Serializer
	{
	public:		
		FlatSerializer();
		FlatSerializer(Data *data);
		~FlatSerializer() override;
		
		void EncodeBytes(void *data, size_t size) override;
		void EncodeObject(Object *object) override;
		void EncodeRootObject(Object *object) override;
		
		void EncodeBool(bool value) override;
		void EncodeDouble(double value) override;
		void EncodeFloat(float value) override;
		
		void *DecodeBytes(size_t *length) override;
		Object *DecodeObject() override;
		
		Data *SerializedData() const;
		
	private:
		void AssertType(char expected, size_t *size);
		void PeekHeader(char *type, size_t *size);
		void DecodeHeader(char *type, size_t *size);
		void DecodeData(void *buffer, size_t size);
		
		void EncodeData(char type, size_t size, const void *data);
		
		uint32 EncodeClassName(String *name);
		
		Data *_data;
		Dictionary *_nametable;
		size_t _index;
	};
}

#endif /* __RAYNE_SERIALIZATION_H__ */
