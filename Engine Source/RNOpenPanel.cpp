//
//  RNOpenPanel.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenPanel.h"
#include "RNBaseInternal.h"

namespace RN
{
	RNDefineMeta(SavePanel, Object)
	RNDefineMeta(OpenPanel, SavePanel)
	
	SavePanel::SavePanel()
	{
		_canCreateDirectories = true;
	}
	
	void SavePanel::SetCanCreateDirectories(bool canCreateDirectories)
	{
		_canCreateDirectories = canCreateDirectories;
	}
	
	
	void SavePanel::SetTitle(const std::string& title)
	{
		_title = title;
	}
	
	void SavePanel::SetMessage(const std::string& message)
	{
		_message = message;
	}
	
	
	void SavePanel::SetAllowedFileTypes(const std::vector<std::string>& fileTypes)
	{
		_allowedFileTypes = fileTypes;
	}
	
	void SavePanel::Show(std::function<void (bool result,  const std::string &path)> callback)
	{
#if RN_PLATFORM_MAC_OS
		NSSavePanel *panel = [NSSavePanel savePanel];
		[panel setCanCreateDirectories:_canCreateDirectories];
		
		[panel setTitle:[NSString stringWithUTF8String:_title.c_str()]];
		[panel setMessage:[NSString stringWithUTF8String:_message.c_str()]];
		
		if(!_allowedFileTypes.empty())
		{
			NSMutableArray *fileTypes = [NSMutableArray array];
			
			for(const std::string& type : _allowedFileTypes)
				[fileTypes addObject:[NSString stringWithUTF8String:type.c_str()]];
			
			[panel setAllowedFileTypes:fileTypes];
		}
		
		
		Retain();
		
		[panel beginSheetModalForWindow:[NSApp keyWindow] completionHandler:^(NSInteger tresult) {
			bool result = (tresult == NSFileHandlingPanelOKButton);
			NSURL *url = [panel URL];
			
			callback(result, url ? [[url path] UTF8String] : "");
			Autorelease();
		}];
#endif
	}
	
	
	
	OpenPanel::OpenPanel()
	{
		_allowsFolders = false;
		_allowsFiles = true;
		_allowsMultipleSelection = false;
	}
	
	
	void OpenPanel::SetAllowsFolders(bool allowsFolders)
	{
		_allowsFolders = allowsFolders;
	}
	
	void OpenPanel::SetAllowsFiles(bool allowsFiles)
	{
		_allowsFiles = allowsFiles;
	}
	
	void OpenPanel::SetAllowsMultipleSelection(bool multipleSelection)
	{
		_allowsMultipleSelection = multipleSelection;
	}
	
	
	void OpenPanel::Show(std::function<void (bool result, const std::vector<std::string>& paths)> callback)
	{
#if RN_PLATFORM_MAC_OS
		NSOpenPanel *panel = [NSOpenPanel openPanel];
		
		[panel setAllowsMultipleSelection:_allowsMultipleSelection];
		[panel setCanChooseDirectories:_allowsFolders];
		[panel setCanChooseFiles:_allowsFiles];
		[panel setCanCreateDirectories:_canCreateDirectories];
		
		[panel setTitle:[NSString stringWithUTF8String:_title.c_str()]];
		[panel setMessage:[NSString stringWithUTF8String:_message.c_str()]];
		
		if(!_allowedFileTypes.empty())
		{
			NSMutableArray *fileTypes = [NSMutableArray array];
			
			for(const std::string& type : _allowedFileTypes)
				[fileTypes addObject:[NSString stringWithUTF8String:type.c_str()]];
			
			[panel setAllowedFileTypes:fileTypes];
		}
		
		Retain();
		
		[panel beginSheetModalForWindow:[NSApp keyWindow] completionHandler:^(NSInteger tresult) {
			bool result = (tresult == NSFileHandlingPanelOKButton);
			NSArray *urls = [panel URLs];
		 
			std::vector<std::string> files;
			for(NSURL *url in urls)
			{
				files.push_back([[url path] UTF8String]);
			}
		 
			callback(result, files);
			Autorelease();
		}];
#endif
		
#if 0 //RN_PLATFORM_WINDOWS
		TCHAR szFile[MAX_PATH];
		TCHAR szFilter[1024];
		OPENFILENAME ofn;
		
		ZeroMemory(szFile, MAX_PATH);
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner   = hWnd;
		ofn.lpstrTitle  = _title.c_str();
		ofn.lpstrFile   = szFile;
		ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
		ofn.nMaxFile    = MAX_PATH;
		ofn.Flags       = OFN_PATHMUSTEXIST | OFN_EXPLORER;
		
		if(_allowsMultipleSelection)
			ofn.Flags |= OFN_ALLOWMULTISELECT;
		
		if(_allowsFiles)
			ofn.Flags |= OFN_FILEMUSTEXIST;
		
		if(!_allowedFileTypes.empty())
		{
			ZeroMemory(szFilter, 1024);
			TCHAR *temp = szFile;
			
			for(const std::string& type : _allowedFileTypes)
			{
				strcpy(temp, type.c_str());
				temp += type.size() + 1;
				
				std::string filter = "*." + type;
				
				strcpy(temp, filter.c_str());
				temp += filter.size() + 1;
			}
			
			ofn.lpstrFilter = szFilter;
		}
			
		bool result = ::GetOpenFileName(&ofn);
		std::vector<std::string> files;
		
		if(result)
		{
			if(_allowsMultipleSelection)
			{
				char *base = szFile;
				char *temp = base + strlen(base) + 1;
				
				while(*temp)
				{
					std::stringstream stream;
					stream << base << temp;
					
					files.push_back(stream.str());
					
					temp += strlen(temp) + 1;
				}
			}
			else
			{
				files.push_back(szFile);
			}
		}
		
		callback(result, files);
#endif
	}
}
