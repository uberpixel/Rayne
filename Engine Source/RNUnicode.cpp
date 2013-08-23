//
//  RNUnicode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUnicode.h"

namespace RN
{
	CodePoint::CodePoint(UniChar character)
	{
		_codePoint = character;
	}
	
	CodePoint::CodePoint(char character)
	{
		_codePoint = static_cast<UniChar>(character);
	}
	
	
	
	UniChar CodePoint::ConvertCharacter(char character)
	{
		return static_cast<UniChar>(character);
	}
	
	char CodePoint::ConverToCharacter(UniChar character)
	{
		if(character >= 0x7f)
			return '?';
		
		char c = static_cast<char>(character);
		return c;
	}
	
	
	bool CodePoint::IsNewline() const
	{
		switch(_codePoint)
		{
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0d:
			case 0x85:
			case 0x2028:
			case 0x2029:
				return true;
				
			default:
				return false;
		}
	}
	
	bool CodePoint::IsWhitespace() const
	{
		switch(_codePoint)
		{
			case 0x20:
			case 0xA0:
			case 0x1680:
			case 0x180E:
			case 0x2000:
			case 0x2001:
			case 0x2002:
			case 0x2003:
			case 0x2004:
			case 0x2005:
			case 0x2006:
			case 0x2007:
			case 0x2008:
			case 0x2009:
			case 0x200A:
			case 0x202F:
			case 0x205F:
			case 0x3000:
				return true;
				
			default:
				return false;
		}
	}
	
	UniChar CodePoint::GetLowerCase() const
	{
		if(_codePoint <= 0x7f)
		{
			char character = static_cast<char>(_codePoint);
			if(character >= 'A' && character <= 'Z')
			{
				character = tolower(character);
				return CodePoint(character);
			}
		}
		
		return *this;
	}
	
	UniChar CodePoint::GetUpperCase() const
	{
		if(_codePoint <= 0x7f)
		{
			char character = static_cast<char>(_codePoint);
			if(character >= 'a' && character <= 'z')
			{
				character = toupper(character);
				return CodePoint(character);
			}
		}
		
		return *this;
	}
}
