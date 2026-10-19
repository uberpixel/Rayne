//
//  RNInputIOS.cpp
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInputIOS.h"

#if RN_PLATFORM_IOS
#import <UIKit/UIKit.h>

@interface RNInputTouchView : UIView
{
@public
	RN::InputIOS *_input;
}
- (void)attachToWindow:(NSNotification *)notification;
- (void)applicationInactive:(NSNotification *)notification;
- (void)applicationActive:(NSNotification *)notification;
@end

@implementation RNInputTouchView
- (void)attachToWindow:(NSNotification *)notification
{
	// Keep the game window during native login UI.
	if(self.window && !self.window.hidden) return;
	UIWindow *target = nil;
	for(UIScene *scene in UIApplication.sharedApplication.connectedScenes)
	{
		if(![scene isKindOfClass:UIWindowScene.class] || scene.activationState == UISceneActivationStateBackground) continue;
		for(UIWindow *window in ((UIWindowScene *)scene).windows)
		{
			if(window.hidden || window.windowLevel != UIWindowLevelNormal) continue;
			if(!target || window.isKeyWindow) target = window;
			if(window.isKeyWindow) break;
		}
		if(target.isKeyWindow) break;
	}
	if(target)
	{
		_input->SetActive(false);
		self.frame = target.bounds;
		[target addSubview:self];
		_input->SetActive(UIApplication.sharedApplication.applicationState == UIApplicationStateActive);
	}
}
- (void)applicationInactive:(NSNotification *)notification
{
	if(_input) _input->SetActive(false);
}
- (void)applicationActive:(NSNotification *)notification
{
	[self attachToWindow:notification];
	if(_input) _input->SetActive(self.window && UIApplication.sharedApplication.applicationState == UIApplicationStateActive);
}
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if(_input) _input->HandleTouches(touches, RN::InputManager::TouchPhase::Began);
}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	if(_input) _input->HandleTouches(touches, RN::InputManager::TouchPhase::Moved);
}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	if(_input) _input->HandleTouches(touches, RN::InputManager::TouchPhase::Ended);
}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	if(_input) _input->HandleTouches(touches, RN::InputManager::TouchPhase::Cancelled);
}
@end

namespace RN
{
	InputIOS::InputIOS(InputManager *manager) : _manager(manager), _view(nullptr)
	{
		auto attach = ^{
			RNInputTouchView *view = [[RNInputTouchView alloc] initWithFrame:CGRectMake(0, 0, 0, 0)];
			_view = view;
			view->_input = this;
			view.multipleTouchEnabled = YES;
			view.exclusiveTouch = YES;
			view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
			NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
			[center addObserver:view selector:@selector(applicationInactive:) name:UIApplicationWillResignActiveNotification object:nil];
			[center addObserver:view selector:@selector(applicationActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
			[center addObserver:view selector:@selector(attachToWindow:) name:UIWindowDidBecomeVisibleNotification object:nil];
			[center addObserver:view selector:@selector(attachToWindow:) name:UIWindowDidBecomeKeyNotification object:nil];
			SetActive(false);
			[view attachToWindow:nil];
		};
		if(NSThread.isMainThread) attach();
		else dispatch_sync(dispatch_get_main_queue(), attach);
	}

	InputIOS::~InputIOS()
	{
		// Do not hold the input lock while waiting on the main thread.
		auto detach = ^{
			RNInputTouchView *view = static_cast<RNInputTouchView *>(_view);
			[NSNotificationCenter.defaultCenter removeObserver:view];
			view->_input = nullptr;
			SetActive(false);
			[view removeFromSuperview];
			[view release];
			_view = nullptr;
		};
		if(NSThread.isMainThread) detach();
		else dispatch_sync(dispatch_get_main_queue(), detach);
	}

	void InputIOS::SetActive(bool active)
	{
		LockGuard<Lockable> lock(_manager->_lock);
		_manager->_touchActive = active;
		_manager->_pendingTouches.clear();
		_manager->_touchEvents.clear();
		++_manager->_touchGeneration;
		_manager->_currentTouchCount = 0;
		_manager->_mouseMovement = Vector3();
	}

	void InputIOS::HandleTouches(void *nativeTouches, InputManager::TouchPhase phase)
	{
		// Publish each UIKit batch together.
		LockGuard<Lockable> lock(_manager->_lock);
		if(!_manager->_touchActive) return;
		UIView *view = static_cast<UIView *>(_view);
		CGSize size = view.bounds.size;
		for(UITouch *touch in static_cast<NSSet *>(nativeTouches))
		{
			CGPoint position = [touch locationInView:view];
			if(phase == InputManager::TouchPhase::Moved)
			{
				CGPoint previous = [touch previousLocationInView:view];
				// Keep integer mouse deltas and fractional touch positions.
				_manager->_mouseMovement = Vector3(static_cast<int>(position.x - previous.x), static_cast<int>(position.y - previous.y), 0);
			}
			else if(phase == InputManager::TouchPhase::Began) ++_manager->_currentTouchCount;
			else if(_manager->_currentTouchCount) --_manager->_currentTouchCount;
			_manager->_pendingTouches.push_back({phase, reinterpret_cast<uintptr_t>(touch), Vector2(position.x, position.y), Vector2(size.width, size.height)});
		}
	}
}
#endif
