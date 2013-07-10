//
//  RNString.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STRING_H__
#define __RAYNE_STRING_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNUnicode.h"

namespace RN
{
	class String : public Object
	{
	public:
		enum
		{
			ComparisonModeCaseInsensitive = (1 << 0),
			ComparisonModeNumerically = (1 << 1),
			ComparisonModeBackwards = (1 << 2)
		};
		typedef uint32 ComparisonMode;
		
		String();
		String(const char *string, va_list args);
		String(const char *string, bool constant=false);
		String(const char *string, size_t length, bool constant=false);
		String(const void *bytes, Encoding encoding, bool constant=false);
		String(const void *bytes, size_t length, Encoding encoding, bool constant=false);
		String(const String *string);
		~String() override;
		
		static String *WithFormat(const char *string, ...);
		static String *WithString(const char *string, bool constant=false);
		static String *WithString(const char *string, size_t length, bool constant=false);
		static String *WithBytes(const void *bytes, Encoding encoding, bool constant=false);
		static String *WithBytes(const void *bytes, size_t length, Encoding encoding, bool constant=false);
		
		machine_hash Hash() const override;
		bool IsEqual(Object *other) const override;
		
		void Append(const String *string);
		void Append(const char *string, ...);
		void Insert(const String *string, size_t index);
		
		void DeleteCharacters(const Range& range);
		
		void ReplaceCharacters(const String *replacement, const Range& range);
		void ReplaceOccurrencesOfString(const String *string, const String *replacement);
		
		Range RangeOfString(const String *string, ComparisonMode mode=0);
		Range RangeOfString(const String *string, ComparisonMode mode, const Range& range);
		
		ComparisonResult Compare(const String *other, ComparisonMode mode=0) const;
		ComparisonResult Compare(const String *other, ComparisonMode mode, const Range& range) const;
		
		String *Substring(const Range& range) const;
		UniChar CharacterAtIndex(size_t index) const;
		
		size_t Length() const;
		
		uint8 *BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const;
		char *UTF8String() const;
		
	private:
		String(void *internal);
		void PromoteStringIfNeeded(Encoding encoding);
		
		Encoding _encoding;
		void *_internal; // Of abstract type BasicString
		
		RNDefineMetaWithTraits(String, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#define RNSTR(...)  RN::String::WithFormat(__VA_ARGS__)
#define RNCSTR(cstr) RN::String::WithString(cstr, true)

#define RNUTF8STR(str)  RN::String::WithBytes(str, Encoding::UTF8)
#define RNCUTF8STR(str) RN::String::WithBytes(str, Encoding::UTF8, true)

#endif /* __RAYNE_STRING_H__ */
