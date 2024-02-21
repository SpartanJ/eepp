#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiwidgettable.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIWidgetTable* UIWidgetTable::New() {
	return eeNew( UIWidgetTable, () );
}

UIWidgetTable::UIWidgetTable() :
	UITouchDraggableWidget( "widgettable" ),
	mContainerPadding(),
	mContainer( NULL ),
	mVScrollBar( NULL ),
	mHScrollBar( NULL ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mColumnsCount( 0 ),
	mRowHeight( 0 ),
	mTotalWidth( 0 ),
	mLastPos( eeINDEX_NOT_FOUND ),
	mVisibleFirst( 0 ),
	mVisibleLast( 0 ),
	mHScrollInit( 0 ),
	mItemsNotVisible( 0 ),
	mSelected( -1 ),
	mSmoothScroll( false ),
	mCollWidthAssigned( false ) {
	setFlags( UI_AUTO_PADDING );

	auto cb = [this]( const Event* ) { containerResize(); };

	mContainer = eeNew( UIItemContainer<UIWidgetTable>, () );
	mContainer->setVisible( true );
	mContainer->setEnabled( true );
	mContainer->setParent( this );
	mContainer->setPosition( mContainerPadding.Left, mContainerPadding.Top );
	mContainer->setSize( getSize().getWidth() - mContainerPadding.Right - mContainerPadding.Left,
						 getSize().getHeight() - mContainerPadding.Top - mContainerPadding.Bottom );

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

	mVScrollBar->addEventListener( Event::OnValueChange,
								   [this] ( auto event ) { onScrollValueChange( event ); } );
	mHScrollBar->addEventListener( Event::OnValueChange,
								   [this] ( auto event ) { onScrollValueChange( event ); } );

	mVScrollBar->addEventListener( Event::OnSizeChange, cb );
	mHScrollBar->addEventListener( Event::OnSizeChange, cb );

	applyDefaultTheme();
}

UIWidgetTable::~UIWidgetTable() {}

Uint32 UIWidgetTable::getType() const {
	return UI_TYPE_WIDGETTABLE;
}

bool UIWidgetTable::isType( const Uint32& type ) const {
	return UIWidgetTable::getType() == type ? true : UITouchDraggableWidget::isType( type );
}

void UIWidgetTable::setDefaultColumnsWidth() {
	if ( mCollWidthAssigned || 0 == mColumnsCount || 0 == mRowHeight )
		return;

	if ( mItemsNotVisible <= 0 ) {
		Uint32 VisibleItems = mContainer->getSize().getHeight() / mRowHeight;
		Int32 oItemsNotVisible = (Int32)mItems.size() - VisibleItems;

		if ( oItemsNotVisible > 0 ) {
			mItemsNotVisible = oItemsNotVisible;

			updateVScroll();

			containerResize();
		}
	}

	Uint32 ColumnWidh = mContainer->getSize().getWidth() / mColumnsCount;

	for ( Uint32 i = 0; i < mColumnsCount; i++ ) {
		if ( 0 == mColumnsWidth[i] )
			mColumnsWidth[i] = ColumnWidh;
	}

	updateSize();
	updateCells();
}

void UIWidgetTable::onScrollValueChange( const Event* ) {
	updateScroll( true );
}

void UIWidgetTable::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "genericgrid" );

	autoPadding();

	onSizeChange();

	onThemeLoaded();
}

void UIWidgetTable::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mContainerPadding = PixelDensity::dpToPx( makePadding() );
	}
}

void UIWidgetTable::onSizeChange() {
	containerResize();
	setDefaultColumnsWidth();
	updateScroll();
	updatePageStep();
}

void UIWidgetTable::containerResize() {
	Rectf padding = mContainerPadding + mPaddingPx;

	mContainer->setPosition( padding.Left, padding.Top );

	if ( mHScrollBar->isVisible() ) {
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left,
								   mSize.getHeight() - padding.Top -
									   mHScrollBar->getPixelsSize().getHeight() );
	} else {
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left,
								   mSize.getHeight() - padding.Bottom - padding.Top );
	}

	if ( mVScrollBar->isVisible() ) {
		mContainer->setPixelsSize( mContainer->getPixelsSize().getWidth() -
									   mVScrollBar->getPixelsSize().getWidth(),
								   mContainer->getPixelsSize().getHeight() );
	}

	setDefaultColumnsWidth();
	updateScrollBar();
}

void UIWidgetTable::updateVScroll() {
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

void UIWidgetTable::updateHScroll() {
	if ( mContainer->isClipped() &&
		 ( ScrollBarMode::Auto == mHScrollMode || ScrollBarMode::AlwaysOn == mHScrollMode ) ) {
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

void UIWidgetTable::setHScrollStep() {
	Float width = (Float)mContainer->getPixelsSize().getWidth();

	if ( ( mItemsNotVisible > 0 && ScrollBarMode::Auto == mVScrollMode ) ||
		 ScrollBarMode::AlwaysOn == mVScrollMode )
		width -= mVScrollBar->getPixelsSize().getWidth();

	Float maxWidth = 0;

	if ( mColumnsCount > 0 ) {
		for ( Uint32 i = 0; i < mColumnsCount; i++ ) {
			maxWidth += mColumnsWidth[i];
		}
	}

	Float stepVal = width / (Float)maxWidth;

	mHScrollBar->setPageStep( stepVal );

	mHScrollBar->setClickStep( stepVal );
}

void UIWidgetTable::updateScrollBar() {
	mVScrollBar->setPosition( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 0 );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth(), getSize().getHeight() );

	mHScrollBar->setPosition( 0, getSize().getHeight() - mHScrollBar->getSize().getHeight() );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(),
						  mHScrollBar->getSize().getHeight() );

	if ( mContainer->isClipped() && ScrollBarMode::Auto == mHScrollMode ) {
		if ( (Int32)mTotalWidth <= mContainer->getSize().getWidth() ) {
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			mHScrollInit = 0;
		}
	}
}

void UIWidgetTable::updateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	UIWidgetTableRow* Item;
	Uint32 i, RelPos = 0, RelPosMax;
	Int32 ItemPos, ItemPosMax;
	Int32 tHLastScroll = mHScrollInit;

	Uint32 VisibleItems = mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible = (Int32)mItems.size() - VisibleItems;
	bool Clipped = 0 != mContainer->isClipped();

	updateVScroll();

	updateHScroll();

	VisibleItems = mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible = (Uint32)mItems.size() - VisibleItems;
	Int32 Scrolleable = (Int32)mItems.size() * mRowHeight - mContainer->getSize().getHeight();
	bool FirstVisible = false;

	if ( Clipped && mSmoothScroll ) {
		if ( Scrolleable >= 0 )
			RelPos = (Uint32)( mVScrollBar->getValue() * Scrolleable );
		else
			RelPos = 0;

		RelPosMax = RelPos + mContainer->getSize().getHeight() + mRowHeight;

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos ) &&
			 ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * i;
			ItemPosMax = ItemPos + mRowHeight;

			if ( ( ItemPos >= (Int32)RelPos || ItemPosMax >= (Int32)RelPos ) &&
				 ( ItemPos <= (Int32)RelPosMax ) ) {
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
		RelPosMax = (Uint32)mItems.size();

		if ( mItemsNotVisible > 0 ) {
			RelPos = (Uint32)( mVScrollBar->getValue() * mItemsNotVisible );
			RelPosMax = RelPos + VisibleItems;
		}

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos ) &&
			 ( !Clipped || tHLastScroll == mHScrollInit ) )
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
		mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(),
							  mHScrollBar->getSize().getHeight() );
	}

	setHScrollStep();
}

void UIWidgetTable::updateSize() {
	updateColumnsPos();

	mTotalHeight = mItems.size() * mRowHeight;
}

void UIWidgetTable::add( UIWidgetTableRow* row ) {
	row->setParent( getContainer() );

	mItems.push_back( row );

	if ( mContainer != row->getParent() )
		row->setParent( mContainer );

	setDefaultColumnsWidth();

	row->onAutoSize();

	updateSize();

	updateScroll();

	updatePageStep();
}

void UIWidgetTable::remove( UIWidgetTableRow* row ) {
	return remove( getItemIndex( row ) );
}

void UIWidgetTable::remove( std::vector<Uint32> itemsIndex ) {
	if ( itemsIndex.size() && eeINDEX_NOT_FOUND != itemsIndex[0] ) {
		std::vector<UIWidgetTableRow*> ItemsCpy;
		bool erase;

		for ( Uint32 i = 0; i < mItems.size(); i++ ) {
			erase = false;

			for ( Uint32 z = 0; z < itemsIndex.size(); z++ ) {
				if ( itemsIndex[z] == i ) {
					if ( (Int32)itemsIndex[z] == mSelected )
						mSelected = -1;

					itemsIndex.erase( itemsIndex.begin() + z );

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

		setDefaultColumnsWidth();
		updateSize();
		updateScroll();
	}
}

void UIWidgetTable::remove( Uint32 itemIndex ) {
	remove( std::vector<Uint32>( 1, itemIndex ) );
}

UIWidgetTable* UIWidgetTable::setColumnWidth( const Uint32& columnIndex,
											  const Uint32& ColumnWidth ) {
	eeASSERT( columnIndex < mColumnsCount );

	mCollWidthAssigned = true;

	mColumnsWidth[columnIndex] = ColumnWidth;

	updateColumnsPos();
	updateCells();
	updateScroll();

	return this;
}

Uint32 UIWidgetTable::getCount() const {
	return mItems.size();
}

UIWidgetTable* UIWidgetTable::setColumnsCount( const Uint32& columnsCount ) {
	mColumnsCount = columnsCount;

	mColumnsWidth.resize( mColumnsCount, 0 );
	mColumnsPos.resize( mColumnsCount, 0 );

	setDefaultColumnsWidth();

	return this;
}

const Uint32& UIWidgetTable::getColumnsCount() const {
	return mColumnsCount;
}

const Uint32& UIWidgetTable::getColumnWidth( const Uint32& columnIndex ) const {
	eeASSERT( columnIndex < mColumnsCount );

	return mColumnsWidth[columnIndex];
}

UIWidgetTable* UIWidgetTable::setRowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		updateSize();
		updateCells();
		updateScroll();
	}

	return this;
}

const Uint32& UIWidgetTable::getRowHeight() const {
	return mRowHeight;
}

UIWidgetTableRow* UIWidgetTable::getRow( const Uint32& rowIndex ) const {
	eeASSERT( rowIndex < mItems.size() );

	return mItems[rowIndex];
}

Uint32 UIWidgetTable::getColumnPosition( const Uint32& columnIndex ) {
	eeASSERT( columnIndex < mColumnsCount );

	return mColumnsPos[columnIndex];
}

void UIWidgetTable::updateCells() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->updateRow();
	}
}

void UIWidgetTable::updateColumnsPos() {
	Uint32 Pos = 0;

	for ( Uint32 i = 0; i < mColumnsCount; i++ ) {
		mColumnsPos[i] = Pos;

		Pos += mColumnsWidth[i];
	}

	mTotalWidth = Pos;
}

void UIWidgetTable::onAlphaChange() {
	UINode::onAlphaChange();

	mVScrollBar->setAlpha( mAlpha );
	mHScrollBar->setAlpha( mAlpha );
}

void UIWidgetTable::setVerticalScrollMode( const ScrollBarMode& mode ) {
	if ( mode != mVScrollMode ) {
		mVScrollMode = mode;

		updateScroll();
	}
}

const ScrollBarMode& UIWidgetTable::getVerticalScrollMode() const {
	return mVScrollMode;
}

void UIWidgetTable::setHorizontalScrollMode( const ScrollBarMode& mode ) {
	if ( mode != mHScrollMode ) {
		mHScrollMode = mode;

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

const ScrollBarMode& UIWidgetTable::getHorizontalScrollMode() const {
	return mHScrollMode;
}

UIScrollBar* UIWidgetTable::getVerticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar* UIWidgetTable::getHorizontalScrollBar() const {
	return mHScrollBar;
}

Uint32 UIWidgetTable::getItemIndex( UIWidgetTableRow* item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIWidgetTable::onSelected() {
	sendCommonEvent( Event::OnItemSelected );

	return 1;
}

UIWidgetTableRow* UIWidgetTable::getItemSelected() {
	if ( -1 != mSelected )
		return mItems[mSelected];

	return NULL;
}

Uint32 UIWidgetTable::getItemSelectedIndex() const {
	return mSelected;
}

Uint32 UIWidgetTable::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::FocusLoss: {
			if ( NULL != getEventDispatcher() ) {
				Node* focusNode = getEventDispatcher()->getFocusNode();

				if ( this != focusNode && !isParentOf( focusNode ) ) {
					onWidgetFocusLoss();
				}

				return 1;
			}
		}
	}

	return 0;
}

UIItemContainer<UIWidgetTable>* UIWidgetTable::getContainer() const {
	return mContainer;
}

bool UIWidgetTable::getSmoothScroll() const {
	return mSmoothScroll;
}

UIWidgetTable* UIWidgetTable::setSmoothScroll( bool smoothScroll ) {
	mSmoothScroll = smoothScroll;

	if ( mSmoothScroll ) {
		mContainer->setClipType( ClipType::ContentBox );
	} else {
		mContainer->setClipType( ClipType::None );
	}

	return this;
}

Rectf UIWidgetTable::getContainerPadding() const {
	return PixelDensity::pxToDp( mContainerPadding + mPaddingPx );
}

void UIWidgetTable::onPaddingChange() {
	containerResize();
}

void UIWidgetTable::onTouchDragValueChange( Vector2f diff ) {
	if ( mVScrollBar->isEnabled() )
		mVScrollBar->setValue( mVScrollBar->getValue() +
							   ( -diff.y / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

	if ( mHScrollBar->isEnabled() )
		mHScrollBar->setValue( mHScrollBar->getValue() + ( -diff.x / mTotalWidth ) );
}

bool UIWidgetTable::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds() && !mVScrollBar->isMouseOverMeOrChilds() &&
		   !mHScrollBar->isMouseOverMeOrChilds();
}

void UIWidgetTable::updatePageStep() {
	mVScrollBar->setPageStep( ( (Float)mContainer->getSize().getHeight() / (Float)mRowHeight ) /
							  (Float)mItems.size() );
}

std::string UIWidgetTable::getPropertyString( const PropertyDefinition* propertyDef,
											  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::RowHeight:
			return String::format( "%ddp", getRowHeight() );
		case PropertyId::VScrollMode:
			return getVerticalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getVerticalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		case PropertyId::HScrollMode:
			return getHorizontalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getHorizontalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		case PropertyId::ScrollBarStyle:
			return mVScrollBar->getScrollBarType() == UIScrollBar::NoButtons ? "no-buttons"
																			 : "two-buttons";
		default:
			return UITouchDraggableWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIWidgetTable::getPropertiesImplemented() const {
	auto props = UITouchDraggableWidget::getPropertiesImplemented();
	auto local = { PropertyId::RowHeight, PropertyId::VScrollMode, PropertyId::HScrollMode,
				   PropertyId::ScrollBarStyle };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIWidgetTable::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::RowHeight:
			setRowHeight( attribute.asDpDimensionI( this ) );
			break;
		case PropertyId::VScrollMode: {
			const std::string& val = attribute.value();
			if ( "auto" == val )
				setVerticalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::HScrollMode: {
			const std::string& val = attribute.value();
			if ( "auto" == val )
				setHorizontalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::ScrollBarStyle: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "no-buttons" == val || "nobuttons" == val ) {
				mVScrollBar->setScrollBarStyle( UIScrollBar::NoButtons );
				mHScrollBar->setScrollBarStyle( UIScrollBar::NoButtons );
			} else if ( "two-buttons" == val || "twobuttons" == val ) {
				mVScrollBar->setScrollBarStyle( UIScrollBar::TwoButtons );
				mHScrollBar->setScrollBarStyle( UIScrollBar::NoButtons );
			}
			break;
		}
		default:
			return UITouchDraggableWidget::applyProperty( attribute );
	}

	return true;
}

}} // namespace EE::UI
