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

#include "RNVector.h"
#include "RNColor.h"
#include "RNMatrixQuaternion.h"

namespace RN
{
	class Value : public Object
	{
	public:
		RNAPI Value(const void *ptr, size_t size, const std::type_info& typeinfo);
		RNAPI Value(const void *ptr);
		RNAPI Value(const Value *other);
		RNAPI ~Value() override;
		
		RNAPI static Value *WithVector2(const Vector2& vector);
		RNAPI static Value *WithVector3(const Vector3& vector);
		RNAPI static Value *WithVector4(const Vector4& vector);
		
		RNAPI static Value *WithColor(const Color& color);
		
		RNAPI static Value *WithQuaternion(const Quaternion& quaternion);
		RNAPI static Value *WithMatrix(const Matrix& matrix);
		
		RNAPI void GetValue(void *ptr) const;
		RNAPI void *GetPointerValue() const;
		
		template<class T>
		T GetValue() const
		{
			RN_ASSERT(typeid(T) == GetTypeInfo(), "");
			
			T temp;
			GetValue(&temp);
			
			return temp;
		}
		
		RNAPI const std::type_info& GetTypeInfo() const { return *_typeinfo; }
		
	private:
		const std::type_info *_typeinfo;
		uint8 *_buffer;
		size_t _size;
		
		RNDefineMetaWithTraits(Value, Object, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_VALUE_H__ */
