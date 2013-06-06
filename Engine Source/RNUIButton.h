//
//  RNUIButton.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIBUTTON_H__
#define __RAYNE_UIBUTTON_H__

#include "RNBase.h"
#include "RNMesh.h"
#include "RNUIImage.h"
#include "RNUIControl.h"

namespace RN
{
	namespace UI
	{
		class Button : public Control
		{
		public:
			Button();
			~Button() override;
			
			void SetImageForState(Image *image, State state);
			
		protected:
			void StateChanged(State state) override;
			bool Render(RenderingObject& object) override;
			
		private:
			void Initialize();
			bool ActivateImage(State state);
			void SetActiveImage(Image *image);
			
			std::map<State, Image *> _images;
			
			Image *_activeImage;
			Mesh  *_mesh;
			
			RNDefineMeta(Button, Control)
		};
	}
}

#endif /* __RAYNE_UIBUTTON_H__ */
