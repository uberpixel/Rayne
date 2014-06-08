//
//  RNUIServer.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBaseInternal.h"
#include "RNUIServer.h"
#include "RNKernel.h"
#include "RNWorld.h"
#include "RNWindow.h"
#include "RNUIWidget.h"
#include "RNUIWidgetInternals.h"
#include "RNUILabel.h"
#include "RNUIButton.h"

#if RN_PLATFORM_MAC_OS

static const char *__RNUIMenuItemWrapperKey = "__RNUIMenuItemWrapperKey";

@interface __RNUIMenuItemWrapper : NSObject
@property (nonatomic, assign) RN::UI::MenuItem *item;
+ (__RNUIMenuItemWrapper *)wrapperWithItem:(RN::UI::MenuItem *)item;
@end

@implementation __RNUIMenuItemWrapper
@synthesize item;

- (void)setItem:(RN::UI::MenuItem *)titem
{
	if(item)
	{
		item->RemoveAssociatedOject(__RNUIMenuItemWrapperKey);
		item->Release();
	}
	
	item = titem;
	
	if(item)
	{
		item->SetAssociatedObject(__RNUIMenuItemWrapperKey, (RN::Object *)self, RN::Object::MemoryPolicy::Assign);
		item->Retain();
	}
}

- (void)dealloc
{
	[self setItem:nil];
	[super dealloc];
}

+ (__RNUIMenuItemWrapper *)wrapperWithItem:(RN::UI::MenuItem *)item
{
	__RNUIMenuItemWrapper *wrapper = [[__RNUIMenuItemWrapper alloc] init];
	[wrapper setItem:item];
	return [wrapper autorelease];
}
@end

#endif /* RN_PLATFORM_MAC_OS */

namespace RN
{
	namespace UI
	{
		RNDefineSingleton(Server)
		
		Server::Server() :
			_keyWidget(nullptr),
			_tracking(nullptr),
			_hover(nullptr),
			_drawDebugFrames(false),
			_menu(nullptr)
#if RN_PLATFORM_WINDOWS
			,_menuTranslation(nullptr)
#endif
		{
			uint32 flags = Camera::Flags::Orthogonal | Camera::Flags::UpdateAspect | Camera::Flags::UpdateStorageFrame | Camera::Flags::NoSorting | Camera::Flags::NoDepthWrite | Camera::Flags::BlendedBlitting;
			_camera = new Camera(Vector2(0.0f), Texture::Format::RGBA16F, flags, RenderStorage::BufferFormatColor);
			_camera->SetClearColor(RN::Color(0.0f, 0.0f, 0.0f, 0.0f));
			_camera->SetClipNear(-500.0f);
			_camera->SetLightManager(nullptr);
			_camera->RemoveFromWorld();
			
			TranslateMenuToPlatform();
		}
		
		Server::~Server()
		{
			_camera->Release();
			SafeRelease(_menu);
#if RN_PLATFORM_WINDOWS
			SafeRelease(_menuTranslation);
#endif
		}
		
		void Server::SetDrawDebugFrames(bool drawDebugFrames)
		{
			_drawDebugFrames = drawDebugFrames;
		}
		
		void Server::SetMainMenu(Menu *menu)
		{
			SafeRelease(_menu);
			_menu = SafeRetain(menu);
			
			TranslateMenuToPlatform();
		}
		
		void Server::AddWidget(Widget *widget)
		{
			RN_ASSERT(widget->_server == nullptr, "");
			
			_widgets.push_front(widget);
			widget->_server = this;
			widget->Retain();
			
			SortWidgets();
		}
		
		void Server::RemoveWidget(Widget *widget)
		{
			RN_ASSERT(widget->_server == this, "");
			
			if(widget == _keyWidget)
				SetKeyWidget(nullptr);
			
			if(_tracking && _tracking->IsKindOfClass(View::GetMetaClass()))
			{
				View *tracking = static_cast<View *>(_tracking);
				
				if(_tracking && (tracking->_widget == widget || !tracking->_widget))
					_tracking = nullptr;
			}
			
			if(_hover && _hover->IsKindOfClass(View::GetMetaClass()))
			{
				View *hover = static_cast<View *>(_hover);
				
				if(_hover && (hover->_widget == widget || !hover->_widget))
					_hover = nullptr;
			}
			
			_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
			widget->_server = nullptr;
			widget->Autorelease();
		}
		
		void Server::MoveWidgetToFront(Widget *widget)
		{
			RN_ASSERT(widget->_server == this, "");
			
			_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
			_widgets.push_back(widget);
			
			SortWidgets();
		}
		
		void Server::SetKeyWidget(Widget *widget)
		{
			RN_ASSERT(!widget || widget->_server == this, "");
			
			if(_keyWidget)
			{
				if(_keyWidget->_backgroundView)
					_keyWidget->_backgroundView->SetState(Control::State::Normal);
				
				_keyWidget = nullptr;
			}
			
			_keyWidget = widget;
			
			if(_keyWidget)
			{
				_keyWidget->AcceptKey();
			
				if(_keyWidget->_backgroundView)
					_keyWidget->_backgroundView->SetState(Control::State::Selected);
			}
		}
		
		void Server::SortWidgets()
		{
			std::stable_sort(_widgets.begin(), _widgets.end(), [](const Widget *a, const Widget *b) {
				return (a->_level < b->_level);
			});
		}
		
		bool Server::ConsumeEvent(Event *event)
		{
			if(event->IsMouse())
			{
				if(_tracking)
				{
					switch(event->GetType())
					{
						case Event::Type::MouseDragged:
							_tracking->MouseDragged(event);
							return true;
							
						case Event::Type::MouseUp:
							_tracking->MouseUp(event);
							_tracking = nullptr;
							return true;
							
						default:
							break;
					}
				}
				
				const Vector2 &position = event->GetMousePosition();
				
				Widget *hitWidget = nullptr;
				Responder *hit = nullptr;
				
				for(auto i = _widgets.rbegin(); i != _widgets.rend(); i ++)
				{
					Widget *widget = *i;
				
					hit = widget->PerformHitTest(position, event);
					if(hit)
					{
						hitWidget = widget;
						break;
					}
				}
				
				if(!hit && !hitWidget)
					hit = Application::GetSharedInstance();
				
				switch(event->GetType())
				{
					case Event::Type::MouseWheel:
						hit->ScrollWheel(event);
						return true;
						
					case Event::Type::MouseDown:
						_tracking   = hit;
						
						if(hitWidget)
						{
							if(!_keyWidget || _keyWidget->CanResignKeyWidget())
							{
								if(_keyWidget)
									_keyWidget->ResignKey();
								
								if(hitWidget->CanBecomeKeyWidget())
									SetKeyWidget(hitWidget);
								
								MoveWidgetToFront(hitWidget);
								hit->MouseDown(event);
							}
						}
						else
						{
							hit->MouseDown(event);
						}
						
						return true;
						
					case Event::Type::MouseMoved:
					{
						if(_hover && _hover != hit)
							_hover->MouseLeft(event);
						
						hit->MouseMoved(event);
						_hover = hit;
						
						return true;
					}
						
					case Event::Type::MouseDragged:
						_tracking = hit;
						
						hit->MouseDragged(event);
						return true;
						
					case Event::Type::MouseUp:
						_tracking = nullptr;
						
						hit->MouseUp(event);
						return true;
						
					default:
						break;
				}
				
				return false;
			}
			
			
			if(event->IsKeyboard())
			{
				Responder *responder = _keyWidget ? _keyWidget->GetFirstResponder() : nullptr;
				if(!responder)
				{
					responder = _keyWidget;
					
					if(!responder)
						responder = Application::GetSharedInstance();
				}
				
				if(responder)
				{
					switch(event->GetType())
					{
						case Event::Type::KeyDown:
							responder->KeyDown(event);
							break;
							
						case Event::Type::KeyUp:
							responder->KeyUp(event);
							break;
							
						case Event::Type::KeyRepeat:
							responder->KeyRepeat(event);
							break;
							
						default:
							break;
					}
					
					return false;
				}
			}
			
			return false;
		}
		
		void Server::UpdateSize()
		{
			Vector2 actualSize = Window::GetSharedInstance()->GetSize();
			Rect    actualFrame(Vector2(), actualSize);
			
			if(_frame != actualFrame)
			{
				_frame = actualFrame;
				
				_camera->SetOrthogonalFrustum(_frame.GetBottom(), _frame.GetTop(), _frame.GetLeft(), _frame.GetRight());
				_camera->SetFrame(_frame);
				
				MessageCenter::GetSharedInstance()->PostMessage(kRNUIServerDidResizeMessage, nullptr, nullptr);
			}
		}
		
		void Server::Render(Renderer *renderer)
		{
			UpdateSize();
			
			// Draw all widgets into the camera
			Kernel::GetSharedInstance()->PushStatistics("ui.update");
			
			for(Widget *widget : _widgets)
			{
				widget->Update();
			}
			
			Kernel::GetSharedInstance()->PopStatistics();
			
			
			// Draw all widgets into the camera
			Kernel::GetSharedInstance()->PushStatistics("ui.render");
			
			renderer->SetMode(Renderer::Mode::ModeUI);
			renderer->BeginCamera(_camera);
			
			for(Widget *widget : _widgets)
			{
				widget->Render(renderer);
			}
			
			renderer->FinishCamera();
			
			Kernel::GetSharedInstance()->PopStatistics();
		}
		
		
#if RN_PLATFORM_MAC_OS
		
		NSMenu *TranslateRNUIMenuToNSMenu(Menu *menu)
		{
			NSMenu *temp = [[NSMenu alloc] init];
			[temp setAutoenablesItems:NO];
			
			if(!menu)
				return [temp autorelease];
			
			menu->GetItems()->Enumerate<MenuItem>([&](MenuItem *item, size_t index, bool &stop) {
				
				if(!item->IsSeparator())
				{
					NSString *title = [NSString stringWithUTF8String:item->GetTitle()->GetUTF8String()];
					NSString *key   = [NSString stringWithUTF8String:item->GetKeyEquivalent()->GetUTF8String()];
					
					uint32 modifier = item->GetKeyEquivalentModifierMask();
					NSUInteger modifierMask = 0;
					
					modifierMask |= (modifier & KeyModifier::KeyCommand) ? NSCommandKeyMask : 0;
					modifierMask |= (modifier & KeyModifier::KeyShift) ? NSShiftKeyMask : 0;
					modifierMask |= (modifier & KeyModifier::KeyControl) ? NSControlKeyMask : 0;
					modifierMask |= (modifier & KeyModifier::KeyAlt) ? NSAlternateKeyMask : 0;
					
					NSMenuItem *titem = [[NSMenuItem alloc] initWithTitle:title action:@selector(performMenuBarAction:) keyEquivalent:key];
					[titem setEnabled:item->IsEnabled()];
					[titem setKeyEquivalentModifierMask:modifierMask];
					
					objc_setAssociatedObject(titem, __RNUIMenuItemWrapperKey, [__RNUIMenuItemWrapper wrapperWithItem:item], OBJC_ASSOCIATION_RETAIN);
					
					if(item->GetSubMenu())
					{
						NSMenu *subMenu = TranslateRNUIMenuToNSMenu(item->GetSubMenu());
						[titem setSubmenu:subMenu];
					}
					
					[temp addItem:[titem autorelease]];
				}
				else
				{
					[temp addItem:[NSMenuItem separatorItem]];
				}
			});
			
			return [temp autorelease];
		}
		
		NSMenuItem *NSQuitMenuItem()
		{
			NSString *quitTitle = [@"Quit " stringByAppendingString:[[NSProcessInfo processInfo] processName]];
			NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
			
			return [quitMenuItem autorelease];
		}
		
		void Server::TranslateMenuToPlatform()
		{
			@autoreleasepool
			{
				NSMenu *menu = [[NSMenu alloc] init];
				[menu setAutoenablesItems:NO];
				
				NSMenu *quitMenu = [[NSMenu alloc] init];
				[quitMenu addItem:NSQuitMenuItem()];
				
				NSMenuItem *appMenu = [[NSMenuItem alloc] init];
				[appMenu setSubmenu:[quitMenu autorelease]];
				
				[menu addItem:[appMenu autorelease]];
				
				if(_menu)
				{
					_menu->GetItems()->Enumerate<MenuItem>([&](MenuItem *item, size_t index, bool &stop) {
						
						NSMenu *tMenu = TranslateRNUIMenuToNSMenu(item->GetSubMenu());
						[tMenu setTitle:[NSString stringWithUTF8String:item->GetTitle()->GetUTF8String()]];
						
						NSMenuItem *menuItem = [[NSMenuItem alloc] init];
						[menuItem setSubmenu:tMenu];
						
						[menu addItem:[menuItem autorelease]];
						
					});
				}
				
				[NSApp setMainMenu:[menu autorelease]];
			}
		}
		
		void Server::PerformMenuBarAction(void *titem)
		{
			NSMenuItem *nsitem = (NSMenuItem *)titem;
			MenuItem *item = [(__RNUIMenuItemWrapper *)objc_getAssociatedObject(nsitem, __RNUIMenuItemWrapperKey) item];
			
			if(item && item->GetCallback())
			{
				item->GetCallback()(item);
			}
		}
		
#endif /* RN_PLATFORM_MAC_OS */

#if RN_PLATFORM_WINDOWS
#define kRNMaxAcceleratorEntries 1024

		struct AcceleratorTable
		{
			AcceleratorTable() :
				_table(new ACCEL[kRNMaxAcceleratorEntries]),
				_index(0)
			{}

			~AcceleratorTable()
			{
				delete [] _table;
			}

			std::string AddItem(MenuItem *item, size_t cmd)
			{
				RN_ASSERT(_index < 1024, "Too many keyboard shortcuts, maximum is %d", kRNMaxAcceleratorEntries);

				BYTE mask = FVIRTKEY;
				uint32 modifier = item->GetKeyEquivalentModifierMask();

				const String *string = item->GetKeyEquivalent();
				UniChar character = string->GetCharacterAtIndex(0);

				WORD key = 0;
				Range range(0, 1);

				if((character == 'F' || character == 'f') && string->GetLength() > 1)
				{
					UniChar fKey = string->GetCharacterAtIndex(1);
					if(fKey > '0' && fKey <= '9')
					{
						uint32 value = fKey - '0';
						range.length ++;
						
						if(string->GetLength() > 2)
						{
							fKey = string->GetCharacterAtIndex(2);
							value += (fKey >= '0' && fKey <= '9') ? (fKey - '0') * 10 : 0;

							if(value >= 10)
								range.length ++;
						}

						key = VK_F1 + (value - 1);
					}
					else
						goto useCharacter;
				}
				else
				{
				useCharacter:
					key = static_cast<WORD>(VkKeyScanEx(static_cast<CHAR>(character), nullptr) & 0xff);
				}

				mask |= (modifier & KeyModifier::KeyCommand || modifier & KeyModifier::KeyControl) ? FCONTROL : 0;
				mask |= (modifier & KeyModifier::KeyShift) ? FSHIFT : 0;
				mask |= (modifier & KeyModifier::KeyAlt) ? FALT : 0;

				_table[_index].cmd   = static_cast<WORD>(cmd);
				_table[_index].fVirt = mask;
				_table[_index].key   = key;

				_index ++;

				std::stringstream stream;
				stream << "\t";

				if(modifier & KeyModifier::KeyCommand || modifier & KeyModifier::KeyControl)
					stream << "ctrl+";
				if(modifier & KeyModifier::KeyAlt)
					stream << "alt+";
				if(modifier & KeyModifier::KeyShift)
					stream << "shift+";

				stream << string->GetSubstring(range)->GetUTF8String();
				return stream.str();
			}

			HACCEL Translate() const
			{
				HACCEL result = ::CreateAcceleratorTable(_table, _index);
				return result;
			}

		private:
			ACCEL *_table;
			size_t _index;
		};

		HMENU Server::TranslateRNUIToWinMenu(Menu *menu, AcceleratorTable &table, size_t &index)
		{
			HMENU hMenu = CreateMenu();

			menu->GetItems()->Enumerate<MenuItem>([&](MenuItem *item, size_t tindex, bool &stop) {

				size_t cmd = index ++;
				_menuTranslation->SetObjectForKey(item, Number::WithUint32(static_cast<uint32>(cmd)));

				if(item->IsSeparator())
				{
					::InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, cmd, nullptr);
				}
				else
				{
					if(item->GetSubMenu())
					{
						HMENU subMenu = TranslateRNUIToWinMenu(item->GetSubMenu(), table, index);
						::InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING | MF_POPUP, UINT(subMenu), item->GetTitle()->GetUTF8String());
					}
					else
					{
						std::stringstream title;
						title << item->GetTitle()->GetUTF8String();

						if(item->GetKeyEquivalent()->GetLength() > 0)
							title << table.AddItem(item, cmd);
						
						::InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, cmd, title.str().c_str());
					}
				}
			});

			return hMenu;
		}

		void Server::TranslateMenuToPlatform()
		{
			SafeRelease(_menuTranslation);
			_menuTranslation = new Dictionary();

			HMENU menu = ::CreateMenu();
			if(_menu)
			{
				AcceleratorTable table;
				size_t index = 0;

				_menu->GetItems()->Enumerate<MenuItem>([&](MenuItem *item, size_t tindex, bool &stop) {

					HMENU tMenu = TranslateRNUIToWinMenu(item->GetSubMenu(), table, index);
					::InsertMenu(menu, -1, MF_BYPOSITION | MF_STRING | MF_POPUP, UINT(tMenu), item->GetTitle()->GetUTF8String());

				});

				HACCEL accelerator = table.Translate();
				Kernel::GetSharedInstance()->UseAccelerator(accelerator);
			}

			HWND wnd = Kernel::GetSharedInstance()->GetMainWindow();
			::SetMenu(wnd, menu);
		}

		void Server::PerformMenuCommand(uint32 command)
		{
			MenuItem *item = _menuTranslation->GetObjectForKey<MenuItem>(Number::WithUint32(static_cast<uint32>(command)));
			if(item && item->GetCallback())
				item->GetCallback()(item);
		}
#endif
	}
}
