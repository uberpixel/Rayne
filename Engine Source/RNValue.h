//
//  RNValue.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VALUE_H__
#define __RAYNE_VALUE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTypeTranslator.h"
#include "RNVector.h"
#include "RNColor.h"
#include "RNMatrixQuaternion.h"
#include "RNAny.h"

namespace RN
{
	class Value : public Object
	{
	public:
		template<class T>
		Value(const T &value) :
			_type(TypeTranslator<T>::value),
			_size(sizeof(T))
		{
			const uint8 *source = reinterpret_cast<const uint8 *>(&value);
			
			_storage = new uint8[_size];
			std::copy(source, source + _size, _storage);
		}
		RNAPI Value(const Value *other);
		RNAPI Value(Deserializer *deserializer);
		RNAPI ~Value();
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI static Value *WithVector2(const Vector2& vector);
		RNAPI static Value *WithVector3(const Vector3& vector);
		RNAPI static Value *WithVector4(const Vector4& vector);
		
		RNAPI static Value *WithColor(const Color& color);
		
		RNAPI static Value *WithQuaternion(const Quaternion& quaternion);
		RNAPI static Value *WithMatrix(const Matrix& matrix);
		
		template<class T>
		T GetValue() const
		{
			RN_ASSERT(TypeTranslator<T>::value == _type, "Type mismatch!");
			return static_cast<T>(*(reinterpret_cast<T *>(_storage)));
		}
		
		char GetTypeValue()
		{
			return _type;
		}
		
	private:
		char _type;
		size_t _size;
		uint8 *_storage;
		
		RNDeclareMeta(Value)
	};
}

#endif /* __RAYNE_VALUE_H__ */
