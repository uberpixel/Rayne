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
		
		enum class Encoding
		{
			ASCII,
			UTF8,
			UTF16LE,
			UTF16BE
		};
		
		String();
		String(const char *string);
		String(const char *string, size_t length);
		String(const void *bytes, Encoding encoding);
		String(const void *bytes, size_t length, Encoding encoding);
		String(const String& string);
		String(const String *string);
		virtual ~String();
		
		static String *WithString(const char *string);
		static String *WithString(const char *string, size_t length);
		static String *WithBytes(const void *bytes, Encoding encoding);
		static String *WithBytes(const void *bytes, size_t length, Encoding encoding);
		
		machine_hash Hash() const override;
		bool IsEqual(Object *other) const override;
		
		bool operator ==(const String& other) const;
		bool operator !=(const String& other) const;
		
		String& operator +=(const String& other);
		String operator +(const String& other) const;
		
		void Append(const String& string);
		void Append(const String *string);
		
		void Insert(const String& string, uint32 index);
		void Insert(const String *string, uint32 index);
		
		void DeleteCharacters(const Range& range);
		
		void ReplaceCharacters(const String& replacement, const Range& range);
		void ReplaceCharacters(const String *replacement, const Range& range);
		
		void ReplaceOccurrencesOfString(const String& string, const String& replacement);
		void ReplaceOccurrencesOfString(const String *string, const String *replacement);
		
		Range RangeOfString(const String& string, ComparisonMode mode=0);
		Range RangeOfString(const String& string, ComparisonMode mode, const Range& range);
		
		Range RangeOfString(const String *string, ComparisonMode mode=0);
		Range RangeOfString(const String *string, ComparisonMode mode, const Range& range);
		
		ComparisonResult Compare(const String& other, ComparisonMode mode=0) const;
		ComparisonResult Compare(const String& other, ComparisonMode mode, const Range& range) const;
		
		ComparisonResult Compare(const String *other, ComparisonMode mode=0) const;
		ComparisonResult Compare(const String *other, ComparisonMode mode, const Range& range) const;
		
		String Substring(const Range& range) const;
		UniChar CharacterAtIndex(uint32 index) const;
		
		uint32 Length() const { return _length; }
		
		uint8 *BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const;
		char *UTF8String() const { return reinterpret_cast<char *>(BytesWithEncoding(Encoding::UTF8, false, nullptr)); }
		
	private:
		void Initialize();
		void AllocateBuffer(size_t size);
		void CheckAndExpandBuffer(uint32 minium);
		
		bool IsLegalUTF8(const uint8 *sequence, int length) const;
		
		
		size_t UTF8ByteLength(const uint8 *bytes) const;
		void CopyUTF8Bytes(const uint8 *bytes, size_t length);
		void CopyBytesWithEncoding(const void *bytes, size_t length, Encoding encoding);

		uint8 *_buffer;
		uint32 _length;
		uint32 _size;
		uint32 _occupied;
		
		RNDefineMetaWithTraits(String, Object, MetaClassTraitCronstructable)
	};
}

#define RNSTR(cstr) RN::String::WithString(cstr)

#endif /* __RAYNE_STRING_H__ */
