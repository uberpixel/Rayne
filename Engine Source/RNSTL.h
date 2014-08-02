//
//  RNSTL.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STL_H__
#define __RAYNE_STL_H__

namespace RN
{
	namespace stl
	{
		static inline std::string html_encode(const std::string &string)
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
		
		static inline void html_encode(std::string &string)
		{
			std::string buffer = std::move(html_encode(const_cast<const std::string&>(string)));
			std::swap(buffer, string);
		}
		
		template<class T>
		class lockable_shim
		{
		public:
			lockable_shim(T &lock) :
				_lock(lock)
			{}
		
			bool try_lock()
			{
				return _lock.TryLock();
			}
			
			void lock()
			{
				return _lock.Lock();
			}
			
			void unlock()
			{
				return _lock.Unlock();
			}
			
		private:
			T &_lock;
		};
	}
}

#endif
