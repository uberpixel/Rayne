//
//  RNPathManager.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBaseInternal.h"
#include "RNPathManager.h"
#include "RNApplication.h"
#include "RNFile.h"
#include "RNKernel.h"

#if RN_PLATFORM_POSIX
	#define RNIsPathDelimiter(c) (c == '/')
	#define RNPathDelimiter '/'
	#define RNPathDelimiterTokens "/"
#endif

#if RN_PLATFORM_WINDOWS
	#define RNIsPathDelimiter(c) (c == '/' || c == '\\')
	#define RNPathDelimiter '\\'
	#define RNPathDelimiterTokens "/\\"
#endif

namespace RN
{
	std::string PathManager::Join(const std::string &path1, const std::string &path2)
	{
		std::string path = std::move(Basepath(path1));
		std::stringstream result;
		
		bool endSeperator   = RNIsPathDelimiter(path[path.size() - 1]);
		bool startSeperator = RNIsPathDelimiter(path2[0]);
		
		if(endSeperator && startSeperator)
		{
			result << path << path2.substr(1);
		}
		else if(endSeperator || startSeperator)
		{
			result << path << path2;
		}
		else
		{
			result << path << RNPathDelimiter << path2;
		}
		
		return result.str();
	}
	
	std::string PathManager::PathByRemovingExtension(const std::string &path)
	{
		size_t i = path.size();
		while((i --) > 0)
		{
			if(path[i] == '.')
				return path.substr(0, i);
		}
			
		return path;
	}
	
	std::string PathManager::PathByRemovingPathComponent(const std::string &path)
	{
		bool hasMarker = false;
		
		size_t marker = path.size();
		size_t i = marker;
		
		while((-- i) > 0)
		{
			if(RNIsPathDelimiter(path[i]))
			{
				marker = i;
				hasMarker = true;
				break;
			}
		}
		
		return hasMarker ? path.substr(0, marker) : path;
	}
	
	std::vector<std::string> PathManager::PathComoponents(const std::string &path)
	{
		const char *cstr = path.c_str();
		std::vector<std::string> result;
		
		while(RNIsPathDelimiter(*cstr))
			cstr ++;
		
		while(1)
		{
			const char *end = strpbrk(cstr, RNPathDelimiterTokens);
			if(!end)
			{
				if(strlen(cstr) > 0)
					result.push_back(cstr);
				
				break;
			}
			
			result.emplace_back(std::string(cstr, end - cstr));
			cstr = end + 1;
		}
		
		return result;
	}
	
	std::string PathManager::PathByJoiningComponents(const std::vector<std::string>& components)
	{
		std::stringstream result;
		
		for(const std::string &component : components)
		{
			result << component;
			result << RNPathDelimiter;
		}
		
		return result.str();
	}
	
	std::string PathManager::Base(const std::string &path)
	{
		size_t marker = path.size();
		size_t i = marker;
		
		while((i --) > 0)
		{
			if(RNIsPathDelimiter(path[i]))
			{
				i ++;
				break;
			}
		}
		
		return path.substr(i, marker - i);
	}
	
	std::string PathManager::Basename(const std::string &path)
	{
		bool hasExtension = false;
		
		size_t marker = path.size();
		size_t i = marker;
		
		while((-- i) > 0)
		{
			if(!hasExtension && path[i] == '.')
			{
				marker = i;
				hasExtension = true;
			}
			
			if(RNIsPathDelimiter(path[i]))
			{
				i ++;
				break;
			}
		}
		
		return path.substr(i, marker - i);
	}
	
	std::string PathManager::Basepath(const std::string &path)
	{
		bool hasExtension = false;
		
		size_t marker = path.size();
		size_t i = marker;
		
		while((-- i) > 0)
		{
			if(RNIsPathDelimiter(path[i]))
			{
				marker = i;
				break;
			}
			
			if(path[i] == '.')
				hasExtension = true;
		}
		
		return hasExtension ? path.substr(0, marker) : path;
	}
	
	std::string PathManager::Extension(const std::string &path)
	{
		size_t marker = path.size();
		size_t i = marker;
		
		while((i --) > 0)
		{
			if(path[i] == '.')
			{
				marker = i + 1;
				break;
			}
		}
		
		return path.substr(marker);
	}
	
	float PathManager::ScaleFactorForName(const std::string &path)
	{
		std::string base = Basename(path);
		return (base.find("@2x") != std::string::npos) ? 2.0f : 1.0f;
	}
	
	std::string Normalize(const std::string &path)
	{
		std::string result(path);
		
		size_t i = path.size();
		
		while((-- i) > 0)
		{
			if(RNIsPathDelimiter(result[i]))
				result[i] = '/';
		}
		
		return result;
	}
	
	
	bool PathManager::PathExists(const std::string &path)
	{
		return PathExists(path, 0);
	}
	
	bool PathManager::PathExists(const std::string &path, bool *isDirectory)
	{
#if RN_PLATFORM_POSIX
		struct stat buf;
		int result = stat(path.c_str(), &buf);
		
		if(result != 0)
			return false;
		
		if(isDirectory)
			*isDirectory = S_ISDIR(buf.st_mode);
		
		return true;
#endif
		
#if RN_PLATFORM_WINDOWS
		DWORD attributes = ::GetFileAttributes(path.c_str());
		
		if(attributes == INVALID_FILE_ATTRIBUTES)
			return false;
		
		if(isDirectory)
			*isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY);
		
		return true;
#endif
	}
	
	static inline bool MakeDirectory(const char *path)
	{
#if RN_PLATFORM_POSIX
		struct stat buf;
		if(stat(path, &buf) != 0)
		{
			if(mkdir(path, 0777) != 0 && errno != EEXIST)
				return false;
		}
		else if(!S_ISDIR(buf.st_mode))
		{
			errno = ENOTDIR;
			return false;
		}
		
		return true;
#endif
		
#if RN_PLATFORM_WINDOWS
		bool isDirectory;
		if(PathManager::PathExists(path, &isDirectory))
			return isDirectory;
		
		return ::CreateDirectory(path, NULL);
#endif
	}
	
	bool PathManager::CreatePath(const std::string &tpath, bool createIntermediateDirectories)
	{
		bool isDirectory;
		
		if(PathExists(tpath, &isDirectory))
			return isDirectory;
		
		char *path = new char[tpath.length() + 1];
		strcpy(path, tpath.c_str());
		
		char *temp = path;
		char *temp2;
		
		bool status = true;
		
		if(createIntermediateDirectories)
		{
			while(status && (temp2 = strpbrk(temp, RNPathDelimiterTokens)))
			{
				if(temp != temp2)
				{
					*temp2 = '\0';
					status = MakeDirectory(temp);
					*temp2 = RNPathDelimiter;
				}
				
				temp = temp2 + 1;
			}
		}
		
		if(status)
			status = MakeDirectory(path);
		
		delete[] path;
		return status;
	}
		
	std::string PathManager::ExecutableDirectory()
	{
#if RN_PLATFORM_WINDOWS
		char buffer[MAX_PATH];
		DWORD size = MAX_PATH;
		
		GetModuleFileNameA(0, buffer, size);
		
		char *temp = buffer + strlen(buffer);
		while(temp != buffer)
		{
			temp --;
			
			if(RNIsPathDelimiter(*temp))
			{
				*temp = '\0';
				break;
			}
		}
		
		return std::string(buffer);
#endif
		
		return "";
	}
	
	std::string PathManager::SaveDirectory()
	{
		std::string title = Kernel::GetSharedInstance()->GetTitle();
		std::string path;
		
#if RN_PLATFORM_MAC_OS
		NSString *basepath = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) objectAtIndex:0];
		path = std::move(Join(std::string([basepath UTF8String]), title));
#endif
		
#if RN_PLATFORM_LINUX
		std::string basepath = "~/." + title;
		
		wordexp_t result;
		wordexp(basepath.c_str(), &result, 0);
		
		path = std::string(result.we_wordv[0]);
		wordfree(&result);
#endif
		
#if RN_PLATFORM_WINDOWS
		TCHAR tpath[MAX_PATH];
		::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, tpath);
		
		std::stringstream stream;
		stream << tpath << RNPathDelimiter << title;

		path = stream.str();
#endif
		
		CreatePath(path);
		return path;
	}
}
