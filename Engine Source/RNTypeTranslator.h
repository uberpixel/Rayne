//
//  RNTypeTranslator.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TYPETRANSLATOR_H__
#define __RAYNE_TYPETRANSLATOR_H__

#include "RNBase.h"

namespace RN
{
	class Object;
	
	template<class T>
	struct TypeTranslator  : public std::integral_constant<char, '?'>
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
}

#endif /* __RAYNE_TYPETRANSLATOR_H__ */
