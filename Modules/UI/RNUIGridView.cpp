//
//  RNUIGridView.cpp
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIGridView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(GridView, ScrollView)

		GridView::GridView(Direction direction) : ScrollView(direction == DirectionVertical, direction == DirectionHorizontal), _direction(direction), _reloadAll(false)
		{
			_usedCells = new Array();
			_unusedCells = new Array();
		}

		GridView::~GridView()
		{
			
		}

		void GridView::Update(float delta, Vector2 cursorPosition, bool touched, Vector2 alternativeScrollSpeed)
		{
			ScrollView::Update(delta, cursorPosition, touched, alternativeScrollSpeed);
			
			Vector2 cellSize = GetCellSize();
			size_t columnCount = GetNumberOfColumns();
			size_t rowCount = GetNumberOfRows();
			
			//Figure out which rows should be visible
			Rect visibleRect = GetFrame();
			visibleRect.x = -GetBounds().x;
			visibleRect.y = -GetBounds().y;
			
			RN::Rect bounds = GetBounds();
			bounds.width = _margins.x + _margins.z + columnCount * cellSize.x + (columnCount > 0? columnCount-1 : 0) * _spacing.x;
			bounds.height = _margins.y + _margins.w + rowCount * cellSize.y + (rowCount > 0? rowCount-1 : 0) * _spacing.y;
			SetBounds(bounds);
			
			Vector2 spacedCellSize = cellSize + _spacing;
			
			int leftColumnIndex = (visibleRect.x - _margins.x) / spacedCellSize.x;
			int rightColumnIndex = (visibleRect.x - _margins.x + visibleRect.width) / spacedCellSize.x;
			
			int topRowIndex = (visibleRect.y - _margins.y) / spacedCellSize.y;
			int bottomRowIndex = (visibleRect.y - _margins.y + visibleRect.height) / spacedCellSize.y;
			
			int availableLeftColumnIndex = -1;
			int availableRightColumnIndex = -1;
			int availableTopRowIndex = -1;
			int availableBottomRowIndex = -1;
			
			//Recycle cells that aren't visible anymore
			std::vector<View *>cellsToRemove;
			_usedCells->Enumerate<View>([&](View *cell, size_t index, bool &stop){
				int cellColumn = (cell->GetFrame().x - _margins.x) / spacedCellSize.x;
				int cellRow = (cell->GetFrame().y - _margins.y) / spacedCellSize.y;
				
				if(_reloadAll || !cell->GetFrame().IntersectsRect(visibleRect) || cellColumn >= columnCount || cellRow >= rowCount)
				{
					cellsToRemove.push_back(cell);
				}
				else
				{
					if(cellColumn < availableLeftColumnIndex || availableLeftColumnIndex == -1)
					{
						availableLeftColumnIndex = cellColumn;
					}
					
					if(cellColumn > availableRightColumnIndex || availableRightColumnIndex == -1)
					{
						availableRightColumnIndex = cellColumn;
					}
					
					if(cellRow < availableTopRowIndex || availableTopRowIndex == -1)
					{
						availableTopRowIndex = cellRow;
					}
					
					if(cellRow > availableBottomRowIndex || availableBottomRowIndex == -1)
					{
						availableBottomRowIndex = cellRow;
					}
				}
			});
			
			_reloadAll = false;
			
			for(View *cell : cellsToRemove)
			{
				RecycleCell(cell);
			}
			
			//Load newly visible cells
			for(size_t row = std::max(topRowIndex, 0); row <= bottomRowIndex; row++)
			{
				if(row >= rowCount) break;
				
				for(size_t column = std::max(leftColumnIndex, 0); column <= rightColumnIndex; column++)
				{
					if(column >= columnCount) break;
					if(row >= availableTopRowIndex && row <= availableBottomRowIndex && availableTopRowIndex != -1 && column >= availableLeftColumnIndex && column <= availableRightColumnIndex && availableLeftColumnIndex != -1)
					{
						continue;
					}
					
					View *cell = ReuseCell(column, row);
					cell->SetFrame(RN::Rect(_margins.x + column * cellSize.x + column * _spacing.x, _margins.y + row * cellSize.y + row * _spacing.y, cellSize.x, cellSize.y));
				}
			}
		}
	
		void GridView::RecycleCell(View *cell)
		{
			_unusedCells->AddObject(cell);
			_usedCells->RemoveObject(cell);
			cell->RemoveFromSuperview();
		}
		
		View *GridView::ReuseCell(size_t column, size_t row)
		{
			View *cell = nullptr;
			if(_unusedCells->GetCount() > 0)
			{
				cell = _unusedCells->GetLastObject<View>()->Retain();
				_unusedCells->RemoveObjectAtIndex(_unusedCells->GetCount()-1);
			}
			
			if(!cell)
			{
				cell = CreateCell();
			}
			
			AddSubview(cell);
			_usedCells->AddObject(cell);
			cell->Release();
			
			return cell;
		}
	
		void GridView::SetMargins(float left, float top, float right, float bottom)
		{
			_margins = Vector4(left, top, right, bottom);
			_reloadAll = true;
		}
	
		void GridView::SetSpacing(float horizontal, float vertical)
		{
			_spacing = Vector2(horizontal, vertical);
			_reloadAll = true;
		}
	}
}
