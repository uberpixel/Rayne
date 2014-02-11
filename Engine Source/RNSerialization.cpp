//
//  RNSerialization.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSerialization.h"
#include "RNData.h"
#include "RNDictionary.h"
#include "RNNumber.h"
#include "RNString.h"

namespace RN
{
	Serializer::~Serializer()
	{}
	Deserializer::~Deserializer()
	{}
	
	// ---------------------
	// MARK: -
	// MARK: FlatSerializer
	// ---------------------
	
	FlatSerializer::FlatSerializer()
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
	
	Data *FlatSerializer::GetSerializedData() const
	{
		Data *result = new Data();
		
		uint32 temp;
		
		temp = static_cast<uint32>(_nametable->GetCount());
		result->Append(&temp, sizeof(uint32));
		
		_nametable->Enumerate([&](Object *value, Object *key, bool &stop) {
			String *name = static_cast<String *>(key);
			Number *index = static_cast<Number *>(value);
			
			size_t length;
			uint8 *tname = name->GetBytesWithEncoding(Encoding::UTF8, false, &length);
			
			temp = static_cast<uint32>(length);
			
			result->Append(&temp, sizeof(uint32));
			result->Append(tname, temp);
			
			temp = index->GetUint32Value();
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
		RN_ASSERT(object->Class()->SupportsSerialization(), "EncodeObject() only works with objects that support serialization!");
	
		uint32 index = EncodeClassName(String::WithString(object->Class()->Fullname().c_str()));
		EncodeData('@', sizeof(uint32), &index);
		
		uint32 temp = static_cast<uint32>(_data->GetLength());
		object->Serialize(this);
		
		uint32 length = static_cast<uint32>(_data->GetLength()) - temp;
		_data->ReplaceBytes(&length, Range(temp - (sizeof(uint32) * 2), sizeof(uint32))); // Replace the encoded size for index
	}
	
	void FlatSerializer::EncodeRootObject(Object *object)
	{
		_data->Release();
		_nametable->Release();
		
		_data = new Data();
		_nametable = new Dictionary();
		
		EncodeObject(object);
	}
	
	void FlatSerializer::EncodeConditionalObject(Object *object)
	{
		
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
		EncodeData('f', sizeof(float), &value);
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
#if RN_PLATFORM_WINDOWS
		#pragma pack(push,1)

		struct
		{
			char type;
			uint32 size;
		} header;

		#pragma pack(pop)
#else
		struct __attribute__((packed))
		{
			char type;
			uint32 size;
		} header;
#endif
		
		header.type = type;
		header.size = static_cast<uint32>(size);
		
		_data->Append(&header, sizeof(header));
		_data->Append(data, size);
	}
	
	uint32 FlatSerializer::EncodeClassName(String *name)
	{
		Number *index = _nametable->GetObjectForKey<Number>(name);
		if(!index)
		{
			index = Number::WithUint32(static_cast<uint32>(_nametable->GetCount()));
			_nametable->SetObjectForKey(index, name);
		}
		
		return index->GetUint32Value();
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Deserilization
	// ---------------------
	
	FlatDeserializer::FlatDeserializer(Data *data)
	{
		_data = data->Retain();
		_index = 0;
		
		// Decode the name table
		uint32 names;
		_data->GetBytesInRange(&names, Range(_index, sizeof(uint32)));
		_index += sizeof(uint32);
		
		_nametable = new Dictionary(names);
		for(uint32 i=0; i<names; i++)
		{
			uint32 length, temp;
			_data->GetBytesInRange(&length, Range(_index, sizeof(uint32)));
			_index += sizeof(uint32);
			
			char *buffer = new char[length + 1];
			_data->GetBytesInRange(buffer, Range(_index, length));
			_index += length;
			
			buffer[length] = '\0';
			_data->GetBytesInRange(&temp, Range(_index, sizeof(uint32)));
			_index += sizeof(uint32);
			
			String *name = String::WithBytes(buffer, Encoding::UTF8);
			Number *index = Number::WithUint32(temp);
			
			delete[] buffer;
			
			_nametable->SetObjectForKey(name, index); // When deserializing, the name table is in reverse. The index looks up the name!
		}
	}
	
	FlatDeserializer::~FlatDeserializer()
	{
		_data->Release();
		_nametable->Release();
	}
	
	
	void FlatDeserializer::AssertType(char expected, size_t *size)
	{
		char type;

		PeekHeader(&type, size);
		RN_ASSERT(type == expected, "Expected type %c but got %c", expected, type);
		
		DecodeHeader(nullptr, nullptr);
	}
	
	void *FlatDeserializer::DecodeBytes(size_t *length)
	{
		size_t size;
		AssertType('+', &size);
		
		Data *data = _data->GetDataInRange(Range(_index, size));
		_index += size;
		
		if(length)
			*length = size;
		
		return data->GetBytes();
	}
	
	Object *FlatDeserializer::DecodeObject()
	{
		size_t size;
		uint32 index;
		AssertType('@', &size);
		
		_data->GetBytesInRange(&index, Range(_index, sizeof(uint32)));
		_index += sizeof(uint32);
		
		String *name = _nametable->GetObjectForKey<String>(Number::WithUint32(index));
		MetaClassBase *mclass = Catalogue::GetSharedInstance()->GetClassWithName(name->GetUTF8String());
		
		Object *result = mclass->ConstructWithDeserializer(this);
		return result;
	}
	
	Object *FlatDeserializer::DecodeConditionalObject()
	{
		return nullptr;
	}
	
	bool FlatDeserializer::DecodeBool()
	{
		bool result;
		DecodeData('b', &result, sizeof(bool));
		
		return result;
	}
	
	double FlatDeserializer::DecodeDouble()
	{
		double result;
		char type;
		
		PeekHeader(&type, nullptr);
		switch(type)
		{
			case 'd':
				DecodeData(type, &result, sizeof(double));
				break;
				
			case 'f':
			{
				float temp;
				DecodeData(type, &temp, sizeof(float));
				result = temp;
				
				break;
			}
				
			default:
				AssertType('d', nullptr);
				result = 0.0; // Silence compiler warning
				break;
		}
		
		return result;
	}
	
	float FlatDeserializer::DecodeFloat()
	{
		float result;
		char type;
		
		PeekHeader(&type, nullptr);
		switch(type)
		{
			case 'f':
				DecodeData(type, &result, sizeof(float));
				break;
				
			case 'd':
			{
				double temp;
				DecodeData(type, &temp, sizeof(double));
				result = temp;
				
				break;
			}
				
			default:
				AssertType('f', nullptr);
				result = 0.0f; // Silence compiler warning
				break;
		}
		
		return result;
	}
	
	int32 FlatDeserializer::DecodeInt32()
	{
		int32 result;
		char type;
		
		PeekHeader(&type, nullptr);
		switch(type)
		{
			case 'i':
				DecodeData(type, &result, sizeof(int32));
				break;
				
			case 'l':
			{
				int64 temp;
				DecodeData(type, &temp, sizeof(int64));
				result = static_cast<int32>(temp);
				
				break;
			}
				
			default:
				AssertType('i', nullptr);
				result = 0; // Silence compiler warning
				break;
		}
		
		return result;
	}
	
	int64 FlatDeserializer::DecodeInt64()
	{
		int64 result;
		char type;
		
		PeekHeader(&type, nullptr);
		switch(type)
		{
			case 'l':
				DecodeData(type, &result, sizeof(int64));
				break;
				
			case 'i':
			{
				int32 temp;
				DecodeData(type, &temp, sizeof(int64));
				result = static_cast<int64>(temp);
				
				break;
			}
				
			default:
				AssertType('l', nullptr);
				result = 0; // Silence compiler warning
				break;
		}
		
		return result;
	}
	
	Vector2 FlatDeserializer::DecodeVector2()
	{
		Vector2 result;
		DecodeData('2', &result, sizeof(Vector2));
		
		return result;
	}
	Vector3 FlatDeserializer::DecodeVector3()
	{
		Vector3 result;
		DecodeData('3', &result, sizeof(Vector3));
		
		return result;
	}
	Vector4 FlatDeserializer::DecodeVector4()
	{
		int64 result;
		DecodeData('4', &result, sizeof(Vector4));
		
		return result;
	}
	
	Matrix FlatDeserializer::DecodeMatrix()
	{
		Matrix result;
		DecodeData('m', &result, sizeof(Matrix));
		
		return result;
	}
	Quaternion FlatDeserializer::DecodeQuaternion()
	{
		Quaternion result;
		DecodeData('q', &result, sizeof(Quaternion));
		
		return result;
	}
	
	
	void FlatDeserializer::PeekHeader(char *type, size_t *size)
	{
#if RN_PLATFORM_WINDOWS
		#pragma pack(push,1)

		struct
		{
			char type;
			uint32 size;
		} header;

		#pragma pack(pop)
#else
		struct __attribute__((packed))
		{
			char type;
			uint32 size;
		} header;
#endif
		
		_data->GetBytesInRange(&header, Range(_index, sizeof(header)));
		
		if(type)
			*type = header.type;
		
		if(size)
			*size = header.size;
	}
	
	void FlatDeserializer::DecodeHeader(char *type, size_t *size)
	{
		PeekHeader(type, size);
		_index += 5;
	}
	
	void FlatDeserializer::DecodeData(char expected, void *buffer, size_t size)
	{
		size_t tsize;
		AssertType(expected, &tsize);
		
		RN_ASSERT(tsize == size, "");
		
		_data->GetBytesInRange(buffer, Range(_index, size));
		_index += size;
	}
}
