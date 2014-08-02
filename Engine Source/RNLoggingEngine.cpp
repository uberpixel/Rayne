//
//  RNLoggingEngine.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLoggingEngine.h"
#include "RNPathManager.h"
#include "RNKernel.h"
#include "RNSTL.h"

namespace RN
{
	namespace Log
	{
		RNDefineMeta(LoggingEngine, Object)
		RNDefineMeta(CallbackLoggingEngine, LoggingEngine)
		RNDefineMeta(StdoutLoggingEngine, LoggingEngine)
		RNDefineMeta(SimpleLoggingEngine, LoggingEngine)
		RNDefineMeta(HTMLLoggingEngine, LoggingEngine)
		
		RNDefineSingleton(StdoutLoggingEngine)
		RNDefineSingleton(SimpleLoggingEngine)
		RNDefineSingleton(HTMLLoggingEngine)
		
		// ---------------------
		// MARK: -
		// MARK: LoggingEngine
		// ---------------------
		
		LoggingEngine::LoggingEngine()
		{
#if RN_BUILD_DEBUG
			_level = Level::Debug;
#endif
#if RN_BUILD_RELEASE
			_level = Level::Info;
#endif
		}
		
		void LoggingEngine::SetLevel(Level level)
		{
			_level.store(level);
		}
		
		// ---------------------
		// MARK: -
		// MARK: CallbackLoggingEngine
		// ---------------------
		
		CallbackLoggingEngine::CallbackLoggingEngine(LoggingEngineDelegate *delegate) :
			_delegate(delegate)
		{}
		
		void CallbackLoggingEngine::Open()
		{
			_delegate->Open();
		}
		
		void CallbackLoggingEngine::Close()
		{
			_delegate->Close();
		}
		
		bool CallbackLoggingEngine::IsOpen() const
		{
			return _delegate->IsOpen();
		}
		
		void CallbackLoggingEngine::CutOff()
		{
			_delegate->CutOff();
		}
		
		void CallbackLoggingEngine::Write(const Message &message)
		{
			_delegate->Write(message);
		}
		
		// ---------------------
		// MARK: -
		// MARK: StreamLoggingInternal
		// ---------------------
		
		class StreamLoggingInternal
		{
		public:
			StreamLoggingInternal(std::ostream &stream) :
				_stream(stream)
			{}
			
			void Write(const Message &message)
			{
				const char *level;
				
				switch(message.GetLevel())
				{
					case Level::Debug:
						level = "(dbg)";
						break;
						
					case Level::Info:
						level = "(info)";
						break;
						
					case Level::Warning:
						level = "(warn)";
						break;
						
					case Level::Error:
						level = "(err)";
						break;
				}
				
				if(message.HasTitle())
				{
					_stream << message.GetFormattedTime() << ": " << level << " " << message.GetTitle() << ": " << message.GetMessage() << std::endl;
				}
				else
				{
					_stream << message.GetFormattedTime() << ": " << level << " " << message.GetMessage() << std::endl;
				}
			}
			
		private:
			std::ostream &_stream;
		};
		
		// ---------------------
		// MARK: -
		// MARK: StdoutLoggingEngine
		// ---------------------
		
		StdoutLoggingEngine::StdoutLoggingEngine() :
			_internal(std::clog)
		{
			SetLevel(Level::Debug);
		}
		
		void StdoutLoggingEngine::Open()
		{}
		
		void StdoutLoggingEngine::Close()
		{}
		
		bool StdoutLoggingEngine::IsOpen() const
		{
			return true;
		}
		
		void StdoutLoggingEngine::CutOff()
		{}
		
		void StdoutLoggingEngine::Write(const Message &message)
		{
			_internal->Write(message);
		}
		
		// ---------------------
		// MARK: -
		// MARK: SimpleLoggingEngine
		// ---------------------
		
		SimpleLoggingEngine::SimpleLoggingEngine() :
			_internal(_stream)
		{}
		
		void SimpleLoggingEngine::Open()
		{
			_stream.open(PathManager::Join(PathManager::SaveDirectory(), "Log.log"), std::ios_base::out | std::ios_base::trunc);
		}
		
		void SimpleLoggingEngine::Close()
		{
			_stream.flush();
			_stream.close();
		}
		
		bool SimpleLoggingEngine::IsOpen() const
		{
			return _stream.is_open();
		}
		
		void SimpleLoggingEngine::CutOff()
		{}
		
		void SimpleLoggingEngine::Write(const Message &message)
		{
			_internal->Write(message);
		}
		
		// ---------------------
		// MARK: -
		// MARK: HTMLLoggingEngine
		// Mode: 0 = Normal text, 1 = Title
		// ---------------------
		
		void HTMLLoggingEngine::Open()
		{
			_mode = 0xffff;
			_stream.open(PathManager::Join(PathManager::SaveDirectory(), "Log.html"), std::ios_base::out | std::ios_base::trunc);
			
			_stream << "<!DOCTYPE HTML>" << std::endl;
			_stream << "<html><head>" << std::endl;
			
			_stream << "<title>" << Kernel::GetSharedInstance()->GetTitle() << " Log" << "</title>" << std::endl;
			WriteCSSBoilerplate();
			
			_stream << "</head>" << std::endl;
			_stream << "<body><div class=\"content\">" << std::endl;
			
			_stream << "<div class=\"wrapper header\"><div class=\"padding box\">" << std::endl;
			_stream << "<h1>" << Kernel::GetSharedInstance()->GetTitle() << " Log" << "</h1>" << std::endl;
			_stream << "</div></div>" << std::endl;
		}
		
		void HTMLLoggingEngine::Close()
		{
			SwitchMode(0xffff);
			_stream << "</div></body>" << std::endl << "</html>";
			
			_stream.flush();
			_stream.close();
		}
		
		bool HTMLLoggingEngine::IsOpen() const
		{
			return _stream.is_open();
		}
		
		void HTMLLoggingEngine::WriteCSSBoilerplate()
		{
			_stream << "<style type=\"text/css\">" << std::endl;
			_stream << "body { background-color: #e8e8e8;  font-family: Helvetica, Arial, sans-serif; font-weight: 200; font-size: 14px } " << std::endl;
			_stream << "h1 { font-size: 22px; line-height: 1.2em; margin: 0; } " << std::endl;
			_stream << "table { width: 100%; border-top: 1px solid black; border-left: 1px solid black; border-right: 1px solid black; }" << std::endl;
			_stream << "th { width: 180px; vertical-align: top; text-align: left; border-right: 1px solid black; border-bottom: 1px solid black; }" << std::endl;
			_stream << "td { vertical-align: top; border-bottom: 1px solid black; }" << std::endl;
			_stream << ".content { margin: 0 auto; padding: 0 10px; position: relative; max-width: 1000px; }" << std::endl;
			_stream << ".entry-header { }" << std::endl;
			_stream << ".entry { }" << std::endl;
			_stream << ".wrapper { width: 100%; margin-bottom: 10px; background-color: white; }" << std::endl;
			_stream << ".box { border: 1px solid #000; }" << std::endl;
			_stream << ".padding { padding: 5px; }" << std::endl;
			_stream << ".header { background-color: #68aeee; }" << std::endl;
			_stream << ".table-entry { max-height: 130px; overflow: auto; }" << std::endl;
			_stream << "</style>" << std::endl;
		}
		
		void HTMLLoggingEngine::CutOff()
		{
			SwitchMode(0xffff);
		}
		
		void HTMLLoggingEngine::Write(const Message &message)
		{
			int mode = message.HasTitle() ? 1 : 0;
			SwitchMode(mode);
			
			const std::string &header = (message.HasTitle()) ? message.GetTitle() : message.GetFormattedTime();
			std::string body = std::move(stl::html_encode(message.GetMessage()));
			
			switch(mode)
			{
				case 0:
					_stream << "<div class=\"entry\"><span class=\"entry-header\">" << header << "</span>: " << body << "</div>" << std::endl;
					break;
					
				case 1:
					_stream << "<tr><th><div class=\"padding\">" << header << "</div></th><td><div class=\"padding table-entry\">" << body << "</div></td></tr>" << std::endl;
					break;
			}
		}
		
		void HTMLLoggingEngine::SwitchMode(int mode)
		{
			if(_mode != mode)
			{
				switch(_mode)
				{
					case 0:
						_stream << "</div></div>" << std::endl;
						break;
						
					case 1:
						_stream << "</tbody></table></div>" << std::endl;
						break;
						
					case 0xffff:
						break;
				}
				
				_mode = mode;
				
				switch(_mode)
				{
					case 0:
						_stream << "<div class=\"wrapper\"><div class=\"box padding\">" << std::endl;
						break;
						
					case 1:
						_stream << "<div class=\"wrapper\"><table cellspacing=\"0\" cellpadding=\"0\"><tbody>" << std::endl;
						break;
						
					case 0xffff:
						break;
				}
			}
		}
	}
}
