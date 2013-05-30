//
//  RNPathManager.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PATH_H__
#define __RAYNE_PATH_H__

#include "RNBase.h"

namespace RN
{
	class PathManager
	{
	public:
		RNAPI static std::string Join(const std::string& path1, const std::string& path2);
		RNAPI static std::string PathByRemovingExtension(const std::string& path);
		RNAPI static std::vector<std::string> PathComoponents(const std::string& path);
		RNAPI static std::string Base(const std::string& path);
		RNAPI static std::string Basename(const std::string& path);
		RNAPI static std::string Basepath(const std::string& path);
		RNAPI static std::string Extension(const std::string& path);
		
		RNAPI static std::string PathForName(const std::string& name, bool strict=true);
		RNAPI static void AddSearchPath(const std::string& path);
		RNAPI static bool PathExists(const std::string& path);
		RNAPI static bool PathExists(const std::string& path, bool *isDirectory);
		RNAPI static bool CreatePath(const std::string& path, bool createIntermediateDirectories=true);
		
		RNAPI static std::string ExecutableDirectory();
		RNAPI static std::string SaveDirectory();
		
	private:
		static void AddDefaultSearchPaths();
		static void AddFileModifier(const std::string& modifier, const std::string& extension);
		static void AddFileModifier(const std::string& modifier);
		
		static std::vector<std::string> _searchPaths;
		
		static std::vector<std::string> _globalModifiers;
		static std::unordered_map<std::string, std::vector<std::string>> _fileModifiers;
	};
}

#endif /* __RAYNE_PATH_H__ */
