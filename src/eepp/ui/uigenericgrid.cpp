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
	CParams.setParent( this );
	CParams.setPos( mPadding.Left, mPadding.Top );
	CParams.Size = Sizei( mSize.getWidth() - mPadding.Right - mPadding.Left, mSize.getHeight() - mPadding.Top - mPadding.Bottom );
	CParams.Flags = Params.Flags;
	mContainer = eeNew( UIItemContainer<UIGenericGrid> , ( CParams ) );
	mContainer->setVisible( true );
	mContainer->setEnabled( true );

	if ( mFlags & UI_CLIP_ENABLE )
		mFlags &= ~UI_CLIP_ENABLE;

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.setParent				( this );
	ScrollBarP.setPos				( mSize.getWidth() - 15, 0 );
	ScrollBarP.Flags				= UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar	= true;
	ScrollBarP.Size					= Sizei( 15, mSize.getHeight() );
	mVScrollBar						= eeNew( UIScrollBar, ( ScrollBarP ) );

	ScrollBarP.setPos				( 0, mSize.getHeight() - 15 );
	ScrollBarP.Size					= Sizei( mSize.getWidth() - mVScrollBar->getSize().getWidth(), 15 );
	ScrollBarP.VerticalScrollBar	= false;
	mHScrollBar						= eeNew( UIScrollBar, ( ScrollBarP ) );

	if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
		mHScrollBar->setVisible( true );
		mHScrollBar->setEnabled( true );
	}

	if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
		mVScrollBar->setVisible( true );
		mVScrollBar->setEnabled( true );
	}

	mVScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIGenericGrid::onScrollValueChange ) );
	mHScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIGenericGrid::onScrollValueChange ) );

	applyDefaultTheme();
}

UIGenericGrid::~UIGenericGrid() {
}

Uint32 UIGenericGrid::getType() const {
	return UI_TYPE_GENERICGRID;
}

bool UIGenericGrid::isType( const Uint32& type ) const {
	return UIGenericGrid::getType() == type ? true : UIControlAnim::isType( type );
}

void UIGenericGrid::setDefaultCollumnsWidth() {
	if ( mCollWidthAssigned )
		return;

	if ( mItemsNotVisible <= 0 ) {
		Uint32 VisibleItems 	= mContainer->getSize().getHeight() / mRowHeight;
		Int32 oItemsNotVisible 	= (Int32)mItems.size() - VisibleItems;

		if ( oItemsNotVisible > 0 ) {
			mItemsNotVisible = oItemsNotVisible;

			updateVScroll();

			containerResize();
		}
	}

	Uint32 CollumnWidh = mContainer->getSize().getWidth() / mCollumnsCount;

	for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
		mCollumnsWidth[ i ] = CollumnWidh;
	}

	updateSize();
	updateCells();
}

void UIGenericGrid::onScrollValueChange( const UIEvent * Event ) {
	updateScroll( true );
}

void UIGenericGrid::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "genericgrid" );

	autoPadding();

	onSizeChange();
}

void UIGenericGrid::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding();
	}
}

void UIGenericGrid::onSizeChange() {
	mVScrollBar->setPosition( mSize.getWidth() - mVScrollBar->getSize().getWidth(), 0 );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth(), mSize.getHeight() );

	mHScrollBar->setPosition( 0, mSize.getHeight() - mHScrollBar->getSize().getHeight() );
	mHScrollBar->setSize( mSize.getWidth() - mVScrollBar->getSize().getWidth(), mHScrollBar->getSize().getHeight() );

	if ( mContainer->isClipped() && UI_SCROLLBAR_AUTO == mHScrollMode ) {
		if ( (Int32)mTotalWidth <= mContainer->getSize().getWidth() ) {
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			mHScrollInit = 0;
		}
	}

	containerResize();
	setDefaultCollumnsWidth();
	updateScroll();
}

void UIGenericGrid::containerResize() {
	mContainer->setPosition( mPadding.Left, mPadding.Top );

	if( mHScrollBar->isVisible() )
		mContainer->setSize( mSize.getWidth() - mPadding.Right, mSize.getHeight() - mPadding.Top - mHScrollBar->getSize().getHeight() );
	else
		mContainer->setSize( mSize.getWidth() - mPadding.Right, mSize.getHeight() - mPadding.Bottom - mPadding.Top );

	if ( mVScrollBar->isVisible() )
		mContainer->setSize( mContainer->getSize().getWidth() - mVScrollBar->getSize().getWidth(), mContainer->getSize().getHeight() );

	setDefaultCollumnsWidth();
}

void UIGenericGrid::updateVScroll() {
	if ( mItemsNotVisible <= 0 ) {
		if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
			mVScrollBar->setVisible( true );
			mVScrollBar->setEnabled( true );
		} else {
			mVScrollBar->setVisible( false );
			mVScrollBar->setEnabled( false );
		}
	} else {
		if ( UI_SCROLLBAR_AUTO == mVScrollMode || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
			mVScrollBar->setVisible( true );
			mVScrollBar->setEnabled( true );
		} else {
			mVScrollBar->setVisible( false );
			mVScrollBar->setEnabled( false );
		}
	}

	containerResize();
}

void UIGenericGrid::updateHScroll() {
	if ( mContainer->isClipped() && ( UI_SCROLLBAR_AUTO == mHScrollMode || UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) ) {
		if ( mContainer->getSize().getWidth() < (Int32)mTotalWidth ) {
				mHScrollBar->setVisible( true );
				mHScrollBar->setEnabled( true );

				containerResize();

				Int32 ScrollH = mTotalWidth - mContainer->getSize().getWidth();

				Int32 HScrolleable = (Uint32)( mHScrollBar->value() * ScrollH );

				mHScrollInit = -HScrolleable;
		} else {
			if ( UI_SCROLLBAR_AUTO == mHScrollMode ) {
				mHScrollBar->setVisible( false );
				mHScrollBar->setEnabled( false );

				mHScrollInit = 0;

				containerResize();
			}
		}
	}
}

void UIGenericGrid::updateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	UIGridCell * Item;
	Uint32 i, RelPos = 0, RelPosMax;
	Int32 ItemPos, ItemPosMax;
	Int32 tHLastScroll 		= mHScrollInit;

	Uint32 VisibleItems 	= mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible 		= (Int32)mItems.size() - VisibleItems;
	bool Clipped 			= 0 != mContainer->isClipped();

	updateVScroll();

	updateHScroll();

	VisibleItems 			= mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible 		= (Uint32)mItems.size() - VisibleItems;
	Int32 Scrolleable 		= (Int32)mItems.size() * mRowHeight - mContainer->getSize().getHeight();
	bool FirstVisible 		= false;

	if ( Clipped && mSmoothScroll ) {
		if ( Scrolleable >= 0 )
			RelPos 		= (Uint32)( mVScrollBar->value() * Scrolleable );
		else
			RelPos		= 0;

		RelPosMax 	= RelPos + mContainer->getSize().getHeight() + mRowHeight;

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos ) && ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * i;
			ItemPosMax = ItemPos + mRowHeight;

			if ( ( ItemPos >= (Int32)RelPos || ItemPosMax >= (Int32)RelPos ) && ( ItemPos <= (Int32)RelPosMax ) ) {
				Item->setPosition( mHScrollInit, ItemPos - RelPos );
				Item->setEnabled( true );
				Item->setVisible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				Item->setEnabled( false );
				Item->setVisible( false );
			}
		}
	} else {
		RelPosMax		= (Uint32)mItems.size();

		if ( mItemsNotVisible > 0 ) {
			RelPos 				= (Uint32)( mVScrollBar->value() * mItemsNotVisible );
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
					Item->setPosition( mHScrollInit, ItemPos );
				else
					Item->setPosition( 0, ItemPos );

				Item->setEnabled( true );
				Item->setVisible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				Item->setEnabled( false );
				Item->setVisible( false );
			}
		}
	}

	if ( mHScrollBar->isVisible() && !mVScrollBar->isVisible() ) {
		mHScrollBar->setPosition( 0, mSize.getHeight() - mHScrollBar->getSize().getHeight() );
		mHScrollBar->setSize( mSize.getWidth(), mHScrollBar->getSize().getHeight() );
	} else {
		mHScrollBar->setPosition( 0, mSize.getHeight() - mHScrollBar->getSize().getHeight() );
		mHScrollBar->setSize( mSize.getWidth() - mVScrollBar->getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	}
}

void UIGenericGrid::updateSize() {
	updateCollumnsPos();

	mTotalHeight = mItems.size() * mRowHeight;
}

void UIGenericGrid::add( UIGridCell * Cell ) {
	mItems.push_back( Cell );

	if ( mContainer != Cell->getParent() )
		Cell->setParent( mContainer );

	setDefaultCollumnsWidth();

	Cell->autoSize();

	updateSize();

	updateScroll();
}

void UIGenericGrid::remove( UIGridCell * Cell ) {
	return remove( getItemIndex( Cell ) );
}

void UIGenericGrid::remove( std::vector<Uint32> ItemsIndex ) {
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

		setDefaultCollumnsWidth();
		updateSize();
		updateScroll();
	}
}

void UIGenericGrid::remove( Uint32 ItemIndex ) {
	remove( std::vector<Uint32> ( 1, ItemIndex ) );
}

void UIGenericGrid::collumnWidth( const Uint32& CollumnIndex, const Uint32& CollumnWidth ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	mCollWidthAssigned = true;

	mCollumnsWidth[ CollumnIndex ] = CollumnWidth;

	updateCollumnsPos();
	updateCells();
	updateScroll();
}

Uint32 UIGenericGrid::getCount() const {
	return mItems.size();
}

const Uint32& UIGenericGrid::getCollumnsCount() const {
	return mCollumnsCount;
}

const Uint32& UIGenericGrid::collumnWidth( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsWidth[ CollumnIndex ];
}

void UIGenericGrid::rowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		updateSize();
		updateCells();
		updateScroll();
	}
}

const Uint32& UIGenericGrid::rowHeight() const {
	return mRowHeight;
}

UIGridCell * UIGenericGrid::getCell( const Uint32& CellIndex ) const {
	eeASSERT( CellIndex < mItems.size() );

	return mItems[ CellIndex ];
}

Uint32 UIGenericGrid::getCellPosition( const Uint32& CollumnIndex ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsPos[ CollumnIndex ];
}

void UIGenericGrid::updateCells() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->fixCell();
	}
}

void UIGenericGrid::updateCollumnsPos() {
	Uint32 Pos = 0;

	for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
		mCollumnsPos[ i ] = Pos;

		Pos += mCollumnsWidth[ i ];
	}

	mTotalWidth = Pos;
}

void UIGenericGrid::onAlphaChange() {
	UIControlAnim::onAlphaChange();

	mVScrollBar->setAlpha( mAlpha );
	mHScrollBar->setAlpha( mAlpha );
}

void UIGenericGrid::verticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		updateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIGenericGrid::verticalScrollMode() {
	return mVScrollMode;
}

void UIGenericGrid::horizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;

		if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
			containerResize();
		} else if ( UI_SCROLLBAR_ALWAYS_OFF == mHScrollMode ) {
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			containerResize();
		}

		updateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIGenericGrid::horizontalScrollMode() {
	return mHScrollMode;
}

UIScrollBar * UIGenericGrid::verticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar * UIGenericGrid::horizontalScrollBar() const {
	return mHScrollBar;
}

Uint32 UIGenericGrid::getItemIndex( UIGridCell * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIGenericGrid::onSelected() {
	sendCommonEvent( UIEvent::EventOnItemSelected );

	return 1;
}

UIGridCell * UIGenericGrid::getItemSelected() {
	if ( -1 != mSelected )
		return mItems[ mSelected ];

	return NULL;
}

Uint32 UIGenericGrid::getItemSelectedIndex() const {
	return mSelected;
}

Uint32 UIGenericGrid::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->focusControl();

			if ( this != FocusCtrl && !isParentOf( FocusCtrl ) ) {
				onComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

UIItemContainer<UIGenericGrid> * UIGenericGrid::getContainer() const {
	return mContainer;
}

bool UIGenericGrid::touchDragEnable() const {
	return 0 != ( mFlags & UI_TOUCH_DRAG_ENABLED );
}

void UIGenericGrid::touchDragEnable( const bool& enable ) {
	writeFlag( UI_TOUCH_DRAG_ENABLED, true == enable );
}

bool UIGenericGrid::touchDragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING );
}

void UIGenericGrid::touchDragging( const bool& dragging ) {
	writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, true == dragging );
}

void UIGenericGrid::update() {
	if ( mEnabled && mVisible ) {
		if ( mFlags & UI_TOUCH_DRAG_ENABLED ) {
			Uint32 Press	= UIManager::instance()->pressTrigger();
			Uint32 LPress	= UIManager::instance()->lastPressTrigger();

			if ( ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING ) ) {
				// Mouse Not Down
				if ( !( Press & EE_BUTTON_LMASK ) ) {
					writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 0 );
					UIManager::instance()->setControlDragging( false );
					return;
				}

				Vector2i Pos( UIManager::instance()->getMousePos() );

				if ( mTouchDragPoint != Pos ) {
					Vector2i diff = -( mTouchDragPoint - Pos );

					mVScrollBar->value( mVScrollBar->value() + ( -diff.y / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

					mTouchDragAcceleration += elapsed().asMilliseconds() * diff.y * mTouchDragDeceleration;

					mTouchDragPoint = Pos;

					UIManager::instance()->setControlDragging( true );
				} else {
					mTouchDragAcceleration -= elapsed().asMilliseconds() * mTouchDragAcceleration * 0.01f;
				}
			} else {
				// Mouse Down
				if ( isMouseOverMeOrChilds() && !mVScrollBar->isMouseOverMeOrChilds() && !mHScrollBar->isMouseOverMeOrChilds() ) {
					if ( !( LPress & EE_BUTTON_LMASK ) && ( Press & EE_BUTTON_LMASK ) ) {
						writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 1 );

						mTouchDragPoint			= UIManager::instance()->getMousePos();
						mTouchDragAcceleration	= 0;
					}
				}

				// Mouse Up
				if ( ( LPress & EE_BUTTON_LMASK ) && !( Press & EE_BUTTON_LMASK ) ) {
					writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, 0 );
					UIManager::instance()->setControlDragging( false );
				}

				// Deaccelerate
				if ( mTouchDragAcceleration > 0.01f || mTouchDragAcceleration < -0.01f ) {
					mVScrollBar->value( mVScrollBar->value() + ( -mTouchDragAcceleration / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

					mTouchDragAcceleration -= mTouchDragAcceleration * mTouchDragDeceleration * elapsed().asMilliseconds();
				}
			}
		}
	}

	UIComplexControl::update();
}

}}
