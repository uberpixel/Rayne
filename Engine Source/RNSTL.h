//
//  RNSTL.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STL_H__
#define __RAYNE_STL_H__

#include "RNAlgorithm.h"

namespace RN
{
	namespace stl
	{
		std::string html_encode(const std::string& string)
		{
			std::string buffer;
			buffer.reserve(string.size() * 1.1f);
			
			for(size_t i = 0; i < string.size(); i ++)
			{
				switch(string[i])
				{
					case '&':
						buffer.append("&amp;");
						break;
					case '\"':
						buffer.append("&quot;");
						break;
					case '\'':
						buffer.append("&apos;");
						break;
					case '<':
						buffer.append("&lt;");
						break;
					case '>':
						buffer.append("&gt;");
						break;
					case '\n':
						buffer.append("<br />");
						break;
					case '\t':
						buffer.append("&emsp;");
						break;
					default:
						buffer.append(1, string[i]);
						break;
				}
			}
			
			return buffer;
		}
		
		void html_encode(std::string& string)
		{
			std::string buffer = std::move(html_encode(const_cast<const std::string&>(string)));
			std::swap(buffer, string);
		}
	}
}

#endif
