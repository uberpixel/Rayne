//
//  RNNumber.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNumber.h"

namespace RN
{
	RNDeclareMeta(Number)
	
#define NumberPrimitiveAccess(type, target) static_cast<target>(*((type *)_buffer))
#define NumberAccessAndConvert(var, type) \
	do { \
		switch(_type) \
		{ \
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
	
	
	void Number::CopyData(const void *data, size_t size, Type type)
	{
		const uint8 *source = static_cast<const uint8 *>(data);
		
		_buffer = new uint8[size];
		_type = type;
		
		std::copy(source, source + size, _buffer);
	}
	
	
	float Number::FloatValue() const
	{
		float value;
		NumberAccessAndConvert(value, float);
		
		return value;
	}
	double Number::DoubleValue() const
	{
		double value;
		NumberAccessAndConvert(value, double);
		
		return value;
	}
	
	
	int8 Number::Int8Value() const
	{
		int8 value;
		NumberAccessAndConvert(value, int8);
		
		return value;
	}
	int16 Number::Int16Value() const
	{
		int16 value;
		NumberAccessAndConvert(value, int16);
		
		return value;
	}
	int32 Number::Int32Value() const
	{
		int32 value;
		NumberAccessAndConvert(value, int32);
		
		return value;
	}
	int64 Number::Int64Value() const
	{
		int64 value;
		NumberAccessAndConvert(value, int64);
		
		return value;
	}
	
	
	uint8 Number::Uint8Value() const
	{
		uint8 value;
		NumberAccessAndConvert(value, uint8);
		
		return value;
	}
	uint16 Number::Uint16Value() const
	{
		uint16 value;
		NumberAccessAndConvert(value, uint16);
		
		return value;
	}
	uint32 Number::Uint32Value() const
	{
		uint32 value;
		NumberAccessAndConvert(value, uint32);
		
		return value;
	}
	uint64 Number::Uint64Value() const
	{
		uint64 value;
		NumberAccessAndConvert(value, uint64);
		
		return value;
	}
	
#undef NumberPrimitiveAccess
#undef NumberAccessAndConvert
}
