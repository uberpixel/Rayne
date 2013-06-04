//
//  RNSerialization.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSerialization.h"
#include "RNData.h"
#include "RNDictionary.h"
#include "RNNumber.h"
#include "RNString.h"

namespace RN
{
	Serializer::Serializer(Mode mode)
	{
		_mode = mode;
	}
	Serializer::~Serializer()
	{}
	
	void Serializer::EncodeBytes(void *data, size_t size)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeObject(Object *object)
	{
		throw ErrorException(0);
	}
	void Serializer::EncodeRootObject(Object *object)
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
	Object *Serializer::DecodeObject()
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
	
	
	// ---------------------
	// MARK: -
	// MARK: FlatSerializer
	// ---------------------
	
	FlatSerializer::FlatSerializer() :
		Serializer(Mode::Serialize)
	{
		_data      = new Data();
		_nametable = new Dictionary();
		_index = 0;
	}
	
	FlatSerializer::~FlatSerializer()
	{
		_data->Release();
		_nametable->Release();
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Seriliazation
	// ---------------------
	
	Data *FlatSerializer::SerializedData() const
	{
		Data *result = new Data();
		
		uint32 temp;
		
		temp = static_cast<uint32>(_nametable->Count());
		result->Append(&temp, sizeof(uint32));
		
		_nametable->Enumerate([&](Object *value, Object *key, bool *stop) {
			String *name = static_cast<String *>(key);
			Number *index = static_cast<Number *>(value);
			
			size_t length;
			uint8 *tname = name->BytesWithEncoding(String::Encoding::UTF8, false, &length);
			
			temp = static_cast<uint32>(length);
			
			result->Append(&temp, sizeof(uint32));
			result->Append(tname, temp);
			
			temp = index->Uint32Value();
			result->Append(&temp, sizeof(uint32));
		});
		
		result->Append(_data);
		return result->Autorelease();
	}
	
	
	void FlatSerializer::EncodeBytes(void *data, size_t size)
	{
		EncodeData('+', size, data);
	}
	
	void FlatSerializer::EncodeObject(Object *object)
	{
		RN_ASSERT(SerializerMode() == Mode::Serialize, "EncodeObject() only works with serializing serializers!");
		RN_ASSERT(object->Class()->SupportsSerialization(), "EncodeObject() only works with objects that support serialization!");
	
		uint32 index = EncodeClassName(String::WithString(object->Class()->Fullname().c_str()));
		EncodeData('@', sizeof(uint32), &index);
		
		uint32 temp = static_cast<uint32>(_data->Length());
		object->Serialize(this);
		
		uint32 length = static_cast<uint32>(_data->Length()) - temp;
		_data->ReplaceBytes(&length, Range(temp - (sizeof(uint32) * 2), sizeof(uint32))); // Replace the encoded size for index
	}
	
	void FlatSerializer::EncodeRootObject(Object *object)
	{
		RN_ASSERT(SerializerMode() == Mode::Serialize, "EncodeRootObject() only works with serializing serializers!");
		
		_data->Release();
		_nametable->Release();
		
		_data = new Data();
		_nametable = new Dictionary();
		
		EncodeObject(object);
	}
	
	void FlatSerializer::EncodeBool(bool value)
	{
		EncodeData('b', sizeof(bool), &value);
	}
	
	void FlatSerializer::EncodeDouble(double value)
	{
		EncodeData('d', sizeof(double), &value);
	}
	
	void FlatSerializer::EncodeFloat(float value)
	{
		EncodeData('f', sizeof(double), &value);
	}
	
	void FlatSerializer::EncodeInt32(int32 value)
	{
		EncodeData('i', sizeof(int32), &value);
	}
	
	void FlatSerializer::EncodeInt64(int64 value)
	{
		EncodeData('l', sizeof(int64), &value);
	}
	
	void FlatSerializer::EncodeVector2(const Vector2& value)
	{
		EncodeData('2', sizeof(Vector2), &value);
	}
	void FlatSerializer::EncodeVector3(const Vector3& value)
	{
		EncodeData('3', sizeof(Vector2), &value);
	}
	void FlatSerializer::EncodeVector4(const Vector4& value)
	{
		EncodeData('4', sizeof(Vector2), &value);
	}
	
	void FlatSerializer::EncodeMatrix(const Matrix& value)
	{
		EncodeData('m', sizeof(Matrix), &value);
	}
	void FlatSerializer::EncodeQuarternion(const Quaternion& value)
	{
		EncodeData('q', sizeof(Quaternion), &value);
	}
	
	
	void FlatSerializer::EncodeData(char type, size_t size, const void *data)
	{
		RN_ASSERT(SerializerMode() == Mode::Serialize, "EncodeData() only works with serializing serializers!");
		
		struct __attribute__((packed))
		{
			char type;
			uint32 size;
		} header;
		
		header.type = type;
		header.size = static_cast<uint32>(size);
		
		_data->Append(&header, sizeof(header));
		_data->Append(data, size);
	}
	
	uint32 FlatSerializer::EncodeClassName(String *name)
	{
		Number *index = _nametable->ObjectForKey<Number>(name);
		if(!index)
		{
			index = Number::WithUint32(static_cast<uint32>(_nametable->Count()));
			_nametable->SetObjectForKey(index, name);
		}
		
		return index->Uint32Value();
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Deserilization
	// ---------------------
	
	FlatSerializer::FlatSerializer(Data *data) :
		Serializer(Mode::Deserialize)
	{
		_data = data->Retain();
		_index = 0;
		
		// Decode the name table
		uint32 names;
		_data->BytesInRange(&names, Range(_index, sizeof(uint32)));
		_index += sizeof(uint32);
		
		_nametable = new Dictionary(names);
		for(uint32 i=0; i<names; i++)
		{
			uint32 length, temp;
			_data->BytesInRange(&length, Range(_index, sizeof(uint32)));
			_index += sizeof(uint32);
			
			char *buffer = new char[length + 1];
			_data->BytesInRange(buffer, Range(_index, length));
			_index += length;
			
			buffer[length] = '\0';
			_data->BytesInRange(&temp, Range(_index, sizeof(uint32)));
			_index += sizeof(uint32);
			
			String *name = String::WithBytes(buffer, String::Encoding::UTF8);
			Number *index = Number::WithUint32(temp);
			
			delete[] buffer;
			
			_nametable->SetObjectForKey(name, index); // When deserializing, the name table is in reverse. The index looks up the name!
		}
	}
	
	void FlatSerializer::AssertType(char expected, size_t *size)
	{
		char type;

		PeekHeader(&type, size);
		RN_ASSERT(type == expected, "Expected type %c but got %c", expected, type);
		
		DecodeHeader(nullptr, nullptr);
	}
	
	void *FlatSerializer::DecodeBytes(size_t *length)
	{
		size_t size;
		AssertType('+', &size);
		
		Data *data = _data->DataInRange(Range(_index, size));
		_index += size;
		
		if(length)
			*length = size;
		
		return data->Bytes();
	}
	
	Object *FlatSerializer::DecodeObject()
	{
		size_t size;
		uint32 index;
		AssertType('@', &size);
		
		_data->BytesInRange(&index, Range(_index, sizeof(uint32)));
		_index += sizeof(uint32);
		
		String *name = _nametable->ObjectForKey<String>(Number::WithUint32(index));
		MetaClass *mclass = Catalogue::SharedInstance()->ClassWithName(name->UTF8String());
		
		Object *result = mclass->ConstructWithSerializer(this);
		return result;
	}
	
	bool FlatSerializer::DecodeBool()
	{
		bool result;
		DecodeData('b', &result, sizeof(bool));
		
		return result;
	}
	
	double FlatSerializer::DecodeDouble()
	{
		double result;
		DecodeData('d', &result, sizeof(double));
		
		return result;
	}
	
	float FlatSerializer::DecodeFloat()
	{
		float result;
		DecodeData('f', &result, sizeof(float));
		
		return result;
	}
	
	int32 FlatSerializer::DecodeInt32()
	{
		int32 result;
		DecodeData('i', &result, sizeof(int32));
		
		return result;
	}
	
	int64 FlatSerializer::DecodeInt64()
	{
		int64 result;
		DecodeData('l', &result, sizeof(int64));
		
		return result;
	}
	
	Vector2 FlatSerializer::DecodeVector2()
	{
		Vector2 result;
		DecodeData('2', &result, sizeof(Vector2));
		
		return result;
	}
	Vector3 FlatSerializer::DecodeVector3()
	{
		Vector3 result;
		DecodeData('3', &result, sizeof(Vector3));
		
		return result;
	}
	Vector4 FlatSerializer::DecodeVector4()
	{
		int64 result;
		DecodeData('4', &result, sizeof(Vector4));
		
		return result;
	}
	
	Matrix FlatSerializer::DecodeMatrix()
	{
		Matrix result;
		DecodeData('m', &result, sizeof(Matrix));
		
		return result;
	}
	Quaternion FlatSerializer::DecodeQuaternion()
	{
		Quaternion result;
		DecodeData('q', &result, sizeof(Quaternion));
		
		return result;
	}
	
	
	void FlatSerializer::PeekHeader(char *type, size_t *size)
	{
		RN_ASSERT(SerializerMode() == Mode::Deserialize, "PeekHeader() only works with deserializing serializers!");
		
		struct __attribute__((packed))
		{
			char type;
			uint32 size;
		} header;
		
		_data->BytesInRange(&header, Range(_index, sizeof(header)));
		
		if(type)
			*type = header.type;
		
		if(size)
			*size = header.size;
	}
	
	void FlatSerializer::DecodeHeader(char *type, size_t *size)
	{
		RN_ASSERT(SerializerMode() == Mode::Deserialize, "DecodeHeader() only works with deserializing serializers!");
		
		PeekHeader(type, size);
		_index += 5;
	}
	
	void FlatSerializer::DecodeData(char expected, void *buffer, size_t size)
	{
		RN_ASSERT(SerializerMode() == Mode::Deserialize, "DecodeHeader() only works with deserializing serializers!");
		
		size_t tsize;
		AssertType(expected, &tsize);
		
		_data->BytesInRange(buffer, Range(_index, size));
		_index += size;
	}
}
