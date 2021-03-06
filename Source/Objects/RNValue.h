//
//  RNValue.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VALUE_H__
#define __RAYNE_VALUE_H__

#include "../Base/RNBase.h"
#include "../Base/RNTypeTranslator.h"
#include "../Data/RNAny.h"
#include "../Math/RNVector.h"
#include "../Math/RNColor.h"
#include "../Math/RNMatrixQuaternion.h"
#include "RNObject.h"

namespace RN
{
	class Value : public Object
	{
	public:
		template<class T>
		Value(const T &value) :
			_type(TypeTranslator<T>::value),
			_size(sizeof(T)),
			_alignment(std::alignment_of<T>::value)
		{
			const uint8 *source = reinterpret_cast<const uint8 *>(&value);
			
			_storage = reinterpret_cast<uint8 *>(Memory::AllocateAligned(_size, _alignment));
			std::copy(source, source + _size, _storage);
		}
		RNAPI Value(const Value *other);
		RNAPI Value(Deserializer *deserializer);
		RNAPI ~Value();
		
		RNAPI void Serialize(Serializer *serializer) const override;
		RNAPI const String *GetDescription() const override;
		
		RNAPI size_t GetHash() const override;
		RNAPI bool IsEqual(const Object *other) const override;
		
		RNAPI static Value *WithVector2(const Vector2 &vector);
		RNAPI static Value *WithVector3(const Vector3 &vector);
		RNAPI static Value *WithVector4(const Vector4 &vector);
		
		RNAPI static Value *WithColor(const Color &color);
		
		RNAPI static Value *WithQuaternion(const Quaternion &quaternion);
		RNAPI static Value *WithMatrix(const Matrix &matrix);
		
		template<class T>
		bool CanConvertToType() const
		{
			return (TypeTranslator<T>::value == _type && sizeof(T) == _size);
		}
		
		template<class T>
		T GetValue() const
		{
			if(TypeTranslator<T>::value != _type || sizeof(T) != _size)
				throw InconsistencyException("Type mismatch!");
			
			return static_cast<T>(*(reinterpret_cast<T *>(_storage)));
		}
		
		template<class T>
		T &GetValue()
		{
			if(TypeTranslator<T>::value != _type || sizeof(T) != _size)
				throw InconsistencyException("Type mismatch!");
			
			return *(reinterpret_cast<T *>(_storage));
		}
		
		char GetValueType()
		{
			return _type;
		}
		
	private:
		char _type;
		size_t _size;
		size_t _alignment;
		uint8 *_storage;
		
		__RNDeclareMetaInternal(Value)
	};
	
	RNObjectClass(Value)
}

#endif /* __RAYNE_VALUE_H__ */
