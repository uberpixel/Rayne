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

namespace RN
{
	typedef uint32 UniChar;
	
	class String : public Object
	{
	public:
		enum
		{
			ComparisonModeCaseInsensitive = (1 << 0),
			ComparisonModeNumerically = (1 << 1)
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
		virtual ~String();
		
		bool operator ==(const String& other) const;
		bool operator !=(const String& other) const;
		
		String& operator +=(const String& other);
		String operator +(const String& other) const;
		
		void Append(const char *string);
		void Append(const char *string, size_t length);
		void Append(const void *bytes, Encoding encoding);
		void Append(const void *bytes, size_t length, Encoding encoding);
		void Append(const String& string);
		
		Range RangeOfString(const String& string);
		Range RangeOfString(const String& string, ComparisonMode mode);
		Range RangeOfString(const String& string, ComparisonMode mode, const Range& range);
		
		ComparisonResult Compare(const String& other) const;
		ComparisonResult Compare(const String& other, ComparisonMode mode) const;
		ComparisonResult Compare(const String& other, const Range& range, ComparisonMode mode) const;
		
		String Substring(const Range& range) const;
		UniChar CharacterAtIndex(uint32 index) const;
		
		uint32 Length() const { return _length; }
		
		uint8 *BytesWithEncoding(Encoding encoding, bool lossy, size_t *length) const;
		
		void DebugDump() const;
		
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
		
		RNDefineMeta(String, Object)
	};
}

#endif /* __RAYNE_STRING_H__ */
