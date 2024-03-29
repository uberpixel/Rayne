//
//  RNNumber.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNumber.h"

namespace RN
{
	RNDefineMeta(Number, Object)
	
#define NumberPrimitiveAccess(type, target) static_cast<target>(*((type *)_buffer))
#define NumberIsSignedInteger(type) (type == Type::Int8 || type == Type::Int16 || type == Type::Int32 || type == Type::Int64)
#define NumberIsUnsignedInteger(type) (type == Type::Uint8 || type == Type::Uint16 || type == Type::Uint32 || type == Type::Uint64)
#define NumberIsReal(type) (type == Type::Float32 || type == Type::Float64)
#define NumberIsBoolean(type) (type == Type::Boolean)

#define NumberAccessAndConvert(var, type) \
	do { \
		switch(_type) \
		{ \
			case Type::Boolean: \
				var = NumberPrimitiveAccess(bool, type); \
				break; \
			case Type::Int8: \
				var = NumberPrimitiveAccess(int8, type); \
				break; \
			case Type::Uint8: \
				var = NumberPrimitiveAccess(uint8, type); \
				break; \
			case Type::Int16: \
				var = NumberPrimitiveAccess(int16, type); \
				break; \
			case Type::Uint16: \
				var = NumberPrimitiveAccess(uint16, type); \
				break; \
			case Type::Int32: \
				var = NumberPrimitiveAccess(int32, type); \
				break; \
			case Type::Uint32: \
				var = NumberPrimitiveAccess(uint32, type); \
				break; \
			case Type::Int64: \
				var = NumberPrimitiveAccess(int64, type); \
				break; \
			case Type::Uint64: \
				var = NumberPrimitiveAccess(uint64, type); \
				break; \
			case Type::Float32: \
				var = NumberPrimitiveAccess(float, type); \
				break; \
			case Type::Float64: \
				var = NumberPrimitiveAccess(double, type); \
				break; \
			} \
		} while(0)

	Number::Number(Deserializer *deserializer)
	{
		size_t size;
		uint8 *bytes = static_cast<uint8 *>(deserializer->DecodeBytes(&size));
		
		_type = static_cast<Type>(*bytes);
		
		bytes ++;
		size = SizeForType(_type);
		
		_buffer = new uint8[size];
		std::copy(bytes, bytes + size, _buffer);
	}
	
	Number::Number(const Number *number)
	{
		CopyData(number->_buffer, SizeForType(number->_type), number->_type);
	}
	
	Number::Number(bool value)
	{
		CopyData(&value, sizeof(bool), Type::Boolean);
	}
	Number::Number(float value)
	{
		CopyData(&value, sizeof(float), Type::Float32);
	}
	Number::Number(double value)
	{
		CopyData(&value, sizeof(double), Type::Float64);
	}
	
	Number::Number(int8 value)
	{
		CopyData(&value, sizeof(int8), Type::Int8);
	}
	Number::Number(int16 value)
	{
		CopyData(&value, sizeof(int16), Type::Int16);
	}
	Number::Number(int32 value)
	{
		CopyData(&value, sizeof(int32), Type::Int32);
	}
	Number::Number(int64 value)
	{
		CopyData(&value, sizeof(int64), Type::Int64);
	}
	
	Number::Number(uint8 value)
	{
		CopyData(&value, sizeof(uint8), Type::Uint8);
	}
	Number::Number(uint16 value)
	{
		CopyData(&value, sizeof(uint16), Type::Uint16);
	}
	Number::Number(uint32 value)
	{
		CopyData(&value, sizeof(uint32), Type::Uint32);
	}
	Number::Number(uint64 value)
	{
		CopyData(&value, sizeof(uint64), Type::Uint64);
	}
	
	Number::~Number()
	{
		delete [] _buffer;
	}
	
	
	Number *Number::WithBool(bool value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithFloat(float value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithDouble(double value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	
	Number *Number::WithInt8(int8 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithInt16(int16 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithInt32(int32 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithInt64(int64 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	
	Number *Number::WithUint8(uint8 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithUint16(uint16 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithUint32(uint32 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	Number *Number::WithUint64(uint64 value)
	{
		Number *number = new Number(value);
		return number->Autorelease();
	}
	
	
	
	void Number::Serialize(Serializer *serializer) const
	{
#if RN_PLATFORM_WINDOWS
		#pragma pack(push, 1)

		struct
		{
			char type;
			char bytes[32];
		} data;

		#pragma pack(pop)
#else
		struct __attribute__((packed))
		{
			char type;
			char bytes[32];
		} data;
#endif
		
		size_t size = SizeForType(_type);
		
		data.type = static_cast<char>(_type);
		std::copy(_buffer, _buffer + size, data.bytes);
		
		serializer->EncodeBytes(&data, size + sizeof(char));
	}
	
	void Number::CopyData(const void *data, size_t size, Type type)
	{
		const uint8 *source = static_cast<const uint8 *>(data);
		
		_buffer = new uint8[size];
		_type = type;
		
		std::copy(source, source + size, _buffer);
	}
	
	
	float Number::GetFloatValue() const
	{
		float value = 0.0f;
		NumberAccessAndConvert(value, float);
		
		return value;
	}
	double Number::GetDoubleValue() const
	{
		double value = 0.0;
		NumberAccessAndConvert(value, double);
		
		return value;
	}
	
	bool Number::GetBoolValue() const
	{
		bool value = false;
		NumberAccessAndConvert(value, bool);
		
		return value;
	}
	
	
	int8 Number::GetInt8Value() const
	{
		int8 value = 0;
		NumberAccessAndConvert(value, int8);
		
		return value;
	}
	int16 Number::GetInt16Value() const
	{
		int16 value = 0;
		NumberAccessAndConvert(value, int16);
		
		return value;
	}
	int32 Number::GetInt32Value() const
	{
		int32 value = 0;
		NumberAccessAndConvert(value, int32);
		
		return value;
	}
	int64 Number::GetInt64Value() const
	{
		int64 value = 0;
		NumberAccessAndConvert(value, int64);
		
		return value;
	}
	
	
	uint8 Number::GetUint8Value() const
	{
		uint8 value = 0;
		NumberAccessAndConvert(value, uint8);
		
		return value;
	}
	uint16 Number::GetUint16Value() const
	{
		uint16 value = 0;
		NumberAccessAndConvert(value, uint16);
		
		return value;
	}
	uint32 Number::GetUint32Value() const
	{
		uint32 value = 0;
		NumberAccessAndConvert(value, uint32);
		
		return value;
	}
	uint64 Number::GetUint64Value() const
	{
		uint64 value = 0;
		NumberAccessAndConvert(value, uint64);
		
		return value;
	}
	
	size_t Number::SizeForType(Type type)
	{
		switch(type)
		{
			case Type::Int8:
			case Type::Uint8:
				return 1;
				
			case Type::Int16:
			case Type::Uint16:
				return 2;
				
			case Type::Int32:
			case Type::Uint32:
			case Type::Float32:
				return 4;
			
			case Type::Int64:
			case Type::Uint64:
			case Type::Float64:
				return 8;
				
			case Type::Boolean:
				return sizeof(bool);
		}

		throw InconsistencyException("Invalid type!");
	}
	
	size_t Number::GetHash() const
	{
		const uint8 *bytes = _buffer;
		const uint8 *end = bytes + SizeForType(_type);
		
		size_t hash = 0;
		
		while(bytes < end)
		{
			hash ^= (hash << 5) + (hash >> 2) + *bytes;
			bytes ++;
		}
		
		return hash;
	}
	bool Number::IsEqual(const Object *other) const
	{
		const Number *number = other->Downcast<Number>();
		if(!number)
			return false;
		
		bool integer = ((NumberIsSignedInteger(_type) || NumberIsUnsignedInteger(_type)) && (NumberIsSignedInteger(number->_type) || NumberIsUnsignedInteger(number->_type)));
		if(integer)
		{
			if((NumberIsSignedInteger(_type) && NumberIsSignedInteger(number->_type)) || (NumberIsUnsignedInteger(_type) && NumberIsUnsignedInteger(number->_type)))
			{
				return (GetUint64Value() == number->GetUint64Value());
			}
			
			if(NumberIsSignedInteger(_type))
			{
				return (GetInt64Value() == number->GetUint64Value());
			}
			
			if(!NumberIsSignedInteger(_type))
			{
				return (GetUint64Value() == number->GetInt64Value());
			}
		}
		
		if(NumberIsReal(_type) && NumberIsReal(number->_type))
		{
			return (GetDoubleValue() == number->GetDoubleValue());
		}
		
		if(NumberIsBoolean(_type) && NumberIsBoolean(number->_type))
		{
			return (GetBoolValue() == number->GetBoolValue());
		}
		
		return false;
	}
	
#undef NumberPrimitiveAccess
#undef NumberAccessAndConvert
}
