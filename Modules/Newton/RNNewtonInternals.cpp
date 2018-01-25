//
//  RNNewtonInternals.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonInternals.h"

namespace RN
{
	void NewtonSerialization::SerializeCallback(void *serializeHandle, const void *buffer, int size)
	{
		File *file = static_cast<File*>(serializeHandle);
		file->Write(buffer, size);
	}

	void NewtonSerialization::DeserializeCallback(void *serializeHandle, void *buffer, int size)
	{
		File *file = static_cast<File*>(serializeHandle);
		file->Read(buffer, size);
	}
}
