//
//  RNOpenPanel.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		RNAPI OpenPanel();
		
		RNAPI void SetAllowsFolders(bool allowsFolders);
		RNAPI void SetAllowsFiles(bool allowsFiles);
		RNAPI void SetAllowsMultipleSelection(bool multipleSelection);
		RNAPI void SetCanCreateDirectories(bool canCreateDirectories);
		
		RNAPI void SetTitle(const std::string& title);
		RNAPI void SetMessage(const std::string& message);
		
		RNAPI void SetAllowedFileTypes(const std::vector<std::string>& fileTypes);
		
		RNAPI void Show(std::function<void (bool result, const std::vector<std::string>& paths)> callback);
		
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
