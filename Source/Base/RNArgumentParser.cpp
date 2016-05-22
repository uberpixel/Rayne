//
//  RNArgumentParser.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNArgumentParser.h"

namespace RN
{
	RNExceptionImp(ArgumentNotFound)

	ArgumentParser::ArgumentParser(int argc, const char *argv[])
	{
		_arguments = new Array();

		for(int i = 0; i < argc; i ++)
		{
			String *string = new String(argv[i], Encoding::ASCII, false);
			_arguments->AddObject(string);
			string->Release();
		}
	}

	ArgumentParser::~ArgumentParser()
	{
		_arguments->Release();
	}


	ArgumentParser::Argument ArgumentParser::ParseArgument(const char *full, char shorthand) const
	{
		if(!full && shorthand == '\0')
			throw ArgumentNotFoundException("No such argument");

		if(full)
		{
			String *prefix = new String("--", Encoding::ASCII, true);
			String *suffix = new String(full, Encoding::ASCII, false);

			ScopeGuard guard([=]{
				prefix->Release();
				suffix->Release();
			});

			size_t count = _arguments->GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				String *value = _arguments->GetObjectAtIndex<String>(i);
				if(value->GetRangeOfString(prefix).origin == 0 && value->GetLength() == suffix->GetLength() + 2)
				{
					Range range = value->GetRangeOfString(suffix);
					if(range.origin == 2)
					{
						if(i == count - 1)
							throw ArgumentNotFoundException("No value provided for argument");

						return Argument(value, _arguments->GetObjectAtIndex<String>(i + 1));
					}
				}
			}
		}

		if(shorthand != '\0')
		{
			char string[2] = { 0 };
			string[0] = shorthand;

			String *truePrefix = new String("-", Encoding::ASCII, true);
			String *falsePrefix = new String("--", Encoding::ASCII, true);
			CharacterSet *set = new CharacterSet(string);

			ScopeGuard guard([=]{
				truePrefix->Release();
				falsePrefix->Release();
				set->Release();
			});

			size_t count = _arguments->GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				String *value = _arguments->GetObjectAtIndex<String>(i);
				if(value->GetRangeOfString(truePrefix).origin == 0 && value->GetRangeOfString(falsePrefix).origin != 0)
				{
					Range range = value->GetRangeOfCharacterInSet(set);
					if(range.origin != kRNNotFound)
					{
						if(i == count - 1)
							throw ArgumentNotFoundException("No value provided for argument");

						return Argument(value, _arguments->GetObjectAtIndex<String>(i + 1));
					}
				}
			}
		}

		throw ArgumentNotFoundException("No such argument");
	}

	bool ArgumentParser::HasArgument(const char *full, char shorthand) const RN_NOEXCEPT
	{
		if(!full && shorthand == '\0')
			return false;

		if(full)
		{
			String *prefix = new String("--", Encoding::ASCII, true);
			String *suffix = new String(full, Encoding::ASCII, false);

			ScopeGuard guard([=]{
				prefix->Release();
				suffix->Release();
			});

			size_t count = _arguments->GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				String *value = _arguments->GetObjectAtIndex<String>(i);
				if(value->GetRangeOfString(prefix).origin == 0 && value->GetLength() == suffix->GetLength() + 2)
				{
					Range range = value->GetRangeOfString(suffix);

					if(range.origin == 2)
						return true;
				}
			}
		}

		if(shorthand != '\0')
		{
			char string[2] = { 0 };
			string[0] = shorthand;

			String *truePrefix = new String("-", Encoding::ASCII, true);
			String *falsePrefix = new String("--", Encoding::ASCII, true);
			CharacterSet *set = new CharacterSet(string);

			ScopeGuard guard([=]{
				truePrefix->Release();
				falsePrefix->Release();
				set->Release();
			});

			size_t count = _arguments->GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				String *value = _arguments->GetObjectAtIndex<String>(i);
				if(value->GetRangeOfString(truePrefix).origin == 0 && value->GetRangeOfString(falsePrefix).origin != 0)
				{
					Range range = value->GetRangeOfCharacterInSet(set);
					if(range.origin != kRNNotFound)
						return true;
				}
			}
		}

		return false;
	}

	bool ArgumentParser::HasArgumentAndValue(const char *full, char shorthand) const RN_NOEXCEPT
	{
		if(!full && shorthand == '\0')
			return false;

		if(full)
		{
			String *prefix = new String("--", Encoding::ASCII, true);
			String *suffix = new String(full, Encoding::ASCII, false);

			ScopeGuard guard([=]{
				prefix->Release();
				suffix->Release();
			});

			size_t count = _arguments->GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				String *value = _arguments->GetObjectAtIndex<String>(i);
				if(value->GetRangeOfString(prefix).origin == 0 && value->GetLength() == suffix->GetLength() + 2)
				{
					Range range = value->GetRangeOfString(suffix);

					if(range.origin == 2)
						return !(i == count - 1);
				}
			}
		}

		if(shorthand != '\0')
		{
			char string[2] = { 0 };
			string[0] = shorthand;

			String *truePrefix = new String("-", Encoding::ASCII, true);
			String *falsePrefix = new String("--", Encoding::ASCII, true);
			CharacterSet *set = new CharacterSet(string);

			ScopeGuard guard([=]{
				truePrefix->Release();
				falsePrefix->Release();
				set->Release();
			});

			size_t count = _arguments->GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				String *value = _arguments->GetObjectAtIndex<String>(i);
				if(value->GetRangeOfString(truePrefix).origin == 0 && value->GetRangeOfString(falsePrefix).origin != 0)
				{
					Range range = value->GetRangeOfCharacterInSet(set);
					if(range.origin != kRNNotFound)
						return !(i == count - 1);
				}
			}
		}

		return false;
	}
}
