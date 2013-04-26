//
//  RNNumber.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NUMBER_H__
#define __RAYNE_NUMBER_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Number : public Object
	{
	public:
		RNAPI Number(float value);
		RNAPI Number(double value);
		
		RNAPI Number(int8 value);
		RNAPI Number(int16 value);
		RNAPI Number(int32 value);
		RNAPI Number(int64 value);
		
		RNAPI Number(uint8 value);
		RNAPI Number(uint16 value);
		RNAPI Number(uint32 value);
		RNAPI Number(uint64 value);
		RNAPI virtual ~Number();
		
		RNAPI static Number *WithFloat(float value);
		RNAPI static Number *WithDouble(double value);
		
		RNAPI static Number *WithInt8(int8 value);
		RNAPI static Number *WithInt16(int16 value);
		RNAPI static Number *WithInt32(int32 value);
		RNAPI static Number *WithInt64(int64 value);
		
		RNAPI static Number *WithUint8(uint8 value);
		RNAPI static Number *WithUint16(uint16 value);
		RNAPI static Number *WithUint32(uint32 value);
		RNAPI static Number *WithUint64(uint64 value);
		
		
		RNAPI float FloatValue() const;
		RNAPI double DoubleValue() const;
		
		RNAPI int8 Int8Value() const;
		RNAPI int16 Int16Value() const;
		RNAPI int32 Int32Value() const;
		RNAPI int64 Int64Value() const;
		
		RNAPI uint8 Uint8Value() const;
		RNAPI uint16 Uint16Value() const;
		RNAPI uint32 Uint32Value() const;
		RNAPI uint64 Uint64Value() const;
		
	private:		
		enum Type
		{
			TypeInt8,
			TypeInt16,
			TypeInt32,
			TypeInt64,
			TypeUint8,
			TypeUint16,
			TypeUint32,
			TypeUint64,
			TypeFloat32,
			TypeFloat64
		};
		
		void CopyData(const void *data, size_t size, Type type);
		
		uint8 *_buffer;
		Type _type;
		
		RNDefineConstructorlessMeta(Number, Object);
	};
}

#endif /* __RAYNE_NUMBER_H__ */
