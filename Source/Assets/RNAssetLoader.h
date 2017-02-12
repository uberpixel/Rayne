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
#include "../Threads/RNWorkQueue.h"

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
				resourceClasses({ resourceClass }),
				supportsBackgroundLoading(false),
				supportsVirtualFiles(false),
				priority(10),
				_extensions(nullptr),
				_magicBytes(nullptr),
				_magicBytesOffset(0)
			{}

			Config(const std::initializer_list<MetaClass *> &tresourceClasses) :
				resourceClasses(tresourceClasses),
				supportsBackgroundLoading(false),
				supportsVirtualFiles(false),
				priority(10),
				_extensions(nullptr),
				_magicBytes(nullptr),
				_magicBytesOffset(0)
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

		struct LoadOptions
		{
			MetaClass *meta;
			StrongRef<Dictionary> settings;
			WorkQueue *queue;
		};

		friend class AssetManager;

		RNAPI ~AssetLoader();

		RNAPI virtual Asset *Load(File *file, const LoadOptions &options);
		RNAPI virtual Asset *Load(const String *name, const LoadOptions &options);

		RNAPI virtual bool SupportsLoadingFile(File *file) const;
		RNAPI virtual bool SupportsLoadingName(const String *name) const;
		RNAPI bool SupportsResourceClass(MetaClass *meta) const;

		uint32 GetPriority() const { return _priority; }
		const std::vector<MetaClass *> &GetResourceClasses() const { return _resourceClasses; }

	protected:
		RNAPI AssetLoader(const Config &config);

	private:
		using Callback = std::function<void (Asset *)>;

		void __LoadInBackground(Object *fileOrName, const LoadOptions &options, void *token);
		Expected<Asset *> __Load(Object *fileOrName, const LoadOptions &options) RN_NOEXCEPT;

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
