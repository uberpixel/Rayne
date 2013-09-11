//
//  RNValue.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VALUE_H__
#define __RAYNE_VALUE_H__

#include "RNBase.h"
#include "RNObject.h"

#include "RNVector.h"
#include "RNColor.h"

namespace RN
{
	class Value : public Object
	{
	public:
		Value(const void *ptr, size_t size, const std::type_info& typeinfo);
		Value(const void *ptr);
		~Value() override;
		
		static Value *WithVector2(const Vector2& vector);
		static Value *WithVector3(const Vector3& vector);
		static Value *WithVector4(const Vector4& vector);
		
		static Value *WithColor(const Color& color);
		
		void GetValue(void *ptr) const;
		void *GetPointerValue() const;
		
		template<class T>
		T GetValue() const
		{
			RN_ASSERT(typeid(T) == GetTypeInfo(), "");
			
			T temp;
			GetValue(&temp);
			
			return temp;
		}
		
		const std::type_info& GetTypeInfo() const { return *_typeinfo; }
		
	private:
		const std::type_info *_typeinfo;
		uint8 *_buffer;
		size_t _size;
		
		RNDefineMeta(Value, Object)
	};
}

#endif /* __RAYNE_VALUE_H__ */
