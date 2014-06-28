#ifndef EE_UICUIGENERICGRID_HPP
#define EE_UICUIGENERICGRID_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uigridcell.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiitemcontainer.hpp>

namespace EE { namespace UI {

class EE_API UIGenericGrid : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					SmoothScroll( true ),
					VScrollMode( UI_SCROLLBAR_AUTO ),
					HScrollMode( UI_SCROLLBAR_AUTO ),
					CollumnsCount(1),
					RowHeight( 24 ),
					GridWidth( 0 ),
					TouchDragDeceleration( 0.01f )
				{
				}

				inline ~CreateParams() {}

				bool				SmoothScroll;
				UI_SCROLLBAR_MODE	VScrollMode;
				UI_SCROLLBAR_MODE	HScrollMode;
				Uint32				CollumnsCount;
				Uint32				RowHeight;
				Uint32				GridWidth;
				Recti				PaddingContainer;
				Float				TouchDragDeceleration;
		};

		UIGenericGrid( const UIGenericGrid::CreateParams& Params );

		~UIGenericGrid();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		void Add( UIGridCell * Cell );

		void Remove( UIGridCell * Cell );

		void Remove( std::vector<Uint32> ItemsIndex );

		void Remove( Uint32 ItemIndex );

		void CollumnWidth( const Uint32& CollumnIndex, const Uint32& CollumnWidth );

		Uint32 Count() const;

		const Uint32& CollumnsCount() const;

		const Uint32& CollumnWidth( const Uint32& CollumnIndex ) const;

		void RowHeight( const Uint32& height );

		const Uint32& RowHeight() const;

		UIGridCell * GetCell( const Uint32& CellIndex ) const;

		void VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& VerticalScrollMode();

		void HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& HorizontalScrollMode();

		Uint32 GetCellPos( const Uint32& CollumnIndex );

		UIScrollBar * VerticalScrollBar() const;

		UIScrollBar * HorizontalScrollBar() const;

		Uint32 GetItemIndex( UIGridCell * Item );

		UIGridCell * GetItemSelected();

		Uint32 GetItemSelectedIndex() const;

		Uint32 OnMessage( const UIMessage * Msg );

		UIItemContainer<UIGenericGrid> * Container() const;

		virtual void Update();

		bool TouchDragEnable() const;

		void TouchDragEnable( const bool& enable );

		bool TouchDragging() const;

		void TouchDragging( const bool& dragging );
	protected:
		friend class UIItemContainer<UIGenericGrid>;
		friend class UIGridCell;

		Recti						mPadding;
		bool						mSmoothScroll;
		UIItemContainer<UIGenericGrid> *	mContainer;
		UIScrollBar *				mVScrollBar;
		UIScrollBar *				mHScrollBar;
		UI_SCROLLBAR_MODE			mVScrollMode;
		UI_SCROLLBAR_MODE			mHScrollMode;
		std::vector<UIGridCell*>	mItems;
		Uint32						mCollumnsCount;
		Uint32						mRowHeight;
		std::vector<Uint32>			mCollumnsWidth;
		std::vector<Uint32>			mCollumnsPos;
		Uint32						mTotalWidth;
		Uint32						mTotalHeight;
		Uint32						mLastPos;
		Uint32						mVisibleFirst;
		Uint32						mVisibleLast;
		Int32						mHScrollInit;
		Int32						mItemsNotVisible;
		Int32						mSelected;

		Vector2i					mTouchDragPoint;
		Float						mTouchDragAcceleration;
		Float						mTouchDragDeceleration;

		bool						mCollWidthAssigned;

		void UpdateCells();

		void UpdateCollumnsPos();

		void AutoPadding();

		virtual void OnSizeChange();

		virtual void OnAlphaChange();

		void ContainerResize();

		void OnScrollValueChange( const UIEvent * Event );

		void SetDefaultCollumnsWidth();

		void UpdateScroll( bool FromScrollChange = false );

		void UpdateSize();

		virtual Uint32 OnSelected();

		void UpdateVScroll();

		void UpdateHScroll();
};

}}

#endif
