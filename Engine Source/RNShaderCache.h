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
		
		ShaderCache();
		~ShaderCache() override;
		
		void InvalidateCacheEntries(Shader *shader);
		void CacheShaderProgram(Shader *shader, ShaderProgram *program, const ShaderLookup& lookup);
		ShaderProgram *DequeShaderProgram(Shader *shader, const ShaderLookup& lookup);
		
		static bool SupportsCaching();
		
	private:
		void InitializeDatabase();
		bool UpdateDatabase();
		void BumpVersion();
		
		Mutex _lock;
		PIMPL<ShaderCacheInternal> _internals;
	};
}

#endif /* __RAYNE_SHADERCACHE_H__ */
