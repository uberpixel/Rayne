//
//  RNAssetLoader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSETLOADER_H_
#define __RAYNE_ASSETLOADER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNData.h"
#include "../Objects/RNSet.h"
#include "../System/RNFile.h"

#include "RNAsset.h"

namespace RN
{
	class AssetLoader : public Object
	{
	public:
		friend class AssetCoordinator;

		RNAPI ~AssetLoader();

		RNAPI virtual Asset *Load(File *file, Dictionary *settings);
		RNAPI virtual Asset *Load(const String *name, Dictionary *settings);

		RNAPI virtual bool SupportsBackgroundLoading();
		RNAPI virtual bool SupportsLoadingFile(File *file);
		RNAPI virtual bool SupportsLoadingName(const String *name);

		RNAPI virtual uint32 GetPriority() const;

		RNAPI MetaClass *GetResourceClass() const { return _resourceClass; }

	protected:
		RNAPI AssetLoader(MetaClass *resourceClass);

		RNAPI void SetFileExtensions(const Set *extensions);
		RNAPI void SetMagicBytes(const Data *data, size_t begin);
		RNAPI void SetSupportsVirtualFiles(bool support);

	private:
		using Callback = std::function<void (Asset *, Tag)>;

		std::future<Asset *> LoadInBackground(Object *fileOrName, Dictionary *settings, Tag tag, Callback &&callback);
		Expected<Asset *> __Load(Object *fileOrName, Dictionary *settings) RN_NOEXCEPT;

		Data *_magicBytes;
		size_t _magicBytesOffset;
		Set *_fileExtensions;
		bool _supportsVirtualFiles;

		MetaClass *_resourceClass;
		RNDeclareMeta(AssetLoader)
	};
}


#endif /* __RAYNE_ASSETLOADER_H_ */
