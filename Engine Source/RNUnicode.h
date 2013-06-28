//
//  RNUnicode.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		CodePoint(UniChar character);
		CodePoint(char character);
		
		static UniChar ConvertCharacter(char character);
		static char ConverToCharacter(UniChar character);
		static UniChar Ellipsis() { return 0x2026; }
		
		char ASCIICharacter() const;
		UniChar Character() const { return _codePoint; }
		
		bool IsNewline() const;
		
		UniChar UpperCase() const;
		UniChar LowerCase() const;
		
		bool operator ==(const CodePoint& other) const
		{
			return (_codePoint == other._codePoint);
		}
		
		bool operator >(const CodePoint& other) const
		{
			return (_codePoint > other._codePoint);
		}
		
		bool operator <(const CodePoint& other) const
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
}

#endif /* __RAYNE_UNICODE_H__ */
