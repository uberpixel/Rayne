//
//  RNUIGridView.h
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIGRIDVIEW_H_
#define __RAYNE_UIGRIDVIEW_H_

#include "RNUIScrollView.h"

namespace RN
{
	namespace UI
	{
		class GridView : public ScrollView
		{
		public:
			enum Direction
			{
				DirectionVertical,
				DirectionHorizontal
			};
			UIAPI GridView(Direction direction = DirectionVertical);
			UIAPI ~GridView();
			
			void SetMargins(float left, float top, float right, float bottom);
			void SetSpacing(float horizontal, float vertical);
			void SetReloadAll() { _reloadAll = true; }
			void SetToStart();

			UIAPI void Update(float delta, Vector2 cursorPosition, bool touched, Vector2 alternativeScrollSpeed = Vector2()) override;
			
		protected:
			UIAPI virtual void RecycleCell(View *cell);
			UIAPI virtual View *ReuseCell(size_t column, size_t row);
			UIAPI virtual View *CreateCell() = 0;
			
			UIAPI virtual size_t GetNumberOfRows() = 0;
			UIAPI virtual size_t GetNumberOfColumns() = 0;
			UIAPI virtual Vector2 GetCellSize() = 0;

		private:
			Array *_unusedCells;
			Array *_usedCells;
			
			Vector4 _margins;
			Vector2 _spacing;
			
			Direction _direction;
			bool _reloadAll;

			RNDeclareMetaAPI(GridView, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIGRIDVIEW_H_ */
