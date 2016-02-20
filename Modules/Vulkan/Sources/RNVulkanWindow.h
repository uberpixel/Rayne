//
//  RNVulkanWindow.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANWINDOW_H_
#define __RAYNE_VULKANWINDOW_H_

#include <Rayne.h>

namespace RN
{
	class VulkanRenderer;
	class VulkanWindow : public Window
	{
	public:
		friend class VulkanRenderer;

		~VulkanWindow();

		void SetTitle(const String *title) final;
		Screen *GetScreen() final;

		void Show() final;
		void Hide() final;

		Vector2 GetSize() const final;

	private:
		VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer);

		HWND _hwnd;
		VulkanRenderer *_renderer;

		RNDeclareMeta(VulkanWindow)
	};
}


#endif /* __RAYNE_VULKANWINDOW_H_ */
