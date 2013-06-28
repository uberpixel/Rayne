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
		char c = static_cast<char>(character);
		if(c > 0x7f)
			c = '?';
		
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
	
	UniChar CodePoint::LowerCase() const
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
	
	UniChar CodePoint::UpperCase() const
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
