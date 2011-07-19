#ifndef EE_UICUIGENERICGRID_HPP
#define EE_UICUIGENERICGRID_HPP

#include "cuicontrolanim.hpp"
#include "cuigridcell.hpp"
#include "cuiscrollbar.hpp"
#include "tuiitemcontainer.hpp"

namespace EE { namespace UI {

class EE_API cUIGenericGrid : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					cUIControlAnim::CreateParams(),
					SmoothScroll( true ),
					VScrollMode( UI_SCROLLBAR_AUTO ),
					HScrollMode( UI_SCROLLBAR_AUTO ),
					CollumnsCount(1),
					RowHeight( 24 ),
					GridWidth( 0 )
				{
				}

				inline ~CreateParams() {}

				bool				SmoothScroll;
				UI_SCROLLBAR_MODE	VScrollMode;
				UI_SCROLLBAR_MODE	HScrollMode;
				Uint32				CollumnsCount;
				Uint32				RowHeight;
				Uint32				GridWidth;
				eeRecti				PaddingContainer;
		};

		cUIGenericGrid( const cUIGenericGrid::CreateParams& Params );

		~cUIGenericGrid();

		virtual void SetTheme( cUITheme * Theme );

		void Add( cUIGridCell * Cell );

		void Remove( cUIGridCell * Cell );

		void Remove( std::vector<Uint32> ItemsIndex );

		void Remove( Uint32 ItemIndex );

		void CollumnWidth( const Uint32& CollumnIndex, const Uint32& CollumnWidth );

		Uint32 Count() const;

		const Uint32& CollumnsCount() const;

		const Uint32& CollumnWidth( const Uint32& CollumnIndex ) const;

		void RowHeight( const Uint32& height );

		const Uint32& RowHeight() const;

		cUIGridCell * GetCell( const Uint32& CellIndex ) const;

		void VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& VerticalScrollMode();

		void HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode );

		const UI_SCROLLBAR_MODE& HorizontalScrollMode();

		Uint32 GetCellPos( const Uint32& CollumnIndex );

		cUIScrollBar * VerticalScrollBar() const;

		cUIScrollBar * HorizontalScrollBar() const;

		Uint32 GetItemIndex( cUIGridCell * Item );

		cUIGridCell * GetItemSelected();

		Uint32 GetItemSelectedIndex() const;

		Uint32 OnMessage( const cUIMessage * Msg );

		tUIItemContainer<cUIGenericGrid> * Container() const;
	protected:
		friend class tUIItemContainer<cUIGenericGrid>;
		friend class cUIGridCell;

		eeRecti						mPadding;
		bool						mSmoothScroll;
		tUIItemContainer<cUIGenericGrid> *	mContainer;
		cUIScrollBar *				mVScrollBar;
		cUIScrollBar *				mHScrollBar;
		UI_SCROLLBAR_MODE			mVScrollMode;
		UI_SCROLLBAR_MODE			mHScrollMode;
		std::vector<cUIGridCell*>	mItems;
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
		bool						mCollWidthAssigned;

		void UpdateCells();

		void UpdateCollumnsPos();

		void AutoPadding();

		virtual void OnSizeChange();

		virtual void OnAlphaChange();

		void ContainerResize();

		void OnScrollValueChange( const cUIEvent * Event );

		void SetDefaultCollumnsWidth();

		void UpdateScroll( bool FromScrollChange = false );

		void UpdateSize();

		virtual Uint32 OnSelected();

		void UpdateVScroll();

		void UpdateHScroll();
};

}}

#endif
