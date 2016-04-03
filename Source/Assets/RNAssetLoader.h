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
		struct Config
		{
			friend class AssetLoader;

			Config(MetaClass *resourceClass) :
				_extensions(nullptr),
				_magicBytes(nullptr),
				_magicBytesOffset(0),
				resourceClasses({ resourceClass }),
				supportsBackgroundLoading(false),
				supportsVirtualFiles(false),
				priority(10)
			{}

			Config(const std::initializer_list<MetaClass *> &tresourceClasses) :
				_extensions(nullptr),
				_magicBytes(nullptr),
				_magicBytesOffset(0),
				resourceClasses(tresourceClasses),
				supportsBackgroundLoading(false),
				supportsVirtualFiles(false),
				priority(10)
			{}

			~Config()
			{
				SafeRelease(_extensions);
				SafeRelease(_magicBytes);
			}

			void SetExtensions(const Set *extensions)
			{
				SafeRelease(_extensions);
				_extensions = SafeCopy(extensions);
			}

			void SetMagicBytes(const Data *bytes, size_t offset)
			{
				SafeRelease(_magicBytes);

				_magicBytes = SafeCopy(bytes);
				_magicBytesOffset = offset;
			}

			const Set *GetExtensions() const { return _extensions; }
			const Data *GetMagicBytes() const { return _magicBytes; }
			size_t GetMagicBytesOffset() const { return _magicBytesOffset; }

			std::vector<MetaClass *> resourceClasses;

			bool supportsBackgroundLoading;
			bool supportsVirtualFiles;

			uint32 priority;

		private:
			Set *_extensions;

			Data *_magicBytes;
			size_t _magicBytesOffset;

		};

		friend class AssetManager;

		RNAPI ~AssetLoader();

		RNAPI virtual Asset *Load(File *file, MetaClass *meta, Dictionary *settings);
		RNAPI virtual Asset *Load(const String *name, MetaClass *meta, Dictionary *settings);

		RNAPI virtual bool SupportsLoadingFile(File *file) const;
		RNAPI virtual bool SupportsLoadingName(const String *name) const;
		RNAPI bool SupportsResourceClass(MetaClass *meta) const;

		uint32 GetPriority() const { return _priority; }
		const std::vector<MetaClass *> &GetResourceClasses() const { return _resourceClasses; }

	protected:
		RNAPI AssetLoader(const Config &config);

	private:
		using Callback = std::function<void (Asset *)>;

		std::future<StrongRef<Asset>> LoadInBackground(Object *fileOrName, MetaClass *meta, Dictionary *settings, Callback &&callback);
		Expected<Asset *> __Load(Object *fileOrName, MetaClass *meta, Dictionary *settings) RN_NOEXCEPT;

		Data *_magicBytes;
		size_t _magicBytesOffset;
		Set *_fileExtensions;

		uint32 _priority;
		bool _supportsBackgroundLoading;
		bool _supportsVirtualFiles;

		std::vector<MetaClass *> _resourceClasses;
		__RNDeclareMetaInternal(AssetLoader)
	};
}


#endif /* __RAYNE_ASSETLOADER_H_ */
