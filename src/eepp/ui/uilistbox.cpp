#include <algorithm>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/input.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIListBox* UIListBox::New() {
	return eeNew( UIListBox, () );
}

UIListBox* UIListBox::NewWithTag( const std::string& tag ) {
	return eeNew( UIListBox, ( tag ) );
}

UIListBox::UIListBox( const std::string& tag ) :
	UITouchDraggableWidget( tag ),
	mRowHeight( 0 ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mContainerPadding(),
	mContainer( NULL ),
	mVScrollBar( NULL ),
	mHScrollBar( NULL ),
	mLastPos( eeINDEX_NOT_FOUND ),
	mMaxTextWidth( 0 ),
	mHScrollInit( 0 ),
	mItemsNotVisible( 0 ),
	mVisibleFirst( 0 ),
	mVisibleLast( 0 ),
	mSmoothScroll( true ) {
	setFlags( UI_AUTO_PADDING );

	auto cb = [this]( const Event* ) { containerResize(); };

	mContainer = eeNew( UIItemContainer<UIListBox>, () );
	mContainer->setParent( this );
	mContainer->setSize( getSize() );
	mContainer->setVisible( true );
	mContainer->setEnabled( true );
	mContainer->setFlags( mFlags );
	mContainer->setPosition( 0, 0 );
	mContainer->setClipType( ClipType::ContentBox );

	mVScrollBar = UIScrollBar::NewVertical();
	mVScrollBar->setParent( this );
	mVScrollBar->setPosition( getSize().getWidth() - 8, 0 );
	mVScrollBar->setSize( 8, getSize().getHeight() );
	mVScrollBar->setEnabled( false )->setVisible( false );
	mVScrollBar->addEventListener( Event::OnSizeChange, cb );
	mVScrollBar->addEventListener( Event::OnValueChange,
								   [this] ( auto event ) { onScrollValueChange( event ); } );

	mHScrollBar = UIScrollBar::NewHorizontal();
	mHScrollBar->setParent( this );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth(), 8 );
	mHScrollBar->setPosition( 0, getSize().getHeight() - 8 );
	mHScrollBar->setEnabled( false )->setVisible( false );
	mHScrollBar->addEventListener( Event::OnSizeChange, cb );
	mHScrollBar->addEventListener( Event::OnValueChange,
								   [this] ( auto event ) { onHScrollValueChange( event ); } );

	mDummyItem = createListBoxItem( "" );
	mDummyItem->setSize( 0, 0 );
	mDummyItem->setParent( this );
	mDummyItem->setVisible( false );
	mDummyItem->setEnabled( false );

	setSmoothScroll( true );

	applyDefaultTheme();

	setRowHeight();
}

UIListBox::UIListBox() : UIListBox( "listbox" ) {}

UIListBox::~UIListBox() {}

Uint32 UIListBox::getType() const {
	return UI_TYPE_LISTBOX;
}

bool UIListBox::isType( const Uint32& type ) const {
	return UIListBox::getType() == type ? true : UITouchDraggableWidget::isType( type );
}

void UIListBox::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	mVScrollBar->setTheme( Theme );
	mHScrollBar->setTheme( Theme );

	setThemeSkin( Theme, "listbox" );

	autoPadding();

	onSizeChange();

	onThemeLoaded();
}

void UIListBox::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mContainerPadding = PixelDensity::dpToPx( makePadding() );
	}
}

UIScrollBar* UIListBox::getVerticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar* UIListBox::getHorizontalScrollBar() const {
	return mHScrollBar;
}

void UIListBox::addListBoxItems( std::vector<String> texts ) {
	mItems.reserve( mItems.size() + texts.size() );
	mTexts.reserve( mTexts.size() + texts.size() );

	for ( Uint32 i = 0; i < texts.size(); i++ ) {
		mTexts.push_back( std::move( texts[i] ) );
		mItems.push_back( NULL );
	}

	updatePageStep();
	updateScroll();
}

Uint32 UIListBox::addListBoxItem( UIListBoxItem* Item ) {
	mItems.push_back( Item );
	mTexts.push_back( Item->getText() );

	if ( Item->getParent() != mContainer )
		Item->setParent( mContainer );

	updateScroll();

	Uint32 tMaxTextWidth = mMaxTextWidth;

	itemUpdateSize( Item );

	if ( tMaxTextWidth != mMaxTextWidth ) {
		updateListBoxItemsSize();
		updateScroll();
	}

	updatePageStep();

	return (Uint32)( mItems.size() - 1 );
}

Uint32 UIListBox::addListBoxItem( const String& text ) {
	mTexts.push_back( text );
	mItems.push_back( NULL );

	updatePageStep();
	updateScroll();

	return (Uint32)( mItems.size() - 1 );
}

UIListBoxItem* UIListBox::createListBoxItem( const String& Name ) {
	UIListBoxItem* tItem = UIListBoxItem::NewWithTag( mTag + "::item" );
	tItem->setParent( mContainer );
	tItem->setTheme( mTheme );
	tItem->setHorizontalAlign( UI_HALIGN_LEFT )->setVerticalAlign( UI_VALIGN_CENTER );
	tItem->setText( Name );

	return tItem;
}

Uint32 UIListBox::removeListBoxItem( const String& Text ) {
	return removeListBoxItem( getListBoxItemIndex( Text ) );
}

Uint32 UIListBox::removeListBoxItem( UIListBoxItem* Item ) {
	return removeListBoxItem( getListBoxItemIndex( Item ) );
}

void UIListBox::removeListBoxItems( std::vector<Uint32> ItemsIndex ) {
	if ( ItemsIndex.empty() || eeINDEX_NOT_FOUND == ItemsIndex[0] )
		return;

	size_t selectedSize = mSelected.size();

	for ( Uint32 z = 0; z < ItemsIndex.size(); z++ ) {
		auto idx = ItemsIndex[z];
		auto selIt = std::find( mSelected.begin(), mSelected.end(), idx );
		if ( selIt != mSelected.end() )
			mSelected.erase( selIt );
		if ( idx < mItems.size() && NULL != mItems[idx] ) {
			mItems[idx]->close();
			mItems[idx]->setVisible( false );
			mItems[idx]->setEnabled( false );
		}
		mItems.erase( mItems.begin() + idx );
		mTexts.erase( mTexts.begin() + idx );
	}

	findMaxWidth();
	updateScroll();
	updateListBoxItemsSize();

	if ( selectedSize != mSelected.size() )
		sendCommonEvent( Event::OnSelectionChanged );

	if ( mTexts.empty() )
		sendCommonEvent( Event::OnClear );
}

void UIListBox::clear() {
	mTexts.clear();
	mItems.clear();
	mSelected.clear();
	mVScrollBar->setValue( 0 );

	findMaxWidth();
	updateScroll();
	updateListBoxItemsSize();

	sendCommonEvent( Event::OnClear );
	sendCommonEvent( Event::OnSelectionChanged );
}

Uint32 UIListBox::removeListBoxItem( Uint32 ItemIndex ) {
	removeListBoxItems( { ItemIndex } );

	return ItemIndex;
}

Uint32 UIListBox::getListBoxItemIndex( const String& Name ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Name == mTexts[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIListBox::getListBoxItemIndex( UIListBoxItem* Item ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

void UIListBox::onScrollValueChange( const Event* ) {
	updateScroll( true );
}

void UIListBox::onHScrollValueChange( const Event* ) {
	updateScroll( true );
}

void UIListBox::onSizeChange() {
	containerResize();
	updateScrollBarState();
	updateScroll();
	updatePageStep();
	UIWidget::onSizeChange();
}

void UIListBox::onPaddingChange() {
	containerResize();
	updateScroll();
}

void UIListBox::setRowHeight() {
	Uint32 tOldRowHeight = mRowHeight;

	if ( 0 == mRowHeight ) {
		Uint32 fontSize = 12;

		const FontStyleConfig& fontStyleConfig = mDummyItem->getFontStyleConfig();

		if ( NULL != fontStyleConfig.getFont() )
			fontSize =
				fontStyleConfig.getFont()->getFontHeight( fontStyleConfig.getFontCharacterSize() );

		mRowHeight = PixelDensity::pxToDp( fontSize ) + PixelDensity::dpToPx( 4 );
	}

	if ( tOldRowHeight != mRowHeight ) {
		updateScroll();
		updateListBoxItemsSize();
	}
}

void UIListBox::setHScrollStep() {
	Float width = (Float)mContainer->getPixelsSize().getWidth();

	if ( ( mItemsNotVisible > 0 && ScrollBarMode::Auto == mVScrollMode ) ||
		 ScrollBarMode::AlwaysOn == mVScrollMode )
		width -= mVScrollBar->getPixelsSize().getWidth();

	Float stepVal = width / (Float)mMaxTextWidth;

	mHScrollBar->setPageStep( stepVal );

	mHScrollBar->setClickStep( stepVal );
}

void UIListBox::updateScrollBar() {
	mVScrollBar->setPosition( getSize().getWidth() - mVScrollBar->getSize().getWidth() +
								  mVScrollBar->getPadding().Left,
							  mVScrollBar->getPadding().Top );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth() + mVScrollBar->getPadding().Right,
						  getSize().getHeight() + mVScrollBar->getPadding().Bottom );

	mHScrollBar->setPosition( mHScrollBar->getPadding().Left,
							  getSize().getHeight() - mHScrollBar->getSize().getHeight() +
								  mHScrollBar->getPadding().Top );
	mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth() +
							  mHScrollBar->getPadding().Right,
						  mHScrollBar->getSize().getHeight() + mHScrollBar->getPadding().Bottom );
}

void UIListBox::updatePageStep() {
	mVScrollBar->setPageStep( ( (Float)mContainer->getSize().getHeight() / (Float)mRowHeight ) /
							  (Float)mTexts.size() );
}

void UIListBox::onTouchDragValueChange( Vector2f diff ) {
	if ( mVScrollBar->isEnabled() )
		mVScrollBar->setValue( mVScrollBar->getValue() +
							   ( -diff.y / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

	if ( mHScrollBar->isEnabled() )
		mHScrollBar->setValue( mHScrollBar->getValue() + ( -diff.x / mMaxTextWidth ) );
}

bool UIListBox::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds() && !mVScrollBar->isMouseOverMeOrChilds() &&
		   !mHScrollBar->isMouseOverMeOrChilds();
}

void UIListBox::findMaxWidth() {
	const FontStyleConfig& fontStyleConfig = mDummyItem->getFontStyleConfig();

	if ( NULL == fontStyleConfig.getFont() )
		return;

	Uint32 size = (Uint32)mItems.size();
	Int32 width;
	Text textCache;
	textCache.setStyleConfig( fontStyleConfig );

	mMaxTextWidth = 0;

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( NULL != mItems[i] ) {
			width = (Int32)mItems[i]->getTextWidth();
		} else {
			textCache.setString( mTexts[i] );
			width = textCache.getTextWidth();
		}

		if ( width > (Int32)mMaxTextWidth )
			mMaxTextWidth = width;
	}
}

void UIListBox::updateListBoxItemsSize() {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ )
		itemUpdateSize( mItems[i] );
}

void UIListBox::itemUpdateSize( UIListBoxItem* Item ) {
	if ( NULL != Item ) {
		Int32 width = (Int32)Item->getTextWidth();

		if ( width > (Int32)mMaxTextWidth ) {
			mMaxTextWidth = width;
		}

		if ( !mHScrollBar->isVisible() ) {
			if ( width < mContainer->getSize().getWidth() )
				width = mContainer->getSize().getWidth();

			if ( ( mItemsNotVisible > 0 && ScrollBarMode::Auto == mVScrollMode ) ||
				 ScrollBarMode::AlwaysOn == mVScrollMode )
				width -= mVScrollBar->getSize().getWidth();
		} else {
			width = mMaxTextWidth;
		}

		Item->setSize( width, mRowHeight );
	}
}

void UIListBox::containerResize() {
	Rectf padding = mContainerPadding + mPaddingPx;

	mContainer->setPixelsPosition( padding.Left, padding.Top );

	if ( mHScrollBar->isVisible() ) {
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left,
								   mSize.getHeight() - padding.Top -
									   mHScrollBar->getPixelsSize().getHeight() );
	} else {
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left,
								   mSize.getHeight() - padding.Bottom - padding.Top );
	}

	updateListBoxItemsSize();
	updateScrollBar();
}

void UIListBox::createItemIndex( const Uint32& i ) {
	if ( NULL == mItems[i] ) {
		mItems[i] = createListBoxItem( mTexts[i] );

		itemUpdateSize( mItems[i] );

		for ( auto it = mSelected.begin(); it != mSelected.end(); ++it ) {
			if ( *it == i ) {
				mItems[i]->select();

				break;
			}
		}
	}
}

void UIListBox::updateScrollBarState() {
	bool clipped = 0 != mContainer->isClipped();

	Uint32 visibleItems = mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible = (Int32)mItems.size() - visibleItems;

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

	if ( clipped &&
		 ( ScrollBarMode::Auto == mHScrollMode || ScrollBarMode::AlwaysOn == mHScrollMode ) ) {
		if ( ( mVScrollBar->isVisible() &&
			   mContainer->getPixelsSize().getWidth() - mVScrollBar->getPixelsSize().getWidth() <
				   (Int32)mMaxTextWidth ) ||
			 ( !mVScrollBar->isVisible() &&
			   mContainer->getPixelsSize().getWidth() < (Int32)mMaxTextWidth ) ) {
			mHScrollBar->setVisible( true );
			mHScrollBar->setEnabled( true );

			containerResize();

			Int32 ScrollH;

			if ( mVScrollBar->isVisible() )
				ScrollH = PixelDensity::pxToDpI( mMaxTextWidth ) -
						  mContainer->getSize().getWidth() + mVScrollBar->getSize().getWidth();
			else
				ScrollH = PixelDensity::pxToDpI( mMaxTextWidth ) - mContainer->getSize().getWidth();

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

void UIListBox::updateScroll( bool fromScrollChange ) {
	if ( !mItems.size() )
		return;

	bool clipped = 0 != mContainer->isClipped();
	UIListBoxItem* item;
	Uint32 i, relPos = 0, relPosMax;
	Int32 itemPos, itemPosMax;
	Int32 tHLastScroll = mHScrollInit;

	bool wasScrollVisible = mVScrollBar->isVisible();
	bool wasHScrollVisible = mHScrollBar->isVisible();
	bool wasFirstTime = mVisibleFirst == 0 && mVisibleLast == 0;

	updateScrollBarState();

	Uint32 visibleItems = mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible = (Uint32)mItems.size() - visibleItems;
	Int32 scrolleable = (Int32)mItems.size() * mRowHeight - mContainer->getSize().getHeight();
	bool isScrollVisible = mVScrollBar->isVisible();
	bool isHScrollVisible = mHScrollBar->isVisible();
	bool FirstVisible = false;

	if ( clipped && mSmoothScroll ) {
		if ( scrolleable >= 0 )
			relPos = (Uint32)( mVScrollBar->getValue() * scrolleable );
		else
			relPos = 0;

		relPosMax = relPos + mContainer->getSize().getHeight() + mRowHeight;

		if ( ( fromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == relPos ) &&
			 ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = relPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			item = mItems[i];
			itemPos = mRowHeight * i;
			itemPosMax = itemPos + mRowHeight;

			if ( ( itemPos >= (Int32)relPos || itemPosMax >= (Int32)relPos ) &&
				 ( itemPos <= (Int32)relPosMax ) ) {
				if ( NULL == item ) {
					createItemIndex( i );
					item = mItems[i];
				}

				item->setPosition( mHScrollInit, itemPos - (Int32)relPos );
				item->setEnabled( true );
				item->setVisible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else if ( NULL != mItems[i] ) {
				mItems[i]->close();
				mItems[i]->setVisible( false );
				mItems[i]->setEnabled( false );
				mItems[i] = NULL;
				item = NULL;
			}

			if ( NULL != item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) ||
					 ( wasScrollVisible && !isScrollVisible ) ||
					 ( !wasHScrollVisible && isHScrollVisible ) ||
					 ( wasHScrollVisible && !isHScrollVisible ) )
					itemUpdateSize( item );
			}
		}
	} else {
		relPosMax = (Uint32)mItems.size();

		if ( mItemsNotVisible > 0 ) {
			relPos = (Uint32)( mVScrollBar->getValue() * mItemsNotVisible );
			relPosMax = relPos + visibleItems;
		}

		if ( ( fromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == relPos ) &&
			 ( !clipped || tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = relPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			item = mItems[i];
			itemPos = mRowHeight * ( (Int32)i - (Int32)relPos );

			if ( i >= relPos && i < relPosMax ) {
				if ( NULL == item ) {
					createItemIndex( i );
					item = mItems[i];
				}

				if ( clipped )
					item->setPosition( mHScrollInit, itemPos );
				else
					item->setPosition( 0, itemPos );

				item->setEnabled( true );
				item->setVisible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				mItems[i]->close();
				mItems[i]->setVisible( false );
				mItems[i]->setEnabled( false );
				mItems[i] = NULL;
				item = NULL;
			}

			if ( NULL != item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) ||
					 ( wasScrollVisible && !isScrollVisible ) ||
					 ( !wasHScrollVisible && isHScrollVisible ) ||
					 ( wasHScrollVisible && !isHScrollVisible ) )
					itemUpdateSize( item );
			}
		}
	}

	if ( mHScrollBar->isVisible() && !mVScrollBar->isVisible() ) {
		mHScrollBar->setPosition( mHScrollBar->getPadding().Left,
								  getSize().getHeight() - mHScrollBar->getSize().getHeight() +
									  mHScrollBar->getPadding().Top );
		mHScrollBar->setSize( getSize().getWidth() + mHScrollBar->getPadding().Right,
							  mHScrollBar->getSize().getHeight() +
								  mHScrollBar->getPadding().Bottom );
	} else {
		mHScrollBar->setPosition( mHScrollBar->getPadding().Left,
								  getSize().getHeight() - mHScrollBar->getSize().getHeight() +
									  mHScrollBar->getPadding().Top );
		mHScrollBar->setSize( getSize().getWidth() - mVScrollBar->getSize().getWidth() +
								  mHScrollBar->getPadding().Right,
							  mHScrollBar->getSize().getHeight() +
								  mHScrollBar->getPadding().Bottom );
	}

	setHScrollStep();

	if ( wasFirstTime )
		updateScrollBarState();

	invalidateDraw();
}

void UIListBox::itemKeyEvent( const KeyEvent& Event ) {
	KeyEvent ItemEvent( Event.getNode(), Event::OnItemKeyDown, Event.getKeyCode(),
						Event.getScancode(), Event.getChar(), Event.getMod() );
	sendEvent( &ItemEvent );
}

void UIListBox::itemClicked( UIListBoxItem* Item ) {
	Event ItemEvent( Item, Event::OnItemClicked );
	sendEvent( &ItemEvent );

	if ( !( isMultiSelect() && NULL != getEventDispatcher() &&
			getEventDispatcher()->getInput()->isKeyDown( KEY_LCTRL ) ) )
		resetItemsStates();
}

Uint32 UIListBox::onSelected() {
	NodeMessage tMsg( this, NodeMessage::Selected, 0 );
	messagePost( &tMsg );

	sendCommonEvent( Event::OnSelectionChanged );
	sendCommonEvent( Event::OnItemSelected );

	return 1;
}

void UIListBox::resetItemsStates() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( NULL != mItems[i] )
			mItems[i]->unselect();
	}
}

bool UIListBox::isMultiSelect() const {
	return 0 != ( mFlags & UI_MULTI_SELECT );
}

UIListBoxItem* UIListBox::getItem( const Uint32& Index ) const {
	eeASSERT( Index < mItems.size() );

	return mItems[Index];
}

UIListBoxItem* UIListBox::getItemSelected() {
	if ( mSelected.size() ) {
		if ( NULL == mItems[mSelected.front()] )
			createItemIndex( mSelected.front() );

		return mItems[mSelected.front()];
	}

	return NULL;
}

Uint32 UIListBox::getItemSelectedIndex() const {
	if ( mSelected.size() )
		return mSelected.front();

	return eeINDEX_NOT_FOUND;
}

bool UIListBox::hasSelection() const {
	return !mSelected.empty();
}

String UIListBox::getItemSelectedText() const {
	String tstr;

	if ( mSelected.size() )
		return mTexts[mSelected.front()];

	return tstr;
}

const std::vector<String>& UIListBox::getItemsText() const {
	return mTexts;
}

std::vector<Uint32> UIListBox::getItemsSelectedIndex() const {
	return mSelected;
}

std::vector<UIListBoxItem*> UIListBox::getItemsSelected() {
	std::vector<UIListBoxItem*> tItems;
	std::vector<Uint32>::iterator it;

	for ( it = mSelected.begin(); it != mSelected.end(); ++it ) {
		if ( NULL == mItems[*it] )
			createItemIndex( *it );

		tItems.push_back( mItems[*it] );
	}

	return tItems;
}

Uint32 UIListBox::getItemIndex( UIListBoxItem* Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIListBox::getItemIndex( const String& Text ) {
	for ( Uint32 i = 0; i < mTexts.size(); i++ ) {
		if ( Text == mTexts[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Rectf UIListBox::getContainerPadding() const {
	return PixelDensity::pxToDp( mContainerPadding + mPadding );
}

void UIListBox::setSmoothScroll( const bool& soft ) {
	if ( soft != mSmoothScroll ) {
		mSmoothScroll = soft;

		updateScroll();
	}
}

const bool& UIListBox::isSmoothScroll() const {
	return mSmoothScroll;
}

void UIListBox::setRowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		updateListBoxItemsSize();
		updateScroll();
	}
}

const Uint32& UIListBox::getRowHeight() const {
	return mRowHeight;
}

Uint32 UIListBox::getCount() const {
	return (Uint32)mItems.size();
}

bool UIListBox::isEmpty() const {
	return mItems.empty();
}

void UIListBox::setSelected( const String& Text ) {
	setSelected( getItemIndex( Text ) );
}

void UIListBox::setSelected( Uint32 Index ) {
	if ( std::find( mSelected.begin(), mSelected.end(), Index ) != mSelected.end() )
		return;

	if ( Index < mItems.size() ) {
		if ( isMultiSelect() ) {
			for ( auto it = mSelected.begin(); it != mSelected.end(); ++it ) {
				if ( *it == Index )
					return;
			}
		} else {
			if ( mSelected.size() ) {
				if ( NULL != mItems[mSelected.front()] ) {
					mItems[mSelected.front()]->unselect();
				}

				mSelected.clear();
			}
		}

		mSelected.push_back( Index );

		if ( NULL != mItems[Index] ) {
			mItems[Index]->select();
		} else {
			updateScroll();
			onSelected();
		}
	}
}

void UIListBox::selectPrev() {
	if ( !isMultiSelect() && mSelected.size() ) {
		Int32 SelIndex = mSelected.front() - 1;

		if ( SelIndex >= 0 ) {
			if ( NULL == mItems[mSelected.front()] )
				createItemIndex( mSelected.front() );

			if ( NULL == mItems[SelIndex] )
				createItemIndex( SelIndex );

			if ( mItems[SelIndex]->getPosition().y < 0 ) {
				mVScrollBar->setValue( (Float)( SelIndex * mRowHeight ) /
									   (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[SelIndex]->setFocus();
			}

			setSelected( SelIndex );
		}
	}
}

void UIListBox::selectNext() {
	if ( !isMultiSelect() && mSelected.size() ) {
		Int32 SelIndex = mSelected.front() + 1;

		if ( SelIndex < (Int32)mItems.size() ) {
			if ( NULL == mItems[mSelected.front()] )
				createItemIndex( mSelected.front() );

			if ( NULL == mItems[SelIndex] )
				createItemIndex( SelIndex );

			if ( mItems[SelIndex]->getPosition().y + (Int32)getRowHeight() >
				 mContainer->getSize().getHeight() ) {
				mVScrollBar->setValue( (Float)( SelIndex * mRowHeight ) /
									   (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[SelIndex]->setFocus();
			}

			setSelected( SelIndex );
		}
	}
}

Uint32 UIListBox::onKeyDown( const KeyEvent& event ) {
	UINode::onKeyDown( event );

	if ( mFlags & UI_MULTI_SELECT )
		return 0;

	if ( mSelected.empty() ) {
		switch ( event.getKeyCode() ) {
			case KEY_DOWN:
			case KEY_UP:
			case KEY_HOME:
			case KEY_END:
			case KEY_PAGEUP:
			case KEY_PAGEDOWN: {
				setSelected( 0 );
				itemKeyEvent( event );
				return 1;
			}
			default: {
			}
		}
	}

	switch ( event.getKeyCode() ) {
		case KEY_DOWN:
			selectNext();
			break;
		case KEY_UP:
			selectPrev();
			break;
		case KEY_HOME:
			if ( mSelected.front() != 0 ) {
				mVScrollBar->setValue( 0 );
				mItems[0]->setFocus();
				setSelected( 0 );
			}
			break;
		case KEY_END:
			if ( mSelected.front() != getCount() - 1 ) {
				mVScrollBar->setValue( 1 );
				mItems[getCount() - 1]->setFocus();
				setSelected( getCount() - 1 );
			}
			break;
		case KEY_PAGEUP: {
			Int32 index = getItemSelectedIndex();
			if ( eeINDEX_NOT_FOUND == (Uint32)index )
				index = 0;
			Int32 pageSize = eefloor( mDpSize.getHeight() / mRowHeight );
			index = eemax( 0, index - pageSize );
			setSelected( index );
			mVScrollBar->setValue( (Float)( index * mRowHeight ) /
								   (Float)( ( mItems.size() - 1 ) * mRowHeight ) );
			break;
		}
		case KEY_PAGEDOWN: {
			Int32 index = getItemSelectedIndex();
			if ( eeINDEX_NOT_FOUND == (Uint32)index )
				index = 0;
			Int32 pageSize = eefloor( mDpSize.getHeight() / mRowHeight );
			index = eemin( getCount() ? (Int32)getCount() - 1 : 0, index + pageSize );
			setSelected( index );
			mVScrollBar->setValue( (Float)( index * mRowHeight ) /
								   (Float)( ( mItems.size() - 1 ) * mRowHeight ) );
			break;
		}
		default: {
		}
	}

	itemKeyEvent( event );

	return 1;
}

Uint32 UIListBox::onMessage( const NodeMessage* Msg ) {
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

void UIListBox::onAlphaChange() {
	UIWidget::onAlphaChange();

	if ( mItems.size() ) {
		for ( Uint32 i = mVisibleFirst; i <= mVisibleLast; i++ ) {
			if ( NULL != mItems[i] )
				mItems[i]->setAlpha( mAlpha );
		}
	}

	mVScrollBar->setAlpha( mAlpha );
	mHScrollBar->setAlpha( mAlpha );
}

void UIListBox::setVerticalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		updateScroll();
	}
}

const ScrollBarMode& UIListBox::getVerticalScrollMode() const {
	return mVScrollMode;
}

void UIListBox::setHorizontalScrollMode( const ScrollBarMode& Mode ) {
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

const ScrollBarMode& UIListBox::getHorizontalScrollMode() const {
	return mHScrollMode;
}

std::string UIListBox::getPropertyString( const PropertyDefinition* propertyDef,
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
		case PropertyId::SelectedIndex:
			return String::toString( getItemSelectedIndex() );
		case PropertyId::SelectedText:
			return getItemSelectedText();
		case PropertyId::ScrollBarStyle:
			return mVScrollBar->getScrollBarType() == UIScrollBar::NoButtons ? "no-buttons"
																			 : "two-buttons";
		default:
			return UITouchDraggableWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIListBox::getPropertiesImplemented() const {
	auto props = UITouchDraggableWidget::getPropertiesImplemented();
	auto local = { PropertyId::RowHeight,	 PropertyId::VScrollMode,
				   PropertyId::HScrollMode,	 PropertyId::SelectedIndex,
				   PropertyId::SelectedText, PropertyId::ScrollBarStyle };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

Uint32 UIListBox::getMaxTextWidth() const {
	return mMaxTextWidth;
}

void UIListBox::setItemText( const Uint32& index, const String& newText ) {
	if ( index < mTexts.size() ) {
		mTexts[index] = newText;
		if ( nullptr != mItems[index] )
			mItems[index]->setText( newText );
		ItemValueEvent event( this, Event::OnItemValueChange, index );
		sendEvent( &event );
	}
}

bool UIListBox::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::RowHeight:
			setRowHeight( attribute.asDpDimensionI( this ) );
			break;
		case PropertyId::VScrollMode: {
			std::string val = attribute.asString();
			if ( "auto" == val )
				setVerticalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::HScrollMode: {
			std::string val = attribute.asString();
			if ( "auto" == val )
				setHorizontalScrollMode( ScrollBarMode::Auto );
			else if ( "on" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
			break;
		}
		case PropertyId::SelectedIndex:
			setSelected( attribute.asUint() );
			break;
		case PropertyId::SelectedText:
			setSelected( attribute.asString() );
			break;
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

void UIListBox::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UITouchDraggableWidget::loadFromXmlNode( node );

	loadItemsFromXmlNode( node );

	endAttributesTransaction();
}

void UIListBox::loadItemsFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	std::vector<String> items;
	for ( pugi::xml_node item = node.child( "item" ); item; item = item.next_sibling( "item" ) ) {
		std::string data( item.text().as_string() );
		items.push_back( getTranslatorString( data ) );
	}

	if ( !items.empty() ) {
		addListBoxItems( items );
	}

	endAttributesTransaction();
}

}} // namespace EE::UI
