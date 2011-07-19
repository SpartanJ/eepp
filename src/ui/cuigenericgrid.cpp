#include "cuigenericgrid.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIGenericGrid::cUIGenericGrid( const cUIGenericGrid::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mPadding( Params.PaddingContainer ),
	mSmoothScroll( Params.SmoothScroll ),
	mContainer( NULL ),
	mVScrollBar( NULL ),
	mHScrollBar( NULL ),
	mVScrollMode( Params.VScrollMode ),
	mHScrollMode( Params.HScrollMode ),
	mCollumnsCount( Params.CollumnsCount ),
	mRowHeight( Params.RowHeight ),
	mTotalWidth( Params.GridWidth ),
	mLastPos( 0xFFFFFFFF ),
	mVisibleFirst(0),
	mVisibleLast(0),
	mHScrollInit(0),
	mItemsNotVisible(0),
	mSelected(-1),
	mCollWidthAssigned( false )
{
	mType = UI_TYPE_GENERICGRID;

	mCollumnsWidth.resize( mCollumnsCount, 0 );
	mCollumnsPos.resize( mCollumnsCount, 0 );

	cUIComplexControl::CreateParams CParams;
	CParams.Parent( this );
	CParams.PosSet( mPadding.Left, mPadding.Top );
	CParams.Size = eeSize( mSize.Width() - mPadding.Right - mPadding.Left, mSize.Height() - mPadding.Top - mPadding.Bottom );
	CParams.Flags = Params.Flags;
	mContainer = eeNew( tUIItemContainer<cUIGenericGrid> , ( CParams ) );
	mContainer->Visible( true );
	mContainer->Enabled( true );

	if ( mFlags & UI_CLIP_ENABLE )
		mFlags &= ~UI_CLIP_ENABLE;

	cUIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent				( this );
	ScrollBarP.PosSet				( mSize.Width() - 15, 0 );
	ScrollBarP.Flags				= UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar	= true;
	ScrollBarP.Size					= eeSize( 15, mSize.Height() );
	mVScrollBar						= eeNew( cUIScrollBar, ( ScrollBarP ) );

	ScrollBarP.PosSet				( 0, mSize.Height() - 15 );
	ScrollBarP.Size					= eeSize( mSize.Width() - mVScrollBar->Size().Width(), 15 );
	ScrollBarP.VerticalScrollBar	= false;
	mHScrollBar						= eeNew( cUIScrollBar, ( ScrollBarP ) );

	if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
		mHScrollBar->Visible( true );
		mHScrollBar->Enabled( true );
	}

	if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
		mVScrollBar->Visible( true );
		mVScrollBar->Enabled( true );
	}

	mVScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIGenericGrid::OnScrollValueChange ) );
	mHScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIGenericGrid::OnScrollValueChange ) );

	ApplyDefaultTheme();
}

cUIGenericGrid::~cUIGenericGrid() {
}

void cUIGenericGrid::SetDefaultCollumnsWidth() {
	if ( mCollWidthAssigned )
		return;

	if ( mItemsNotVisible <= 0 ) {
		Uint32 VisibleItems 	= mContainer->Size().Height() / mRowHeight;
		Int32 oItemsNotVisible 	= (Int32)mItems.size() - VisibleItems;

		if ( oItemsNotVisible > 0 ) {
			mItemsNotVisible = oItemsNotVisible;

			UpdateVScroll();

			ContainerResize();
		}
	}

	Uint32 CollumnWidh = mContainer->Size().Width() / mCollumnsCount;

	for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
		mCollumnsWidth[ i ] = CollumnWidh;
	}

	UpdateSize();
	UpdateCells();
}

void cUIGenericGrid::OnScrollValueChange( const cUIEvent * Event ) {
	UpdateScroll( true );
}

void cUIGenericGrid::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "genericgrid" );

	AutoPadding();

	OnSizeChange();
}

void cUIGenericGrid::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding();
	}
}

void cUIGenericGrid::OnSizeChange() {
	mVScrollBar->Pos( mSize.Width() - mVScrollBar->Size().Width(), 0 );
	mVScrollBar->Size( mVScrollBar->Size().Width(), mSize.Height() );

	mHScrollBar->Pos( 0, mSize.Height() - mHScrollBar->Size().Height() );
	mHScrollBar->Size( mSize.Width() - mVScrollBar->Size().Width(), mHScrollBar->Size().Height() );

	if ( mContainer->IsClipped() && UI_SCROLLBAR_AUTO == mHScrollMode ) {
		if ( (Int32)mTotalWidth <= mContainer->Size().Width() ) {
			mHScrollBar->Visible( false );
			mHScrollBar->Enabled( false );
			mHScrollInit = 0;
		}
	}

	ContainerResize();
	SetDefaultCollumnsWidth();
	UpdateScroll();
}

void cUIGenericGrid::ContainerResize() {
	mContainer->Pos( mPadding.Left, mPadding.Top );

	if( mHScrollBar->Visible() )
		mContainer->Size( mSize.Width() - mPadding.Right, mSize.Height() - mPadding.Top - mHScrollBar->Size().Height() );
	else
		mContainer->Size( mSize.Width() - mPadding.Right, mSize.Height() - mPadding.Bottom - mPadding.Top );

	if ( mVScrollBar->Visible() )
		mContainer->Size( mContainer->Size().Width() - mVScrollBar->Size().Width(), mContainer->Size().Height() );

	SetDefaultCollumnsWidth();
}

void cUIGenericGrid::UpdateVScroll() {
	if ( mItemsNotVisible <= 0 ) {
		if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
			mVScrollBar->Visible( true );
			mVScrollBar->Enabled( true );
		} else {
			mVScrollBar->Visible( false );
			mVScrollBar->Enabled( false );
		}
	} else {
		if ( UI_SCROLLBAR_AUTO == mVScrollMode || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
			mVScrollBar->Visible( true );
			mVScrollBar->Enabled( true );
		} else {
			mVScrollBar->Visible( false );
			mVScrollBar->Enabled( false );
		}
	}

	ContainerResize();
}

void cUIGenericGrid::UpdateHScroll() {
	if ( mContainer->IsClipped() && ( UI_SCROLLBAR_AUTO == mHScrollMode || UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) ) {
		if ( mContainer->Size().Width() < (Int32)mTotalWidth ) {
				mHScrollBar->Visible( true );
				mHScrollBar->Enabled( true );

				ContainerResize();

				Int32 ScrollH = mTotalWidth - mContainer->Size().Width();

				Int32 HScrolleable = (Uint32)( mHScrollBar->Value() * ScrollH );

				mHScrollInit = -HScrolleable;
		} else {
			if ( UI_SCROLLBAR_AUTO == mHScrollMode ) {
				mHScrollBar->Visible( false );
				mHScrollBar->Enabled( false );

				mHScrollInit = 0;

				ContainerResize();
			}
		}
	}
}

void cUIGenericGrid::UpdateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	cUIGridCell * Item;
	Uint32 i, RelPos = 0, RelPosMax;
	Int32 ItemPos, ItemPosMax;
	Int32 tHLastScroll 		= mHScrollInit;

	Uint32 VisibleItems 	= mContainer->Size().Height() / mRowHeight;
	mItemsNotVisible 		= (Int32)mItems.size() - VisibleItems;
	bool Clipped 			= 0 != mContainer->IsClipped();

	UpdateVScroll();

	UpdateHScroll();

	VisibleItems 			= mContainer->Size().Height() / mRowHeight;
	mItemsNotVisible 		= (Uint32)mItems.size() - VisibleItems;
	Int32 Scrolleable 		= (Int32)mItems.size() * mRowHeight - mContainer->Size().Height();
	bool FirstVisible 		= false;

	if ( Clipped && mSmoothScroll ) {
		if ( Scrolleable >= 0 )
			RelPos 		= (Uint32)( mVScrollBar->Value() * Scrolleable );
		else
			RelPos		= 0;

		RelPosMax 	= RelPos + mContainer->Size().Height() + mRowHeight;

		if ( ( FromScrollChange && 0xFFFFFFFF != mLastPos && mLastPos == RelPos ) && ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * i;
			ItemPosMax = ItemPos + mRowHeight;

			if ( ( ItemPos >= (Int32)RelPos || ItemPosMax >= (Int32)RelPos ) && ( ItemPos <= (Int32)RelPosMax ) ) {
				Item->Pos( mHScrollInit, ItemPos - RelPos );
				Item->Enabled( true );
				Item->Visible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				Item->Enabled( false );
				Item->Visible( false );
			}
		}
	} else {
		RelPosMax		= (Uint32)mItems.size();

		if ( mItemsNotVisible > 0 ) {
			RelPos 				= (Uint32)( mVScrollBar->Value() * mItemsNotVisible );
			RelPosMax			= RelPos + VisibleItems;
		}

		if ( ( FromScrollChange && 0xFFFFFFFF != mLastPos && mLastPos == RelPos )  && ( !Clipped || tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * ( i - RelPos );

			if ( i >= RelPos && i < RelPosMax ) {
				if ( Clipped )
					Item->Pos( mHScrollInit, ItemPos );
				else
					Item->Pos( 0, ItemPos );

				Item->Enabled( true );
				Item->Visible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				Item->Enabled( false );
				Item->Visible( false );
			}
		}
	}

	if ( mHScrollBar->Visible() && !mVScrollBar->Visible() ) {
		mHScrollBar->Pos( 0, mSize.Height() - mHScrollBar->Size().Height() );
		mHScrollBar->Size( mSize.Width(), mHScrollBar->Size().Height() );
	} else {
		mHScrollBar->Pos( 0, mSize.Height() - mHScrollBar->Size().Height() );
		mHScrollBar->Size( mSize.Width() - mVScrollBar->Size().Width(), mHScrollBar->Size().Height() );
	}
}

void cUIGenericGrid::UpdateSize() {
	UpdateCollumnsPos();

	mTotalHeight = mItems.size() * mRowHeight;
}

void cUIGenericGrid::Add( cUIGridCell * Cell ) {
	mItems.push_back( Cell );

	if ( mContainer != Cell->Parent() )
		Cell->Parent( mContainer );

	SetDefaultCollumnsWidth();

	Cell->AutoSize();

	UpdateSize();

	UpdateScroll();
}

void cUIGenericGrid::Remove( cUIGridCell * Cell ) {
	return Remove( GetItemIndex( Cell ) );
}

void cUIGenericGrid::Remove( std::vector<Uint32> ItemsIndex ) {
	if ( ItemsIndex.size() && 0xFFFFFFFF != ItemsIndex[0] ) {
		std::vector<cUIGridCell*> ItemsCpy;
		bool erase;

		for ( Uint32 i = 0; i < mItems.size(); i++ ) {
			erase = false;

			for ( Uint32 z = 0; z < ItemsIndex.size(); z++ ) {
				if ( ItemsIndex[z] == i ) {
					if ( (Int32)ItemsIndex[z] == mSelected )
						mSelected = -1;

					ItemsIndex.erase( ItemsIndex.begin() + z );

					erase = true;

					break;
				}
			}

			if ( !erase ) {
				ItemsCpy.push_back( mItems[i] );
			} else {
				eeSAFE_DELETE( mItems[i] ); // doesn't call to mItems[i]->Close(); because is not checking for close.
			}
		}

		mItems = ItemsCpy;

		SetDefaultCollumnsWidth();
		UpdateSize();
		UpdateScroll();
	}
}

void cUIGenericGrid::Remove( Uint32 ItemIndex ) {
	Remove( std::vector<Uint32> ( 1, ItemIndex ) );
}

void cUIGenericGrid::CollumnWidth( const Uint32& CollumnIndex, const Uint32& CollumnWidth ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	mCollWidthAssigned = true;

	mCollumnsWidth[ CollumnIndex ] = CollumnWidth;

	UpdateCollumnsPos();
	UpdateCells();
	UpdateScroll();
}

Uint32 cUIGenericGrid::Count() const {
	return mItems.size();
}

const Uint32& cUIGenericGrid::CollumnsCount() const {
	return mCollumnsCount;
}

const Uint32& cUIGenericGrid::CollumnWidth( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsWidth[ CollumnIndex ];
}

void cUIGenericGrid::RowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		UpdateSize();
		UpdateCells();
		UpdateScroll();
	}
}

const Uint32& cUIGenericGrid::RowHeight() const {
	return mRowHeight;
}

cUIGridCell * cUIGenericGrid::GetCell( const Uint32& CellIndex ) const {
	eeASSERT( CellIndex < mItems.size() );

	return mItems[ CellIndex ];
}

Uint32 cUIGenericGrid::GetCellPos( const Uint32& CollumnIndex ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsPos[ CollumnIndex ];
}

void cUIGenericGrid::UpdateCells() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->FixCell();
	}
}

void cUIGenericGrid::UpdateCollumnsPos() {
	Uint32 Pos = 0;

	for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
		mCollumnsPos[ i ] = Pos;

		Pos += mCollumnsWidth[ i ];
	}

	mTotalWidth = Pos;
}

void cUIGenericGrid::OnAlphaChange() {
	cUIControlAnim::OnAlphaChange();

	mVScrollBar->Alpha( mAlpha );
	mHScrollBar->Alpha( mAlpha );
}

void cUIGenericGrid::VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		UpdateScroll();
	}
}

const UI_SCROLLBAR_MODE& cUIGenericGrid::VerticalScrollMode() {
	return mVScrollMode;
}

void cUIGenericGrid::HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;

		if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
			mHScrollBar->Visible( true );
			mHScrollBar->Enabled( true );
			ContainerResize();
		} else if ( UI_SCROLLBAR_ALWAYS_OFF == mHScrollMode ) {
			mHScrollBar->Visible( false );
			mHScrollBar->Enabled( false );
			ContainerResize();
		}

		UpdateScroll();
	}
}

const UI_SCROLLBAR_MODE& cUIGenericGrid::HorizontalScrollMode() {
	return mHScrollMode;
}

cUIScrollBar * cUIGenericGrid::VerticalScrollBar() const {
	return mVScrollBar;
}

cUIScrollBar * cUIGenericGrid::HorizontalScrollBar() const {
	return mHScrollBar;
}

Uint32 cUIGenericGrid::GetItemIndex( cUIGridCell * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return 0xFFFFFFFF;
}

Uint32 cUIGenericGrid::OnSelected() {
	SendCommonEvent( cUIEvent::EventOnItemSelected );

	return 1;
}

cUIGridCell * cUIGenericGrid::GetItemSelected() {
	if ( -1 != mSelected )
		return mItems[ mSelected ];

	return NULL;
}

Uint32 cUIGenericGrid::GetItemSelectedIndex() const {
	return mSelected;
}

Uint32 cUIGenericGrid::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgFocusLoss:
		{
			cUIControl * FocusCtrl = cUIManager::instance()->FocusControl();

			if ( this != FocusCtrl && !IsParentOf( FocusCtrl ) ) {
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

tUIItemContainer<cUIGenericGrid> * cUIGenericGrid::Container() const {
	return mContainer;
}

}}
