//
//  RNPathManager.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "RNBaseInternal.h"

#include "RNPathManager.h"
#include "RNApplication.h"
#include "RNFile.h"
#include "RNKernel.h"

namespace RN
{
	std::vector<std::string> PathManager::_searchPaths;
	std::vector<std::string> PathManager::_globalModifiers;
	std::unordered_map<std::string, std::vector<std::string>> PathManager::_fileModifiers;
	
	std::string PathManager::Join(const std::string& path1, const std::string& path2)
	{
		std::string result = Basepath(path1);
		
		bool endSeperator = (result[result.size() - 1] == '/');
		bool startSeperator = (path2[0] == '/');
		
		if(endSeperator && startSeperator)
		{
			result += path2.substr(1);
		}
		else if(endSeperator || startSeperator)
		{
			result += path2;
		}
		else
		{
			result += "/";
			result += path2;
		}
		
		return result;
	}
	
	std::string PathManager::PathByRemovingExtension(const std::string& path)
	{
		size_t i = path.size();
		while((i --) > 0)
		{
			if(path[i] == '.')
				return path.substr(0, i);
		}
			
		return path;
	}
	
	std::vector<std::string> PathManager::PathComoponents(const std::string& path)
	{
		const char *cstr = path.c_str();
		std::vector<std::string> result;
		
		while((*cstr == '/' || *cstr == '\\'))
			cstr ++;
		
		while(1)
		{
			const char *end = strpbrk(cstr, "/\\");
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
	
	std::string PathManager::Base(const std::string& path)
	{
		size_t marker = path.size();
		size_t i = marker;
		
		while((i --) > 0)
		{
			if(path[i] == '/')
			{
				i ++;
				break;
			}
		}
		
		return path.substr(i, marker - i);
	}
	
	std::string PathManager::Basename(const std::string& path)
	{
		bool hasExtension = false;
		
		size_t marker = path.size();
		size_t i = marker;
		
		while((i --) > 0)
		{
			if(!hasExtension && path[i] == '.')
			{
				marker = i;
				hasExtension = true;
			}
			
			if(path[i] == '/')
			{
				i ++;
				break;
			}
		}
		
		return path.substr(i, marker - i);
	}
	
	std::string PathManager::Basepath(const std::string& path)
	{
		bool hasExtension = false;
		
		size_t marker = path.size();
		size_t i = marker;
		
		while((i --) > 0)
		{
			if(path[i] == '/')
			{
				marker = i;
				break;
			}
			
			if(path[i] == '.')
				hasExtension = true;
		}
		
		return hasExtension ? path.substr(0, marker) : path;
	}
	
	std::string PathManager::Extension(const std::string& path)
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
	
	
	bool PathManager::PathExists(const std::string& path)
	{
		return PathExists(path, 0);
	}
	
	bool PathManager::PathExists(const std::string& path, bool *isDirectory)
	{
		struct stat buf;
		int result = stat(path.c_str(), &buf);
		
		if(result != 0)
			return false;
		
		if(isDirectory)
			*isDirectory = S_ISDIR(buf.st_mode);
		
		return true;
	}
	
	static inline bool MakeDirectory(const char *path)
	{
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
	}
	
	bool PathManager::CreatePath(const std::string& tpath, bool createIntermediateDirectories)
	{
		bool isDirectory;
		
		if(PathExists(tpath, &isDirectory))
			return isDirectory;
		
		char *path = new char[tpath.length()];
		strcpy(path, tpath.c_str());
		
		char *temp = path;
		char *temp2;
		
		bool status = true;
		
		if(createIntermediateDirectories)
		{
			while(status && (temp2 = strpbrk(temp, "/\\")))
			{
				if(temp != temp2)
				{
					*temp2 = '\0';
					status = MakeDirectory(temp);
					*temp2 = '/';
				}
				
				temp = temp2 + 1;
			}
		}
		
		if(status)
			status = MakeDirectory(path);
		
		delete[] path;
		return status;
	}
	
	
	void PathManager::AddSearchPath(const std::string& path)
	{
		_searchPaths.push_back(path);
	}
	
	void PathManager::AddFileModifier(const std::string& modifier, const std::string& extension)
	{
		auto iterator = _fileModifiers.find(extension);
		if(iterator != _fileModifiers.end())
		{
			std::vector<std::string>& modifiers = iterator->second;
			modifiers.push_back(modifier);
		}
		else
		{
			std::vector<std::string> modifiers;
			modifiers.push_back(modifier);
			
			_fileModifiers.insert(std::unordered_map<std::string, std::vector<std::string>>::value_type(extension, modifiers));
		}
	}
	
	void PathManager::AddFileModifier(const std::string& modifier)
	{
		_globalModifiers.push_back(modifier);
	}
	
	
	
	void PathManager::AddDefaultSearchPaths()
	{
		static bool addedDefaultSearchPaths = false;
		
		if(!addedDefaultSearchPaths)
		{
#if RN_PLATFORM_MAC_OS
			NSString *path = [[NSBundle mainBundle] resourcePath];
			
			AddSearchPath(std::string([path UTF8String]));
			AddSearchPath(std::string([path UTF8String]) + "/Engine Resources");
			
			AddFileModifier("~mac");
#endif
			
#if RN_PLATFORM_LINUX
			char *path = get_current_dir_name();
			AddSearchPath(std::string(path));
			AddSearchPath(std::string(path) + "/Engine Resources");
			free(path);
			
			AddFileModifier("~linux");
#endif
			
#if RN_PLATFORM_LINUX || RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
			AddFileModifier("~150", "vsh");
			AddFileModifier("~140", "vsh");
			AddFileModifier("~130", "vsh");
			AddFileModifier("~120", "vsh");
			AddFileModifier("~110", "vsh");
			
			AddFileModifier("~150", "fsh");
			AddFileModifier("~140", "fsh");
			AddFileModifier("~130", "fsh");
			AddFileModifier("~120", "fsh");
			AddFileModifier("~110", "fsh");
			
			AddFileModifier("~150", "gsh");
			AddFileModifier("~140", "gsh");
			AddFileModifier("~130", "gsh");
			AddFileModifier("~120", "gsh");
			AddFileModifier("~110", "gsh");
#endif
			
			
#if RN_PLATFORM_IOS
			NSString *path = [[NSBundle mainBundle] resourcePath];
			AddSearchPath(std::string([path UTF8String]));
			AddSearchPath(std::string([path UTF8String]) + "/Engine Resources");
			
			FileModifiers.push_back("~es2");
			FileModifiers.push_back("~ios");
			
			if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
			{
				FileModifiers.push_back("~iphone~es2");
				FileModifiers.push_back("~iphone");
			}
			else if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
			{
				FileModifiers.push_back("~ipad~es2");
				FileModifiers.push_back("~ipad");
			}
#endif
			
#if RN_PLATFORM_WINDOWS
			char buffer[MAX_PATH];
			DWORD size = MAX_PATH;
			
			GetModuleFileNameA(0, buffer, size);
			
			char *temp = buffer + strlen(buffer);
			while(temp != buffer)
			{
				temp --;
				
				if(*temp == '\\')
				{
					*temp = '\0';
					break;
				}
			}
			
			AddSearchPath(std::string(buffer));
			AddSearchPath(std::string(buffer) + "/Engine Resources");
			
#ifndef NDEBUG
			while(temp != buffer)
			{
				temp --;
				
				if(*temp == '\\')
				{
					*temp = '\0';
					break;
				}
			}
			
			AddSearchPath(std::string(buffer));
			AddSearchPath(std::string(buffer) + "/Engine Resources");
#endif
#endif
			
			addedDefaultSearchPaths = true;
		}
	}
	
	std::string PathManager::PathForName(const std::string& path, bool strict)
	{
		if(PathExists(path))
			return path;
		
		AddDefaultSearchPaths();
		
		std::string basepath = PathByRemovingExtension(path);
		std::string extension = Extension(path);
		
		std::vector<std::string> modifiers;
		
		auto iterator = _fileModifiers.find(extension);
		if(iterator != _fileModifiers.end())
		{
			modifiers.insert(modifiers.end(), iterator->second.begin(), iterator->second.end());
		}
		
		modifiers.insert(modifiers.end(), _globalModifiers.begin(), _globalModifiers.end());
		extension = "." + extension;
		
		for(auto i=modifiers.begin(); i!=modifiers.end(); i++)
		{
			std::string filePath = basepath + *i + extension;
			
			if(PathExists(filePath))
				return filePath;
		}
		
		for(auto i=_searchPaths.begin(); i!=_searchPaths.end(); i++)
		{
			std::string base = Join(*i, basepath);
			
			for(auto j=modifiers.begin(); j!=modifiers.end(); j++)
			{
				std::string filePath = base + *j + extension;
				
				if(PathExists(filePath))
					return filePath;
			}
			
			base += extension;
			
			if(PathExists(base))
				return base;
		}
		
		if(!strict)
		{
			try
			{
				std::string tpath = Basepath(path);
				tpath = PathForName(tpath);
				
				return Join(tpath, Base(path));
			}
			catch(Exception)
			{}
		}
		
		throw Exception(Exception::Type::GenericException, "Couldn't resolve path " + path);
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
			
			if(*temp == '\\')
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
		
		CreatePath(path);
		return path;
	}
}
