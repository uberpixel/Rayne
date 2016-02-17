//
//  RNUnicode.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UNICODE_H__
#define __RAYNE_UNICODE_H__

#include "RNBase.h"

namespace RN
{
	typedef uint32 UniChar;
	
	class CodePoint
	{
	public:
		CodePoint() {}
		RNAPI CodePoint(UniChar character);
		RNAPI CodePoint(char character);
		
		RNAPI static UniChar ConvertCharacter(char character);
		RNAPI static char ConverToCharacter(UniChar character);
		
		static UniChar Ellipsis() { return 0x2026; }
		static CodePoint ASCIITerminator() { return CodePoint(char(0x7f)); }
		
		RNAPI char GetASCIICharacter() const;
		UniChar GetCharacter() const { return _codePoint; }
		
		RNAPI bool IsNewline() const;
		RNAPI bool IsWhitespace() const;
		RNAPI bool IsPrintable() const;
		
		RNAPI UniChar GetUpperCase() const;
		RNAPI UniChar GetLowerCase() const;
		
		bool operator ==(const CodePoint &other) const
		{
			return (_codePoint == other._codePoint);
		}
		
		bool operator >(const CodePoint &other) const
		{
			return (_codePoint > other._codePoint);
		}
		
		bool operator <(const CodePoint &other) const
		{
			return (_codePoint < other._codePoint);
		}
		
		operator uint32() const
		{
			return static_cast<uint32>(_codePoint);
		}
		
	private:
		UniChar _codePoint;
	};
	
	enum class Encoding
	{
		ASCII,
		UTF8,
		UTF16LE,
		UTF16BE,
		UTF32,
		
		UTF16 = UTF16LE
	};
}

#endif /* __RAYNE_UNICODE_H__ */
