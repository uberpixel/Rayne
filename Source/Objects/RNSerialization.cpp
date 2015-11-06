//
//  RNSerialization.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSerialization.h"
#include "RNDictionary.h"
#include "RNData.h"
#include "RNNumber.h"
#include "RNString.h"

namespace RN
{
	RNDefineMeta(Serializer, Object)
	RNDefineMeta(Deserializer, Object)
	
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
		
		_nametable->Enumerate([&](Object *value, const Object *key, bool &stop) {
			const String *name = static_cast<const String *>(key);
			Number *index = static_cast<Number *>(value);
			
			size_t length;
			uint8 *tname = name->GetBytesWithEncoding(Encoding::UTF8, false, length);
			
			temp = static_cast<uint32>(length);
			
			result->Append(&temp, sizeof(uint32));
			result->Append(tname, temp);
			
			temp = index->GetUint32Value();
			result->Append(&temp, sizeof(uint32));
		});
		
		size_t offset = result->GetLength();
		
		result->Append(_data);

		for(uint64 location : _jumpTable)
		{
			location += offset + 5;
			
			uint64 jump;
			
			result->GetBytesInRange(&jump, Range(location, sizeof(uint64)));
			
			jump += offset;
			
			result->ReplaceBytes(&jump, Range(location, sizeof(uint64)));
		}
		
		
		for(auto i = _conditionalTable.begin(); i != _conditionalTable.end(); i ++)
		{
			const Object *object = i->first;
			const std::vector<uint64> &locations = i->second;
			
			auto iterator = _objectTable.find(object);
			if(iterator != _objectTable.end())
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
				
				header.type = 'J';
				header.size = sizeof(uint64);
				
				uint64 location = iterator->second + offset;
				
				for(uint64 index : locations)
				{
					result->ReplaceBytes(&header,   Range(index + offset, sizeof(header)));
					result->ReplaceBytes(&location, Range(index + offset + sizeof(header), sizeof(uint64)));
				}
			}
		}
		
		return result->Autorelease();
	}
	
	
	void FlatSerializer::EncodeBytes(void *data, size_t size)
	{
		EncodeData('+', size, data);
	}
	
	void FlatSerializer::EncodeObject(const Object *object)
	{
		if(!object)
		{
			uint64 temp = 0xdeadbeef;
			EncodeData('L', sizeof(uint64), &temp);
			
			return;
		}
		
		if(/*!object->IsKindOfClass(Asset::GetMetaClass()) && */!object->GetClass()->SupportsSerialization())
			throw InvalidArgumentException(RNSTR("EncodeObject() only works with objects that support serialization (tried serializing '" << object->GetClass()->GetFullname() << "')!"));
		
		auto iterator = _objectTable.find(object);
		if(iterator != _objectTable.end())
		{
			_jumpTable.push_back(_data->GetLength());
			
			uint64 index = iterator->second;

			EncodeData('J', sizeof(uint64), &index);
			return;
		}
		
		uint32 index  = EncodeClassName(String::WithString(object->GetClass()->GetFullname().c_str()));
		size_t tindex = _data->GetLength();
		
		EncodeData('@', sizeof(uint32), &index);
		
		uint32 temp = static_cast<uint32>(_data->GetLength());
		object->Serialize(this);
		
		uint32 length = static_cast<uint32>(_data->GetLength()) - temp;
		_data->ReplaceBytes(&length, Range(temp - (sizeof(uint32) * 2), sizeof(uint32))); // Replace the encoded size for index
	
		_objectTable.emplace(object, static_cast<uint64>(tindex));
	}
	
	void FlatSerializer::EncodeRootObject(const Object *object)
	{
		_data->Release();
		_nametable->Release();
		
		_data = new Data();
		_nametable = new Dictionary();
		
		EncodeObject(object);
	}
	
	void FlatSerializer::EncodeConditionalObject(const Object *object)
	{
		size_t index = _data->GetLength();
		
		uint64 temp = 0xdeadbeef;
		EncodeData('L', sizeof(uint64), &temp);
		
		auto iterator = _conditionalTable.find(object);
		if(iterator != _conditionalTable.end())
		{
			std::vector<uint64> &vector = iterator->second;
			vector.push_back(static_cast<uint64>(index));
			
			return;
		}
		
		_conditionalTable.emplace(object, std::vector<uint64>{ index });
	}
	
	void FlatSerializer::EncodeString(const std::string &string)
	{
		EncodeData('s', string.size() + 1, string.c_str());
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
	
	void FlatSerializer::EncodeVector2(const Vector2 &value)
	{
		EncodeData('2', sizeof(Vector2), &value);
	}
	void FlatSerializer::EncodeVector3(const Vector3 &value)
	{
		EncodeData('3', sizeof(Vector3), &value);
	}
	void FlatSerializer::EncodeVector4(const Vector4 &value)
	{
		EncodeData('4', sizeof(Vector4), &value);
	}
	void FlatSerializer::EncodeColor(const Color &value)
	{
		EncodeData('c', sizeof(Color), &value);
	}
	
	void FlatSerializer::EncodeMatrix(const Matrix &value)
	{
		EncodeData('m', sizeof(Matrix), &value);
	}
	void FlatSerializer::EncodeQuarternion(const Quaternion &value)
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
		_data = data->Copy();
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
		
		if(type != expected)
			throw InconsistencyException(RNSTRF("Expected type %c but got %c!", expected, type));
		
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
		{
			char type;
			size_t size;
			
			PeekHeader(&type, &size);
			
			if(type == 'L')
			{
				AssertType('L', &size);
				_index += size;
				
				return nullptr;
			}
			
			if(type == 'J')
			{
				AssertType('J', &size);

				uint64 index;
				
				_data->GetBytesInRange(&index, Range(_index, sizeof(uint64)));
				_index += size;
				
				
				
				auto iterator = _objectTable.find(index);
				if(iterator == _objectTable.end())
				{
					size_t temp = _index;
					_index = static_cast<size_t>(index);
					
					Object *object = DecodeObject();
					
					size_t size = (_index - index) - 5;
					_index = temp;
					
					
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
					
					header.type = 'J';
					header.size = static_cast<uint32>(size);
					
					_data->ReplaceBytes(&header, Range(index, sizeof(header)));
					_data->ReplaceBytes(&index,  Range(index + sizeof(header), sizeof(uint64)));
					
					return object;
				}
				
				return iterator->second;
			}
		}
		
		uint64 temp = _index;
		
		size_t size;
		uint32 index;
		AssertType('@', &size);
		
		_data->GetBytesInRange(&index, Range(_index, sizeof(uint32)));
		_index += sizeof(uint32);
		
		String *name = _nametable->GetObjectForKey<String>(Number::WithUint32(index));
		MetaClass *mclass = Catalogue::GetSharedInstance()->GetClassWithName(name->GetUTF8String());
		
		/*if(mclass->InheritsFromClass(Asset::GetMetaClass()))
		{
			Object *result = Asset::Deserialize(this);
			_objectTable.emplace(temp, result);
			
			return result;
		}*/
		
		Object *result = mclass->ConstructWithDeserializer(this);
		_objectTable.emplace(temp, result);
		
		
		return result->Autorelease();
	}
	
	std::string FlatDeserializer::DecodeString()
	{
		size_t size;
		AssertType('s', &size);
		
		Data *data = _data->GetDataInRange(Range(_index, size));
		_index += size;
		
		return std::string(reinterpret_cast<char *>(data->GetBytes()));
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
		Vector4 result;
		DecodeData('4', &result, sizeof(Vector4));
		
		return result;
	}
	Color FlatDeserializer::DecodeColor()
	{
		Color result;
		DecodeData('c', &result, sizeof(Color));
		
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
		
		if(tsize != size)
			throw InconsistencyException(RNSTR("Size inconsitency (" << tsize << " != " << size << ")"));
		
		_data->GetBytesInRange(buffer, Range(_index, size));
		_index += size;
	}
}
