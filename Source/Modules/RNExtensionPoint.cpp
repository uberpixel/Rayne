//
//  RNExtensionPoint.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNExtensionPoint.h"

namespace RN
{
	static SpinLock __pointLock;
	static std::unordered_map<std::string, __ExtensionPointBase *> __pointStorage;

	void __ExtensionPointBase::InitializeExtensionPoints()
	{
		__pointStorage.clear();
	}
	void __ExtensionPointBase::TeardownExtensionPoints()
	{
		__pointStorage.clear();
	}


	void __ExtensionPointBase::InstallExtensionPoint(__ExtensionPointBase *point)
	{
		LockGuard<SpinLock> lock(__pointLock);
		__pointStorage.emplace(point->GetName(), point);
	}

	void __ExtensionPointBase::RemoveExtensionPoint(__ExtensionPointBase *point)
	{
		LockGuard<SpinLock> lock(__pointLock);
		auto iterator = __pointStorage.find(point->GetName());

		if(iterator != __pointStorage.end() && iterator->second == point)
			__pointStorage.erase(iterator);
	}

	__ExtensionPointBase *__ExtensionPointBase::GetExtensionPoint(const std::string &name)
	{
		LockGuard<SpinLock> lock(__pointLock);
		auto iterator = __pointStorage.find(name);

		return (iterator == __pointStorage.end()) ? nullptr : iterator->second;
	}
}
