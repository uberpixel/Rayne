//
//  RNAny.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ANY_H__
#define __RAYNE_ANY_H__

#include "RNBase.h"

namespace RN
{
	namespace stl
	{
		class any
		{
		public:
			any() :
				_content(nullptr)
			{}
			template<class T>
			any(const T &value) :
				_content(new holder<T>(value))
			{}
			any(const any &other) :
				_content(other._content ? other._content->clone() : nullptr)
			{}
			any(any &&other) :
				_content(other._content)
			{
				other._content = nullptr;
			}
			~any()
			{
				delete _content;
			}
			
			
			any &operator =(const any &other)
			{
				any temp(other);
				std::swap(_content, temp._content);
				
				return *this;
			}
			any &operator =(any &&other)
			{
				std::swap(_content, other._content);
				
				delete other._content;
				other._content = nullptr;
				
				return *this;
			}
			template<class T>
			any &operator =(const T &value)
			{
				any temp(value);
				std::swap(_content, temp._content);
				
				return *this;
			}
			
			
			any &swap(any &other)
			{
				std::swap(_content, other._content);
				return *this;
			}
			
			
			bool empty() const
			{
				return !_content;
			}
			void clear()
			{
				delete _content;
				_content = nullptr;
			}
			const std::type_info &type() const
			{
				return _content ? _content->type() : typeid(void);
			}
			
		private:
			class placeholder
			{
			public:
				virtual ~placeholder()
				{}
				
				virtual const std::type_info &type() const = 0;
				virtual placeholder *clone() const = 0;
			};
			
			template<class T>
			class holder : public placeholder
			{
			public:
				holder(const T &value) :
					held(value)
				{}
				holder(T &&value) :
					held(std::move(value))
				{}
				
				const std::type_info &type() const override
				{
					return typeid(T);
				}
				placeholder *clone() const override
				{
					return new holder(held);
				}
				
				T held;
				
			private:
				holder &operator =(const holder &);
			};
			
			template<class T>
			friend T *any_cast(any *);
			
			placeholder *_content;
		};
		
		class bad_any_cast : public std::bad_cast
		{
		public:
			const char *what() const RN_NOEXCEPT override
			{
				return "stl::bad_any_cast: failed conversion using stl::any_cast";
			}
		};
	
		
		template<class T>
		T *any_cast(any *operand)
		{
			return (operand && operand->type() == typeid(T)) ? &static_cast<any::holder<T> *>(operand->_content)->held : nullptr;
		}
		template<class T>
		const T *any_cast(const any *operand)
		{
			return any_cast<T>(const_cast<any *>(operand));
		}
		
		template<class T>
		T any_cast(any &operand)
		{
			typedef typename std::remove_reference<T>::type type;
			
			type *result = any_cast<type>(&operand);
			if(!result)
				throw bad_any_cast();
				
			return static_cast<type &>(*result);
		}
		template<class T>
		T any_cast(const any &operand)
		{
			typedef typename std::remove_reference<T>::type type;
			return any_cast<const type &>(const_cast<any &>(operand));
		}
	}
}

#endif /* __RAYNE_ANY_H__ */
