//
//  RNResourceLoader.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESOURCELOADER_H__
#define __RAYNE_RESOURCELOADER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNFile.h"
#include "RNData.h"
#include "RNDictionary.h"
#include "RNString.h"
#include "RNAsset.h"

namespace RN
{
	class ResourceLoader : public Object
	{
	public:
		friend class ResourceCoordinator;
	
		typedef std::function<void (Asset *, Tag)> Callback;
		
		RNAPI ~ResourceLoader();
		
		RNAPI virtual Asset *Load(File *file, Dictionary *settings);
		RNAPI virtual Asset *Load(String *name, Dictionary *settings);
		
		RNAPI virtual bool SupportsBackgroundLoading();
		RNAPI virtual bool SupportsLoadingFile(File *file);
		RNAPI virtual bool SupportsLoadingName(String *name);
		
		RNAPI virtual uint32 GetPriority() const;
		
		RNAPI MetaClass *GetResourceClass() const { return _resourceClass; }
		
	protected:
		RNAPI ResourceLoader(MetaClass *resourceClass);
		
		RNAPI void SetFileExtensions(const std::vector<std::string>& extensions);
		RNAPI void SetMagicBytes(const Data *data, size_t begin);
		RNAPI void SetSupportsImaginaryFiles(bool support);
		
	private:
		std::future<Asset *> LoadInBackground(Object *fileOrName, Dictionary *settings, Tag tag, Callback callback);
		
		Data *_magicBytes;
		size_t _magicBytesOffset;
		std::vector<std::string> _fileExtensions;
		bool _imagianryFiles;
		
		MetaClass *_resourceClass;
		
		RNDeclareMeta(ResourceLoader)
	};
}

#endif /* __RAYNE_RESOURCELOADER_H__ */
