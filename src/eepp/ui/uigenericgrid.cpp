#include <eepp/ui/uigenericgrid.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIGenericGrid::UIGenericGrid( const UIGenericGrid::CreateParams& Params ) :
	UIComplexControl( Params ),
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
	mLastPos( eeINDEX_NOT_FOUND ),
	mVisibleFirst(0),
	mVisibleLast(0),
	mHScrollInit(0),
	mItemsNotVisible(0),
	mSelected(-1),
	mTouchDragAcceleration(0),
	mTouchDragDeceleration( Params.TouchDragDeceleration ),
	mCollWidthAssigned( false )
{
	mCollumnsWidth.resize( mCollumnsCount, 0 );
	mCollumnsPos.resize( mCollumnsCount, 0 );

	UIComplexControl::CreateParams CParams;
	CParams.Parent( this );
	CParams.PosSet( mPadding.Left, mPadding.Top );
	CParams.Size = Sizei( mSize.Width() - mPadding.Right - mPadding.Left, mSize.Height() - mPadding.Top - mPadding.Bottom );
	CParams.Flags = Params.Flags;
	mContainer = eeNew( UIItemContainer<UIGenericGrid> , ( CParams ) );
	mContainer->Visible( true );
	mContainer->Enabled( true );

	if ( mFlags & UI_CLIP_ENABLE )
		mFlags &= ~UI_CLIP_ENABLE;

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent				( this );
	ScrollBarP.PosSet				( mSize.Width() - 15, 0 );
	ScrollBarP.Flags				= UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar	= true;
	ScrollBarP.Size					= Sizei( 15, mSize.Height() );
	mVScrollBar						= eeNew( UIScrollBar, ( ScrollBarP ) );

	ScrollBarP.PosSet				( 0, mSize.Height() - 15 );
	ScrollBarP.Size					= Sizei( mSize.Width() - mVScrollBar->Size().Width(), 15 );
	ScrollBarP.VerticalScrollBar	= false;
	mHScrollBar						= eeNew( UIScrollBar, ( ScrollBarP ) );

	if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
		mHScrollBar->Visible( true );
		mHScrollBar->Enabled( true );
	}

	if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
		mVScrollBar->Visible( true );
		mVScrollBar->Enabled( true );
	}

	mVScrollBar->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIGenericGrid::OnScrollValueChange ) );
	mHScrollBar->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIGenericGrid::OnScrollValueChange ) );

	ApplyDefaultTheme();
}

UIGenericGrid::~UIGenericGrid() {
}

Uint32 UIGenericGrid::Type() const {
	return UI_TYPE_GENERICGRID;
}

bool UIGenericGrid::IsType( const Uint32& type ) const {
	return UIGenericGrid::Type() == type ? true : UIControlAnim::IsType( type );
}

void UIGenericGrid::SetDefaultCollumnsWidth() {
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

void UIGenericGrid::OnScrollValueChange( const UIEvent * Event ) {
	UpdateScroll( true );
}

void UIGenericGrid::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "genericgrid" );

	AutoPadding();

	OnSizeChange();
}

void UIGenericGrid::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding();
	}
}

void UIGenericGrid::OnSizeChange() {
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

void UIGenericGrid::ContainerResize() {
	mContainer->Pos( mPadding.Left, mPadding.Top );

	if( mHScrollBar->Visible() )
		mContainer->Size( mSize.Width() - mPadding.Right, mSize.Height() - mPadding.Top - mHScrollBar->Size().Height() );
	else
		mContainer->Size( mSize.Width() - mPadding.Right, mSize.Height() - mPadding.Bottom - mPadding.Top );

	if ( mVScrollBar->Visible() )
		mContainer->Size( mContainer->Size().Width() - mVScrollBar->Size().Width(), mContainer->Size().Height() );

	SetDefaultCollumnsWidth();
}

void UIGenericGrid::UpdateVScroll() {
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

void UIGenericGrid::UpdateHScroll() {
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

void UIGenericGrid::UpdateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	UIGridCell * Item;
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

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos ) && ( tHLastScroll == mHScrollInit ) )
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

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos )  && ( !Clipped || tHLastScroll == mHScrollInit ) )
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

void UIGenericGrid::UpdateSize() {
	UpdateCollumnsPos();

	mTotalHeight = mItems.size() * mRowHeight;
}

void UIGenericGrid::Add( UIGridCell * Cell ) {
	mItems.push_back( Cell );

	if ( mContainer != Cell->Parent() )
		Cell->Parent( mContainer );

	SetDefaultCollumnsWidth();

	Cell->AutoSize();

	UpdateSize();

	UpdateScroll();
}

void UIGenericGrid::Remove( UIGridCell * Cell ) {
	return Remove( GetItemIndex( Cell ) );
}

void UIGenericGrid::Remove( std::vector<Uint32> ItemsIndex ) {
	if ( ItemsIndex.size() && eeINDEX_NOT_FOUND != ItemsIndex[0] ) {
		std::vector<UIGridCell*> ItemsCpy;
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

void UIGenericGrid::Remove( Uint32 ItemIndex ) {
	Remove( std::vector<Uint32> ( 1, ItemIndex ) );
}

void UIGenericGrid::CollumnWidth( const Uint32& CollumnIndex, const Uint32& CollumnWidth ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	mCollWidthAssigned = true;

	mCollumnsWidth[ CollumnIndex ] = CollumnWidth;

	UpdateCollumnsPos();
	UpdateCells();
	UpdateScroll();
}

Uint32 UIGenericGrid::Count() const {
	return mItems.size();
}

const Uint32& UIGenericGrid::CollumnsCount() const {
	return mCollumnsCount;
}

const Uint32& UIGenericGrid::CollumnWidth( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsWidth[ CollumnIndex ];
}

void UIGenericGrid::RowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		UpdateSize();
		UpdateCells();
		UpdateScroll();
	}
}

const Uint32& UIGenericGrid::RowHeight() const {
	return mRowHeight;
}

UIGridCell * UIGenericGrid::GetCell( const Uint32& CellIndex ) const {
	eeASSERT( CellIndex < mItems.size() );

	return mItems[ CellIndex ];
}

Uint32 UIGenericGrid::GetCellPos( const Uint32& CollumnIndex ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsPos[ CollumnIndex ];
}

void UIGenericGrid::UpdateCells() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->FixCell();
	}
}

void UIGenericGrid::UpdateCollumnsPos() {
	Uint32 Pos = 0;

	for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
		mCollumnsPos[ i ] = Pos;

		Pos += mCollumnsWidth[ i ];
	}

	mTotalWidth = Pos;
}

void UIGenericGrid::OnAlphaChange() {
	UIControlAnim::OnAlphaChange();

	mVScrollBar->Alpha( mAlpha );
	mHScrollBar->Alpha( mAlpha );
}

void UIGenericGrid::VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		UpdateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIGenericGrid::VerticalScrollMode() {
	return mVScrollMode;
}

void UIGenericGrid::HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
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

const UI_SCROLLBAR_MODE& UIGenericGrid::HorizontalScrollMode() {
	return mHScrollMode;
}

UIScrollBar * UIGenericGrid::VerticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar * UIGenericGrid::HorizontalScrollBar() const {
	return mHScrollBar;
}

Uint32 UIGenericGrid::GetItemIndex( UIGridCell * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIGenericGrid::OnSelected() {
	SendCommonEvent( UIEvent::EventOnItemSelected );

	return 1;
}

UIGridCell * UIGenericGrid::GetItemSelected() {
	if ( -1 != mSelected )
		return mItems[ mSelected ];

	return NULL;
}

Uint32 UIGenericGrid::GetItemSelectedIndex() const {
	return mSelected;
}

Uint32 UIGenericGrid::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->FocusControl();

			if ( this != FocusCtrl && !IsParentOf( FocusCtrl ) ) {
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

UIItemContainer<UIGenericGrid> * UIGenericGrid::Container() const {
	return mContainer;
}

bool UIGenericGrid::TouchDragEnable() const {
	return 0 != ( mFlags & UI_TOUCH_DRAG_ENABLED );
}

void UIGenericGrid::TouchDragEnable( const bool& enable ) {
	WriteFlag( UI_TOUCH_DRAG_ENABLED, true == enable );
}

bool UIGenericGrid::TouchDragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING );
}

void UIGenericGrid::TouchDragging( const bool& dragging ) {
	WriteCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, true == dragging );
}

void UIGenericGrid::Update() {
	if ( mEnabled && mVisible ) {
		if ( mFlags & UI_TOUCH_DRAG_ENABLED ) {
			Uint32 Press	= UIManager::instance()->PressTrigger();
			Uint32 LPress	= UIManager::instance()->LastPressTrigger();

			if ( ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING ) ) {
				// Mouse Not Down
				if ( !( Press & EE_BUTTON_LMASK ) ) {
					WriteCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 0 );
					UIManager::instance()->SetControlDragging( false );
					return;
				}

				Vector2i Pos( UIManager::instance()->GetMousePos() );

				if ( mTouchDragPoint != Pos ) {
					Vector2i diff = -( mTouchDragPoint - Pos );

					mVScrollBar->Value( mVScrollBar->Value() + ( -diff.y / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

					mTouchDragAcceleration += Elapsed().AsMilliseconds() * diff.y * mTouchDragDeceleration;

					mTouchDragPoint = Pos;

					UIManager::instance()->SetControlDragging( true );
				} else {
					mTouchDragAcceleration -= Elapsed().AsMilliseconds() * mTouchDragAcceleration * 0.01f;
				}
			} else {
				// Mouse Down
				if ( IsMouseOverMeOrChilds() && !mVScrollBar->IsMouseOverMeOrChilds() && !mHScrollBar->IsMouseOverMeOrChilds() ) {
					if ( !( LPress & EE_BUTTON_LMASK ) && ( Press & EE_BUTTON_LMASK ) ) {
						WriteCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 1 );

						mTouchDragPoint			= UIManager::instance()->GetMousePos();
						mTouchDragAcceleration	= 0;
					}
				}

				// Mouse Up
				if ( ( LPress & EE_BUTTON_LMASK ) && !( Press & EE_BUTTON_LMASK ) ) {
					WriteCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 0 );
					UIManager::instance()->SetControlDragging( false );
				}

				// Deaccelerate
				if ( mTouchDragAcceleration > 0.01f || mTouchDragAcceleration < -0.01f ) {
					mVScrollBar->Value( mVScrollBar->Value() + ( -mTouchDragAcceleration / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

					mTouchDragAcceleration -= mTouchDragAcceleration * mTouchDragDeceleration * Elapsed().AsMilliseconds();
				}
			}
		}
	}

	UIComplexControl::Update();
}

}}
