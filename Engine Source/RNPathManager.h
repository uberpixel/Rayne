//
//  RNPathManager.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		RNAPI static std::string Join(const std::string &path1, const std::string &path2);
		RNAPI static std::string PathByRemovingExtension(const std::string &path);
		RNAPI static std::string PathByRemovingPathComponent(const std::string &path);
		RNAPI static std::vector<std::string> PathComoponents(const std::string &path);
		RNAPI static std::string PathByJoiningComponents(const std::vector<std::string>& components);
		RNAPI static std::string Base(const std::string &path);
		RNAPI static std::string Basename(const std::string &path);
		RNAPI static std::string Basepath(const std::string &path);
		RNAPI static std::string Extension(const std::string &path);
		RNAPI static float ScaleFactorForName(const std::string &path);
		
		RNAPI static bool PathExists(const std::string &path);
		RNAPI static bool PathExists(const std::string &path, bool *isDirectory);
		RNAPI static bool CreatePath(const std::string &path, bool createIntermediateDirectories=true);
		
		RNAPI static std::string ExecutableDirectory();
		RNAPI static std::string SaveDirectory();
	};
}

#endif /* __RAYNE_PATH_H__ */
