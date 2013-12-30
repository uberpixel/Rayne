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

namespace RN
{
	class ResourceLoader : public Object
	{
	public:
		friend class ResourceCoordinator;
	
		typedef std::function<void (Object *, Tag)> Callback;
		
		RNAPI ~ResourceLoader();
		
		RNAPI virtual Object *Load(File *file, Dictionary *settings);
		RNAPI virtual Object *Load(String *name, Dictionary *settings);
		
		RNAPI std::future<Object *> LoadInBackground(Object *fileOrName, Dictionary *settings, Tag tag, Callback callback);
		
		RNAPI virtual bool SupportsBackgroundLoading();
		RNAPI virtual bool SupportsLoadingFile(File *file);
		RNAPI virtual bool SupportsLoadingName(String *name);
		
		RNAPI virtual uint32 GetPriority() const;
		
		RNAPI MetaClassBase *GetResourceClass() const { return _resourceClass; }
		
	protected:
		RNAPI ResourceLoader(MetaClassBase *resourceClass);
		
		RNAPI void SetFileExtensions(const std::vector<std::string>& extensions);
		RNAPI void SetMagicBytes(const Data *data, size_t begin);
		RNAPI void SetSupportsImaginaryFiles(bool support);
		
	private:
		Data *_magicBytes;
		size_t _magicBytesOffset;
		std::vector<std::string> _fileExtensions;
		bool _imagianryFiles;
		
		MetaClassBase *_resourceClass;
		
		RNDefineMeta(ResourceLoader, Object)
	};
}

#endif /* __RAYNE_RESOURCELOADER_H__ */
