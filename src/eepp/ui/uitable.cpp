#include <eepp/ui/uitable.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITable * UITable::New() {
	return eeNew( UITable, () );
}

UITable::UITable() :
	UITouchDragableWidget( "table" ),
	mContainerPadding(),
	mContainer( NULL ),
	mVScrollBar( NULL ),
	mHScrollBar( NULL ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mCollumnsCount( 0 ),
	mRowHeight( 0 ),
	mTotalWidth( 0 ),
	mLastPos( eeINDEX_NOT_FOUND ),
	mVisibleFirst(0),
	mVisibleLast(0),
	mHScrollInit(0),
	mItemsNotVisible(0),
	mSelected(-1),
	mSmoothScroll( false ),
	mCollWidthAssigned( false )
{
	setFlags( UI_AUTO_PADDING );

	mContainer = eeNew( UIItemContainer<UITable> , () );
	mContainer->setVisible( true );
	mContainer->setEnabled( true );
	mContainer->setParent( this );
	mContainer->setPosition( mContainerPadding.Left, mContainerPadding.Top );
	mContainer->setSize( getSize().getWidth() - mContainerPadding.Right - mContainerPadding.Left, getSize().getHeight() - mContainerPadding.Top - mContainerPadding.Bottom );

	mVScrollBar = UIScrollBar::New();
	mVScrollBar->setOrientation( UIOrientation::Vertical );
	mVScrollBar->setParent( this );
	mVScrollBar->setPosition( getSize().getWidth() - 16, 0 );
	mVScrollBar->setSize( 16, getSize().getHeight() );

	mHScrollBar = UIScrollBar::New();
	mHScrollBar->setOrientation( UIOrientation::Horizontal );
	mHScrollBar->setParent( this );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 16 );
	mHScrollBar->setPosition( 0, getSize().getHeight() - 16 );

	mHScrollBar->setVisible( ScrollBarMode::AlwaysOn == mHScrollMode );
	mHScrollBar->setEnabled( ScrollBarMode::AlwaysOn == mHScrollMode );
	mVScrollBar->setVisible( ScrollBarMode::AlwaysOn == mVScrollMode );
	mVScrollBar->setEnabled( ScrollBarMode::AlwaysOn == mVScrollMode );

	mVScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &UITable::onScrollValueChange ) );
	mHScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &UITable::onScrollValueChange ) );

	applyDefaultTheme();
}

UITable::~UITable() {
}

Uint32 UITable::getType() const {
	return UI_TYPE_TABLE;
}

bool UITable::isType( const Uint32& type ) const {
	return UITable::getType() == type ? true : UITouchDragableWidget::isType( type );
}

void UITable::setDefaultCollumnsWidth() {
	if ( mCollWidthAssigned || 0 == mCollumnsCount || 0 == mRowHeight )
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
		if ( 0 == mCollumnsWidth[ i ] )
			mCollumnsWidth[ i ] = CollumnWidh;
	}

	updateSize();
	updateCells();
}

void UITable::onScrollValueChange( const Event * ) {
	updateScroll( true );
}

void UITable::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "genericgrid" );

	autoPadding();

	onSizeChange();

	onThemeLoaded();
}

void UITable::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mContainerPadding = PixelDensity::dpToPx( makePadding() );
	}
}

void UITable::onSizeChange() {
	mVScrollBar->setPosition( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 0 );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth(), getSize().getHeight() );

	mHScrollBar->setPosition( 0, getSize().getHeight() - mHScrollBar->getSize().getHeight() );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), mHScrollBar->getSize().getHeight() );

	if ( mContainer->isClipped() && ScrollBarMode::Auto == mHScrollMode ) {
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

void UITable::containerResize() {
	Rectf padding = mContainerPadding + mRealPadding;

	mContainer->setPosition( padding.Left, padding.Top );

	if( mHScrollBar->isVisible() )
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left, mSize.getHeight() - padding.Top - mHScrollBar->getPixelsSize().getHeight() );
	else
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left, mSize.getHeight() - padding.Bottom - padding.Top );

	if ( mVScrollBar->isVisible() )
		mContainer->setPixelsSize( mContainer->getPixelsSize().getWidth() - mVScrollBar->getPixelsSize().getWidth(), mContainer->getPixelsSize().getHeight() );

	setDefaultCollumnsWidth();
}

void UITable::updateVScroll() {
	if ( mItemsNotVisible <= 0 ) {
		if ( ScrollBarMode::AlwaysOn == mVScrollMode ) {
			mVScrollBar->setVisible( true );
			mVScrollBar->setEnabled( true );
		} else {
			mVScrollBar->setVisible( false );
			mVScrollBar->setEnabled( false );
		}
	} else {
		if ( ScrollBarMode::Auto == mVScrollMode || ScrollBarMode::AlwaysOn == mVScrollMode ) {
			mVScrollBar->setVisible( true );
			mVScrollBar->setEnabled( true );
		} else {
			mVScrollBar->setVisible( false );
			mVScrollBar->setEnabled( false );
		}
	}

	containerResize();
}

void UITable::updateHScroll() {
	if ( mContainer->isClipped() && ( ScrollBarMode::Auto == mHScrollMode || ScrollBarMode::AlwaysOn == mHScrollMode ) ) {
		if ( mContainer->getSize().getWidth() < (Int32)mTotalWidth ) {
				mHScrollBar->setVisible( true );
				mHScrollBar->setEnabled( true );

				containerResize();

				Int32 ScrollH = mTotalWidth - mContainer->getSize().getWidth();

				Int32 HScrolleable = (Uint32)( mHScrollBar->getValue() * ScrollH );

				mHScrollInit = -HScrolleable;
		} else {
			if ( ScrollBarMode::Auto == mHScrollMode ) {
				mHScrollBar->setVisible( false );
				mHScrollBar->setEnabled( false );

				mHScrollInit = 0;

				containerResize();
			}
		}
	}
}

void UITable::setHScrollStep() {
	Float width = (Float)mContainer->getPixelsSize().getWidth();

	if ( ( mItemsNotVisible > 0 && ScrollBarMode::Auto == mVScrollMode ) || ScrollBarMode::AlwaysOn == mVScrollMode )
		width -= mVScrollBar->getPixelsSize().getWidth();

	Float maxWidth = 0;

	if ( mCollumnsCount > 0 ) {
		for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
			maxWidth += mCollumnsWidth[i];
		}
	}

	Float stepVal = width / (Float)maxWidth;

	mHScrollBar->setPageStep( stepVal );

	mHScrollBar->setClickStep( stepVal );
}

void UITable::updateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	UITableCell * Item;
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
			RelPos 		= (Uint32)( mVScrollBar->getValue() * Scrolleable );
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
				Item->setPosition( mHScrollInit, ItemPos - (Int32)RelPos );
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
			RelPos 				= (Uint32)( mVScrollBar->getValue() * mItemsNotVisible );
			RelPosMax			= RelPos + VisibleItems;
		}

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos )  && ( !Clipped || tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * ( (Int32)i - (Int32)RelPos );

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
		mHScrollBar->setPosition( 0, getSize().getHeight() - mHScrollBar->getSize().getHeight() );
		mHScrollBar->setSize( getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	} else {
		mHScrollBar->setPosition( 0, getSize().getHeight() - mHScrollBar->getSize().getHeight() );
		mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), mHScrollBar->getSize().getHeight() );
	}

	setHScrollStep();
}

void UITable::updateSize() {
	updateCollumnsPos();

	mTotalHeight = mItems.size() * mRowHeight;
}

void UITable::add( UITableCell * Cell ) {
	Cell->setParent( getContainer() );

	mItems.push_back( Cell );

	if ( mContainer != Cell->getParent() )
		Cell->setParent( mContainer );

	setDefaultCollumnsWidth();

	Cell->onAutoSize();

	updateSize();

	updateScroll();

	mVScrollBar->setPageStep( ( (Float)mContainer->getSize().getHeight() /(Float) mRowHeight ) / (Float)mItems.size() );
}

void UITable::remove( UITableCell * Cell ) {
	return remove( getItemIndex( Cell ) );
}

void UITable::remove( std::vector<Uint32> ItemsIndex ) {
	if ( ItemsIndex.size() && eeINDEX_NOT_FOUND != ItemsIndex[0] ) {
		std::vector<UITableCell*> ItemsCpy;
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
			} else if ( NULL != mItems[i] ) {
				mItems[i]->close();
				mItems[i]->setVisible( false );
				mItems[i]->setEnabled( false );
				mItems[i] = NULL;
			}
		}

		mItems = ItemsCpy;

		setDefaultCollumnsWidth();
		updateSize();
		updateScroll();
	}
}

void UITable::remove( Uint32 ItemIndex ) {
	remove( std::vector<Uint32> ( 1, ItemIndex ) );
}

UITable * UITable::setCollumnWidth( const Uint32& CollumnIndex, const Uint32& CollumnWidth ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	mCollWidthAssigned = true;

	mCollumnsWidth[ CollumnIndex ] = CollumnWidth;

	updateCollumnsPos();
	updateCells();
	updateScroll();

	return this;
}

Uint32 UITable::getCount() const {
	return mItems.size();
}

UITable * UITable::setCollumnsCount(const Uint32 & collumnsCount) {
	mCollumnsCount = collumnsCount;

	mCollumnsWidth.resize( mCollumnsCount, 0 );
	mCollumnsPos.resize( mCollumnsCount, 0 );

	setDefaultCollumnsWidth();

	return this;
}

const Uint32& UITable::getCollumnsCount() const {
	return mCollumnsCount;
}

const Uint32& UITable::getCollumnWidth( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsWidth[ CollumnIndex ];
}

UITable * UITable::setRowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		updateSize();
		updateCells();
		updateScroll();
	}

	return this;
}

const Uint32& UITable::getRowHeight() const {
	return mRowHeight;
}

UITableCell * UITable::getCell( const Uint32& CellIndex ) const {
	eeASSERT( CellIndex < mItems.size() );

	return mItems[ CellIndex ];
}

Uint32 UITable::getCellPosition( const Uint32& CollumnIndex ) {
	eeASSERT( CollumnIndex < mCollumnsCount );

	return mCollumnsPos[ CollumnIndex ];
}

void UITable::updateCells() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->fixCell();
	}
}

void UITable::updateCollumnsPos() {
	Uint32 Pos = 0;

	for ( Uint32 i = 0; i < mCollumnsCount; i++ ) {
		mCollumnsPos[ i ] = Pos;

		Pos += mCollumnsWidth[ i ];
	}

	mTotalWidth = Pos;
}

void UITable::onAlphaChange() {
	UINode::onAlphaChange();

	mVScrollBar->setAlpha( mAlpha );
	mHScrollBar->setAlpha( mAlpha );
}

void UITable::setVerticalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		updateScroll();
	}
}

const ScrollBarMode& UITable::getVerticalScrollMode() {
	return mVScrollMode;
}

void UITable::setHorizontalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;

		if ( ScrollBarMode::AlwaysOn == mHScrollMode ) {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );
			containerResize();
		} else if ( ScrollBarMode::AlwaysOff == mHScrollMode ) {
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			containerResize();
		}

		updateScroll();
	}
}

const ScrollBarMode& UITable::getHorizontalScrollMode() {
	return mHScrollMode;
}

UIScrollBar * UITable::getVerticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar * UITable::getHorizontalScrollBar() const {
	return mHScrollBar;
}

Uint32 UITable::getItemIndex( UITableCell * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UITable::onSelected() {
	sendCommonEvent( Event::OnItemSelected );

	return 1;
}

UITableCell * UITable::getItemSelected() {
	if ( -1 != mSelected )
		return mItems[ mSelected ];

	return NULL;
}

Uint32 UITable::getItemSelectedIndex() const {
	return mSelected;
}

Uint32 UITable::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::FocusLoss:
		{
			if ( NULL != getEventDispatcher() ) {
				Node * FocusCtrl = getEventDispatcher()->getFocusControl();

				if ( this != FocusCtrl && !isParentOf( FocusCtrl ) ) {
					onWidgetFocusLoss();
				}

				return 1;
			}
		}
	}

	return 0;
}

UIItemContainer<UITable> * UITable::getContainer() const {
	return mContainer;
}

bool UITable::getSmoothScroll() const {
	return mSmoothScroll;
}

UITable * UITable::setSmoothScroll(bool smoothScroll) {
	mSmoothScroll = smoothScroll;

	if ( mSmoothScroll ) {
		mContainer->clipEnable();
	} else {
		mContainer->clipDisable();
	}

	return this;
}

Rectf UITable::getContainerPadding() const {
	return PixelDensity::pxToDp( mContainerPadding + mRealPadding );
}

void UITable::onPaddingChange() {
	containerResize();
}

void UITable::onTouchDragValueChange( Vector2f diff ) {
	if ( mVScrollBar->isEnabled() )
		mVScrollBar->setValue( mVScrollBar->getValue() + ( -diff.y / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

	if ( mHScrollBar->isEnabled() )
		mHScrollBar->setValue( mHScrollBar->getValue() + ( -diff.x / mTotalWidth ) );
}

bool UITable::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds() && !mVScrollBar->isMouseOverMeOrChilds() && !mHScrollBar->isMouseOverMeOrChilds();
}

std::string UITable::getPropertyString( const PropertyDefinition* propertyDef ) {
	if ( NULL == propertyDef ) return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::RowHeight:
			return String::format( "%ddp", getRowHeight() );
		case PropertyId::VScrollMode:
			return getVerticalScrollMode() == ScrollBarMode::Auto ? "auto" : (
				getVerticalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off"
			);
		case PropertyId::HScrollMode:
			return getHorizontalScrollMode() == ScrollBarMode::Auto ? "auto" : (
				getHorizontalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off"
			);
		case PropertyId::ScrollBarType:
			return mVScrollBar->getScrollBarType() == UIScrollBar::NoButtons ? "no-buttons" :
																			   "two-buttons";
		default:
			return UITouchDragableWidget::getPropertyString( propertyDef );
	}
}

bool UITable::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) ) return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::RowHeight:
			setRowHeight( attribute.asDpDimensionI() );
			break;
		case PropertyId::VScrollMode:
		{
			std::string val = attribute.asString();
			if ( "auto" == val ) setVerticalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val ) setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val ) setVerticalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::HScrollMode:
		{
			std::string val = attribute.asString();
			if ( "auto" == val ) setHorizontalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val ) setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val ) setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::ScrollBarType:
		{
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "no-buttons" == val || "nobuttons" == val ) {
				mVScrollBar->setScrollBarType( UIScrollBar::NoButtons );
				mHScrollBar->setScrollBarType( UIScrollBar::NoButtons );
			} else if ( "two-buttons" == val || "twobuttons" == val ) {
				mVScrollBar->setScrollBarType( UIScrollBar::TwoButtons );
				mHScrollBar->setScrollBarType( UIScrollBar::NoButtons );
			}
			break;
		}
		default:
			return UITouchDragableWidget::applyProperty( attribute );
	}

	return true;
}

}}
