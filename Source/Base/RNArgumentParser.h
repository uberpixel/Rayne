//
//  RNArgumentParser.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ARGUMENTPARSER_H_
#define __RAYNE_ARGUMENTPARSER_H_

#include "RNBase.h"
#include "../Data/RNAny.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"

namespace RN
{
	class ArgumentParser
	{
	public:
		struct Argument
		{
		public:
			Argument(String *argument, String *value) :
				_argument(argument),
				_value(value)
			{}

			String *GetArgument() const RN_NOEXCEPT { return _argument; }
			String *GetValue() const RN_NOEXCEPT { return _value; }

		private:
			String *_argument;
			String *_value;
		};

		RNAPI ArgumentParser(int argc, const char *argv[]);
		RNAPI ~ArgumentParser();

		RNAPI Argument ParseArgument(const char *full, char shorthand) const;

		RNAPI bool HasArgument(const char *full, char shorthand) const RN_NOEXCEPT;
		RNAPI bool HasArgumentAndValue(const char *full, char shorthand) const RN_NOEXCEPT;

		size_t GetCount() const RN_NOEXCEPT { return _arguments->GetCount(); }
		const String *GetArgumentAtIndex(size_t index) const { return _arguments->GetObjectAtIndex<String>(index); }

	private:
		Array *_arguments;
	};

	RNExceptionType(ArgumentNotFound)
}


#endif /* __RAYNE_ARGUMENTPARSER_H_ */
