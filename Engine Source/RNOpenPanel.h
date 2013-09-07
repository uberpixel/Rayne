//
//  RNOpenPanel.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENPANEL_H__
#define __RAYNE_OPENPANEL_H__

#include "RNBase.h"

namespace RN
{
	class OpenPanel
	{
	public:
		OpenPanel();
		
		void SetAllowsFolders(bool allowsFolders);
		void SetAllowsFiles(bool allowsFiles);
		void SetAllowsMultipleSelection(bool multipleSelection);
		void SetCanCreateDirectories(bool canCreateDirectories);
		
		void SetTitle(const std::string& title);
		void SetMessage(const std::string& message);
		
		void SetAllowedFileTypes(const std::vector<std::string>& fileTypes);
		
		void Show(std::function<void (bool result, const std::vector<std::string>& paths)> callback);
		
	private:
		bool _allowsFolders;
		bool _allowsFiles;
		bool _allowsMultipleSelection;
		bool _canCreateDirectories;
		
		std::string _title;
		std::string _message;
		
		std::vector<std::string> _allowedFileTypes;
	};
}

#endif /* __RAYNE_OPENPANEL_H__ */
