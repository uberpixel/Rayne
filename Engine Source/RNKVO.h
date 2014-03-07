//
//  RNKVO.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVO_H__
#define __RAYNE_KVO_H__

#include "RNBase.h"
#include "RNSignal.h"
#include "RNMatrixQuaternion.h"
#include "RNVector.h"
#include "RNColor.h"

#define kRNObservableNewValueKey RNCSTR("kRNObservableNewValueKey")
#define kRNObservableOldValueKey RNCSTR("kRNObservableOldValueKey")

namespace RN
{
	class Object;
	class Dictionary;
	
	template<class T>
	struct TypeTranslator
	{};
	
	template<>
	struct TypeTranslator<bool> : public std::integral_constant<char, 'B'>
	{};
	
	template<>
	struct TypeTranslator<int8> : public std::integral_constant<char, 'b'>
	{};
	template<>
	struct TypeTranslator<int16> : public std::integral_constant<char, 's'>
	{};
	template<>
	struct TypeTranslator<int32> : public std::integral_constant<char, 'i'>
	{};
	template<>
	struct TypeTranslator<int64> : public std::integral_constant<char, 'l'>
	{};
	
	template<>
	struct TypeTranslator<uint8> : public std::integral_constant<char, '_'>
	{};
	template<>
	struct TypeTranslator<uint16> : public std::integral_constant<char, 'S'>
	{};
	template<>
	struct TypeTranslator<uint32> : public std::integral_constant<char, 'I'>
	{};
	template<>
	struct TypeTranslator<uint64> : public std::integral_constant<char, 'L'>
	{};
	
	template<>
	struct TypeTranslator<float> : public std::integral_constant<char, 'f'>
	{};
	template<>
	struct TypeTranslator<double> : public std::integral_constant<char, 'd'>
	{};
	
	template<>
	struct TypeTranslator<Vector2> : public std::integral_constant<char, '2'>
	{};
	template<>
	struct TypeTranslator<Vector3> : public std::integral_constant<char, '3'>
	{};
	template<>
	struct TypeTranslator<Vector4> : public std::integral_constant<char, '4'>
	{};
	template<>
	struct TypeTranslator<Color> : public std::integral_constant<char, 'c'>
	{};
	template<>
	struct TypeTranslator<Matrix> : public std::integral_constant<char, 'm'>
	{};
	template<>
	struct TypeTranslator<Quaternion> : public std::integral_constant<char, 'q'>
	{};
	
	template<>
	struct TypeTranslator<Object *> : public std::integral_constant<char, '@'>
	{};
	
	
	class ObservableProperty
	{
	public:
		friend class Object;
		
		RNAPI virtual ~ObservableProperty();
	
		std::string GetName() const { return _name; }
		char GetType() const { return _type; }
		
		RNAPI virtual void SetValue(Object *value) = 0;
		RNAPI virtual Object *GetValue() const = 0;
		
		RNAPI void SetWritable(bool writable);
		bool IsWritable() const { return _flags & (1 << 8); }
		
		RNAPI void WillChangeValue();
		RNAPI void DidChangeValue();
		
	protected:
		RNAPI ObservableProperty(const char *name, char type);
		
		Object *_object;
		
	private:
		RNAPI void AssertSignal();
		
		char _type;
		char _name[33];
		uint8 _flags;
		
		Signal<void (Object *, const std::string &, Dictionary *)> *_signal;
		Dictionary *_changeSet;
		void *_opaque;
	};
}

#endif /* __RAYNE_KVO_H__ */
