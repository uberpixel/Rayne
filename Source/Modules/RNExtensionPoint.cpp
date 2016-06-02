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
	static Lockable __pointLock;
	static Dictionary *__pointStorage = nullptr;

	static void InstallStorage()
	{
		if(RN_EXPECT_FALSE(!__pointStorage))
			__pointStorage = new Dictionary();
	}



	void __ExtensionPointBase::InitializeExtensionPoints()
	{
		InstallStorage();
	}
	void __ExtensionPointBase::TeardownExtensionPoints()
	{
		SafeRelease(__pointStorage);
	}


	void __ExtensionPointBase::InstallExtensionPoint(__ExtensionPointBase *point)
	{
		LockGuard<Lockable> lock(__pointLock);

		InstallStorage();

		String *name = new String(point->GetName().c_str());
		__pointStorage->SetObjectForKey(point, name);
		name->Release();
	}
	void __ExtensionPointBase::RemoveExtensionPoint(__ExtensionPointBase *point)
	{
		LockGuard<Lockable> lock(__pointLock);

		InstallStorage();

		String *name = new String(point->GetName().c_str());
		__pointStorage->RemoveObjectForKey(name);
		name->Release();
	}


	__ExtensionPointBase *__ExtensionPointBase::GetExtensionPoint(const std::string &tname)
	{
		LockGuard<Lockable> lock(__pointLock);

		InstallStorage();

		String *name = new String(tname.c_str());
		__ExtensionPointBase *result = static_cast<__ExtensionPointBase *>(__pointStorage->GetObjectForKey(name));
		name->Release();

		return result;
	}
}
