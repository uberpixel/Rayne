//
//  RNShaderCache.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHADERCACHE_H__
#define __RAYNE_SHADERCACHE_H__

#include "RNBase.h"
#include "RNShaderLookup.h"
#include "RNMutex.h"

namespace RN
{
	class Shader;
	class ShaderProgram;
	class Kernel;
	
	struct ShaderCacheInternal;
	
	class ShaderCache : public Singleton<ShaderCache>
	{
	public:
		friend class Shader;
		friend class Kernel;
		
		RNAPI ShaderCache();
		RNAPI ~ShaderCache() override;
		
		RNAPI void InvalidateCacheEntries(Shader *shader);
		RNAPI void CacheShaderProgram(Shader *shader, ShaderProgram *program, const ShaderLookup& lookup);
		RNAPI ShaderProgram *DequeShaderProgram(Shader *shader, const ShaderLookup& lookup);
		
		RNAPI static bool SupportsCaching();
		
	private:
		void InitializeDatabase();
		bool UpdateDatabase();
		void BumpVersion();
		
		Mutex _lock;
		PIMPL<ShaderCacheInternal> _internals;
	};
}

#endif /* __RAYNE_SHADERCACHE_H__ */
