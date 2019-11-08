#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/font.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/window/input.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

UIListBox * UIListBox::New() {
	return eeNew( UIListBox, () );
}

UIListBox * UIListBox::NewWithTag( const std::string& tag ) {
	return eeNew( UIListBox, ( tag ) );
}

UIListBox::UIListBox( const std::string& tag ) :
	UITouchDragableWidget( tag ),
	mRowHeight(0),
	mVScrollMode( UI_SCROLLBAR_AUTO ),
	mHScrollMode( UI_SCROLLBAR_AUTO ),
	mContainerPadding(),
	mHScrollPadding(),
	mVScrollPadding(),
	mContainer( NULL ),
	mVScrollBar( NULL ),
	mHScrollBar( NULL ),
	mLastPos( eeINDEX_NOT_FOUND ),
	mMaxTextWidth(0),
	mHScrollInit(0),
	mItemsNotVisible(0),
	mLastTickMove(0),
	mVisibleFirst(0),
	mVisibleLast(0),
	mSmoothScroll( true )
{
	setFlags( UI_AUTO_PADDING );

	mContainer = eeNew( UIItemContainer<UIListBox>, () );
	mContainer->setParent( this );
	mContainer->setSize( mDpSize.getWidth(), mDpSize.getHeight() );
	mContainer->setVisible( true );
	mContainer->setEnabled( true );
	mContainer->setFlags( mFlags );
	mContainer->setPosition( 0, 0 );
	mContainer->clipEnable();

	mVScrollBar = UIScrollBar::New();
	mVScrollBar->setOrientation( UI_VERTICAL );
	mVScrollBar->setParent( this );
	mVScrollBar->setPosition( mDpSize.getWidth() - 16, 0 );
	mVScrollBar->setSize( 16, mDpSize.getHeight() );
	mVScrollBar->setEnabled( false )->setVisible( false );

	mHScrollBar = UIScrollBar::New();
	mHScrollBar->setOrientation( UI_HORIZONTAL );
	mHScrollBar->setParent( this );
	mHScrollBar->setSize( mDpSize.getWidth() - mVScrollBar->getSize().getWidth(), 16 );
	mHScrollBar->setPosition( 0, mDpSize.getHeight() - 16 );
	mHScrollBar->setEnabled( false )->setVisible( false );

	mVScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &UIListBox::onScrollValueChange ) );
	mHScrollBar->addEventListener( Event::OnValueChange, cb::Make1( this, &UIListBox::onHScrollValueChange ) );

	mDummyItem = createListBoxItem( "" );
	mDummyItem->setSize(0,0);
	mDummyItem->setParent( this );
	mDummyItem->setVisible( false );
	mDummyItem->setEnabled( false );

	setSmoothScroll( true );

	applyDefaultTheme();

	setRowHeight();
}

UIListBox::UIListBox() :
	UIListBox( "listbox" )
{}

UIListBox::~UIListBox() {
}

Uint32 UIListBox::getType() const {
	return UI_TYPE_LISTBOX;
}

bool UIListBox::isType( const Uint32& type ) const {
	return UIListBox::getType() == type ? true : UITouchDragableWidget::isType( type );
}

void UIListBox::setTheme( UITheme * Theme ) {
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

UIScrollBar * UIListBox::getVerticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar * UIListBox::getHorizontalScrollBar() const {
	return mHScrollBar;
}

void UIListBox::addListBoxItems( std::vector<String> Texts ) {
	mItems.reserve( mItems.size() + Texts.size() );
	mTexts.reserve( mTexts.size() + Texts.size() );

	for ( Uint32 i = 0; i < Texts.size(); i++ ) {
		addListBoxItem( Texts[i] );
	}

	updateScroll();
}

Uint32 UIListBox::addListBoxItem( UIListBoxItem * Item ) {
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

	mVScrollBar->setPageStep( ( (Float)mContainer->getSize().getHeight() /(Float) mRowHeight ) / (Float)mItems.size() );

	return (Uint32)(mItems.size() - 1);
}

Uint32 UIListBox::addListBoxItem( const String& text ) {
	mTexts.push_back( text );
	mItems.push_back( NULL );

	const UIFontStyleConfig& fontStyleConfig = mDummyItem->getFontStyleConfig();

	if ( NULL != fontStyleConfig.getFont() ) {
		Text textCache;
		textCache.setStyleConfig( fontStyleConfig );
		textCache.setString( text );

		Uint32 twidth = textCache.getTextWidth();

		if ( twidth > mMaxTextWidth ) {
			mMaxTextWidth = twidth;

			updateListBoxItemsSize();
		}
	}

	mVScrollBar->setPageStep( ( (Float)mContainer->getSize().getHeight() /(Float) mRowHeight ) / (Float)mItems.size() );

	updateScroll();

	return (Uint32)(mItems.size() - 1);
}

UIListBoxItem * UIListBox::createListBoxItem( const String& Name ) {
	UIListBoxItem * tItem		= UIListBoxItem::New();
	tItem->setParent( mContainer );
	tItem->setTheme( mTheme );
	tItem->setHorizontalAlign( UI_HALIGN_LEFT )->setVerticalAlign( UI_VALIGN_CENTER );
	tItem->setText( Name );

	return tItem;
}

Uint32 UIListBox::removeListBoxItem( const String& Text ) {
	return removeListBoxItem( getListBoxItemIndex( Text ) );
}

Uint32 UIListBox::removeListBoxItem( UIListBoxItem * Item ) {
	return removeListBoxItem( getListBoxItemIndex( Item ) );
}

void UIListBox::removeListBoxItems( std::vector<Uint32> ItemsIndex ) {
	if ( ItemsIndex.size() && eeINDEX_NOT_FOUND != ItemsIndex[0] ) {
		std::vector<UIListBoxItem*> ItemsCpy;
		mTexts.clear();

		for ( Uint32 i = 0; i < mItems.size(); i++ ) {
			bool erase = false;

			for ( Uint32 z = 0; z < ItemsIndex.size(); z++ ) {
				if ( ItemsIndex[z] == i ) {
					for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); ++it ) {
						if ( *it == ItemsIndex[z] ) {
							mSelected.erase( it );

							break;
						}
					}

					ItemsIndex.erase( ItemsIndex.begin() + z );

					erase = true;

					break;
				}
			}

			if ( !erase ) {
				ItemsCpy.push_back( mItems[i] );
				mTexts.push_back( mItems[i]->getText() );
			} else {
				eeSAFE_DELETE( mItems[i] ); // doesn't call to mItems[i]->Close(); because is not checking for close.
			}
		}

		mItems = ItemsCpy;

		findMaxWidth();
		updateScroll();
		updateListBoxItemsSize();
	}
}

void UIListBox::clear() {
	mTexts.clear();
	mItems.clear();
	mSelected.clear();
	mVScrollBar->setValue(0);

	findMaxWidth();
	updateScroll();
	updateListBoxItemsSize();

	sendCommonEvent( Event::OnControlClear );
}

Uint32 UIListBox::removeListBoxItem( Uint32 ItemIndex ) {
	removeListBoxItems( std::vector<Uint32>( 1, ItemIndex ) );

	return ItemIndex;
}

Uint32 UIListBox::getListBoxItemIndex( const String& Name ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Name == mItems[i]->getText() )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIListBox::getListBoxItemIndex( UIListBoxItem * Item ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

void UIListBox::onScrollValueChange( const Event * ) {
	updateScroll( true );
}

void UIListBox::onHScrollValueChange( const Event * ) {
	updateScroll( true );
}

void UIListBox::onSizeChange() {
	mVScrollBar->setPosition( mDpSize.getWidth() - mVScrollBar->getSize().getWidth() + mVScrollPadding.Left, mVScrollPadding.Top );
	mVScrollBar->setSize( mVScrollBar->getSize().getWidth() + mVScrollPadding.Right, mDpSize.getHeight() + mVScrollPadding.Bottom );

	mHScrollBar->setPosition( mHScrollPadding.Left, mDpSize.getHeight() - mHScrollBar->getSize().getHeight() + mHScrollPadding.Top );
	mHScrollBar->setSize( mDpSize.getWidth() - mVScrollBar->getSize().getWidth() + mHScrollPadding.Right, mHScrollBar->getSize().getHeight() + mHScrollPadding.Bottom );

	if ( mContainer->isClipped() && UI_SCROLLBAR_AUTO == mHScrollMode ) {
		if ( (Int32)mMaxTextWidth <= mContainer->getPixelsSize().getWidth() ) {
			mHScrollBar->setVisible( false );
			mHScrollBar->setEnabled( false );
			mHScrollInit = 0;
		}
	}

	containerResize();
	updateScrollBarState();
	updateListBoxItemsSize();
	updateScroll();

	UIWidget::onSizeChange();
}

void UIListBox::onPaddingChange() {
	containerResize();
	updateScroll();
}

void UIListBox::setRowHeight() {
	Uint32 tOldRowHeight = mRowHeight;

	if ( 0 == mRowHeight ) {
		Uint32 FontSize = PixelDensity::dpToPxI( 12 );

		const FontStyleConfig& fontStyleConfig = mDummyItem->getFontStyleConfig();

		if ( NULL != fontStyleConfig.getFont() )
			FontSize = fontStyleConfig.getFont()->getFontHeight( PixelDensity::dpToPxI( fontStyleConfig.getFontCharacterSize() ) );

		mRowHeight = (Uint32)PixelDensity::pxToDpI( FontSize ) + 4;
	}

	if ( tOldRowHeight != mRowHeight ) {
		updateScroll();
		updateListBoxItemsSize();
	}
}

void UIListBox::setHScrollStep() {
	Float width = (Float)mContainer->getPixelsSize().getWidth();

	if ( ( mItemsNotVisible > 0 && UI_SCROLLBAR_AUTO == mVScrollMode ) || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode )
		width -= mVScrollBar->getPixelsSize().getWidth();

	Float stepVal = width / (Float)mMaxTextWidth;

	mHScrollBar->setPageStep( stepVal );

	mHScrollBar->setClickStep( stepVal );
}

void UIListBox::onTouchDragValueChange( Vector2f diff ) {
	if ( mVScrollBar->isEnabled() )
		mVScrollBar->setValue( mVScrollBar->getValue() + ( -diff.y / (Float)( ( mItems.size() - 1 ) * mRowHeight ) ) );

	if ( mHScrollBar->isEnabled() )
		mHScrollBar->setValue( mHScrollBar->getValue() + ( -diff.x / mMaxTextWidth ) );
}

bool UIListBox::isTouchOverAllowedChilds() {
	return isMouseOverMeOrChilds() && !mVScrollBar->isMouseOverMeOrChilds() && !mHScrollBar->isMouseOverMeOrChilds();
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
			textCache.setString( mTexts[i]  );
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

	invalidateDraw();
}

void UIListBox::itemUpdateSize( UIListBoxItem * Item ) {
	if ( NULL != Item ) {
		Int32 width = (Int32)Item->getTextWidth();

		if ( width > (Int32)mMaxTextWidth ) {
			mMaxTextWidth = width;
		}

		if ( !mHScrollBar->isVisible() ) {
			if ( width < mContainer->getSize().getWidth() )
				width = mContainer->getSize().getWidth();

			if ( ( mItemsNotVisible > 0 && UI_SCROLLBAR_AUTO == mVScrollMode ) || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode )
				width -= mVScrollBar->getSize().getWidth();
		} else {
			width = mMaxTextWidth;
		}

		Item->setSize( width, mRowHeight );
	}
}

void UIListBox::containerResize() {
	Rectf padding = mContainerPadding + mRealPadding;

	mContainer->setPixelsPosition( padding.Left, padding.Top );

	if( mHScrollBar->isVisible() )
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left, mSize.getHeight() - padding.Top - mHScrollBar->getPixelsSize().getHeight() );
	else
		mContainer->setPixelsSize( mSize.getWidth() - padding.Right - padding.Left, mSize.getHeight() - padding.Bottom - padding.Top );
}

void UIListBox::createItemIndex( const Uint32& i ) {
	if ( NULL == mItems[i] ) {
		mItems[i] = createListBoxItem( mTexts[i] );

		itemUpdateSize( mItems[i] );

		for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); ++it ) {
			if ( *it == i ) {
				mItems[i]->select();

				break;
			}
		}
	}
}

void UIListBox::updateScrollBarState() {
	bool clipped 			= 0 != mContainer->isClipped();

	Uint32 visibleItems 	= mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible 		= (Int32)mItems.size() - visibleItems;

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

	if ( clipped && ( UI_SCROLLBAR_AUTO == mHScrollMode || UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) ) {
		if ( ( mVScrollBar->isVisible() && mContainer->getPixelsSize().getWidth() - mVScrollBar->getPixelsSize().getWidth() < (Int32)mMaxTextWidth ) ||
			( !mVScrollBar->isVisible() && mContainer->getPixelsSize().getWidth() < (Int32)mMaxTextWidth ) ) {
				mHScrollBar->setVisible( true );
				mHScrollBar->setEnabled( true );

				containerResize();

				Int32 ScrollH;

				if ( mVScrollBar->isVisible() )
					ScrollH = PixelDensity::pxToDpI( mMaxTextWidth ) - mContainer->getSize().getWidth() + mVScrollBar->getSize().getWidth();
				else
					ScrollH = PixelDensity::pxToDpI( mMaxTextWidth ) - mContainer->getSize().getWidth();

				Int32 HScrolleable = (Uint32)( mHScrollBar->getValue() * ScrollH );

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

void UIListBox::updateScroll( bool fromScrollChange ) {
	if ( !mItems.size() )
		return;

	bool clipped 			= 0 != mContainer->isClipped();
	UIListBoxItem * item;
	Uint32 i, relPos = 0, relPosMax;
	Int32 itemPos, itemPosMax;
	Int32 tHLastScroll 		= mHScrollInit;

	bool wasScrollVisible 	= mVScrollBar->isVisible();
	bool wasHScrollVisible 	= mHScrollBar->isVisible();

	updateScrollBarState();

	Uint32 visibleItems 	= mContainer->getSize().getHeight() / mRowHeight;
	mItemsNotVisible 		= (Uint32)mItems.size() - visibleItems;
	Int32 scrolleable 		= (Int32)mItems.size() * mRowHeight - mContainer->getSize().getHeight();
	bool isScrollVisible 	= mVScrollBar->isVisible();
	bool isHScrollVisible 	= mHScrollBar->isVisible();
	bool FirstVisible 		= false;

	if ( clipped && mSmoothScroll ) {
		if ( scrolleable >= 0 )
			relPos 		= (Uint32)( mVScrollBar->getValue() * scrolleable );
		else
			relPos		= 0;

		relPosMax 	= relPos + mContainer->getSize().getHeight() + mRowHeight;

		if ( ( fromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == relPos ) && ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = relPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			item = mItems[i];
			itemPos = mRowHeight * i;
			itemPosMax = itemPos + mRowHeight;

			if ( ( itemPos >= (Int32)relPos || itemPosMax >= (Int32)relPos ) && ( itemPos <= (Int32)relPosMax ) ) {
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
			} else {
				eeSAFE_DELETE( mItems[i] );
				item = NULL;
			}

			if ( NULL != item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
					itemUpdateSize( item );
			}
		}
	} else {
		relPosMax		= (Uint32)mItems.size();

		if ( mItemsNotVisible > 0 ) {
			relPos 				= (Uint32)( mVScrollBar->getValue() * mItemsNotVisible );
			relPosMax			= relPos + visibleItems;
		}

		if ( ( fromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == relPos )  && ( !clipped || tHLastScroll == mHScrollInit ) )
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
				eeSAFE_DELETE( mItems[i] );
				item = NULL;
			}

			if ( NULL != item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
					itemUpdateSize( item );
			}
		}
	}


	if ( mHScrollBar->isVisible() && !mVScrollBar->isVisible() ) {
		mHScrollBar->setPosition( mHScrollPadding.Left, mDpSize.getHeight() - mHScrollBar->getSize().getHeight() + mHScrollPadding.Top );
		mHScrollBar->setSize( mDpSize.getWidth() + mHScrollPadding.Right, mHScrollBar->getSize().getHeight() + mHScrollPadding.Bottom );
	} else {
		mHScrollBar->setPosition( mHScrollPadding.Left, mDpSize.getHeight() - mHScrollBar->getSize().getHeight() + mHScrollPadding.Top );
		mHScrollBar->setSize( mDpSize.getWidth() - mVScrollBar->getSize().getWidth() + mHScrollPadding.Right, mHScrollBar->getSize().getHeight() + mHScrollPadding.Bottom );
	}

	setHScrollStep();

	invalidateDraw();
}

void UIListBox::itemKeyEvent( const KeyEvent &Event ) {
	KeyEvent ItemEvent( Event.getNode(), Event::OnItemKeyDown, Event.getKeyCode(), Event.getChar(), Event.getMod() );
	sendEvent( &ItemEvent );
}

void UIListBox::itemClicked( UIListBoxItem * Item ) {
	Event ItemEvent( Item, Event::OnItemClicked );
	sendEvent( &ItemEvent );

	if ( !( isMultiSelect() && NULL != getEventDispatcher() && getEventDispatcher()->getInput()->isKeyDown( KEY_LCTRL ) ) )
		resetItemsStates();
}

Uint32 UIListBox::onSelected() {
	NodeMessage tMsg( this, NodeMessage::Selected, 0 );
	messagePost( &tMsg );

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

UIListBoxItem * UIListBox::getItem( const Uint32& Index ) const {
	eeASSERT( Index < mItems.size() )

	return mItems[ Index ];
}

UIListBoxItem * UIListBox::getItemSelected() {
	if ( mSelected.size() ) {
		if ( NULL == mItems[ mSelected.front() ] )
			createItemIndex( mSelected.front() );

		return mItems[ mSelected.front() ];
	}

	return NULL;
}

Uint32 UIListBox::getItemSelectedIndex() const {
	if ( mSelected.size() )
		return mSelected.front();

	return eeINDEX_NOT_FOUND;
}

String UIListBox::getItemSelectedText() const {
	String tstr;

	if ( mSelected.size() )
		return mTexts[ mSelected.front() ];

	return tstr;
}

std::list<Uint32> UIListBox::getItemsSelectedIndex() const {
	return mSelected;
}

std::list<UIListBoxItem *> UIListBox::getItemsSelected() {
	std::list<UIListBoxItem *> tItems;
	std::list<Uint32>::iterator it;

	for ( it = mSelected.begin(); it != mSelected.end(); ++it ) {
		if ( NULL == mItems[ *it ] )
			createItemIndex( *it );

		tItems.push_back( mItems[ *it ] );
	}

	return tItems;
}

Uint32 UIListBox::getItemIndex( UIListBoxItem * Item ) {
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

Uint32 UIListBox::getCount() {
	return (Uint32)mItems.size();
}

void UIListBox::setSelected( const String& Text ) {
	setSelected( getItemIndex( Text ) );
}

void UIListBox::setSelected( Uint32 Index ) {
	if ( Index < mItems.size() ) {
		if ( isMultiSelect() ) {
			for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); ++it ) {
				if ( *it == Index )
					return;
			}
		} else {
			if ( mSelected.size() ) {
				if ( NULL != mItems[ mSelected.front() ] ) {
					mItems[ mSelected.front() ]->unselect();
				}

				mSelected.clear();
			}
		}

		mSelected.push_back( Index );

		if ( NULL != mItems[ Index ] ) {
			mItems[ Index ]->select();
		} else {
			updateScroll();
		}
	}
}

void UIListBox::selectPrev() {
	if ( !isMultiSelect() && mSelected.size() ) {
		Int32 SelIndex = mSelected.front() - 1;

		if ( SelIndex >= 0 ) {
			if ( NULL == mItems[ mSelected.front() ] )
				createItemIndex( mSelected.front() );

			if ( NULL == mItems[ SelIndex ] )
				createItemIndex( SelIndex );

			if ( mItems[ SelIndex ]->getPosition().y < 0 ) {
				mVScrollBar->setValue( (Float)( SelIndex * mRowHeight ) / (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[ SelIndex ]->setFocus();
			}

			setSelected( SelIndex );
		}
	}
}

void UIListBox::selectNext() {
	if ( !isMultiSelect() && mSelected.size() ) {
		Int32 SelIndex = mSelected.front() + 1;

		if ( SelIndex < (Int32)mItems.size() ) {
			if ( NULL == mItems[ mSelected.front() ] )
				createItemIndex( mSelected.front() );

			if ( NULL == mItems[ SelIndex ] )
				createItemIndex( SelIndex );

			if ( mItems[ SelIndex ]->getPosition().y + (Int32)getRowHeight() > mContainer->getSize().getHeight() ) {
				mVScrollBar->setValue( (Float)( SelIndex * mRowHeight ) / (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[ SelIndex ]->setFocus();
			}

			setSelected( SelIndex );
		}
	}
}

Uint32 UIListBox::onKeyDown( const KeyEvent &Event ) {
	UINode::onKeyDown( Event );

	if ( !mSelected.size() || mFlags & UI_MULTI_SELECT )
		return 0;

	if ( Sys::getTicks() - mLastTickMove > 100 ) {
		if ( KEY_DOWN == Event.getKeyCode() ) {
			mLastTickMove = Sys::getTicks();

			selectNext();
		} else if ( KEY_UP == Event.getKeyCode() ) {
			mLastTickMove = Sys::getTicks();

			selectPrev();
		} else if ( KEY_HOME == Event.getKeyCode() ) {
			mLastTickMove = Sys::getTicks();

			if ( mSelected.front() != 0 ) {
				mVScrollBar->setValue( 0 );

				mItems[ 0 ]->setFocus();

				setSelected( 0 );
			}
		} else if ( KEY_END == Event.getKeyCode() ) {
			mLastTickMove = Sys::getTicks();

			if ( mSelected.front() != getCount() - 1 ) {
				mVScrollBar->setValue( 1 );

				mItems[ getCount() - 1 ]->setFocus();

				setSelected( getCount() - 1 );
			}
		}
	}

	itemKeyEvent( Event );

	return 1;
}

Uint32 UIListBox::onMessage( const NodeMessage * Msg ) {
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

void UIListBox::setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		updateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIListBox::getVerticalScrollMode() {
	return mVScrollMode;
}

void UIListBox::setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
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

const UI_SCROLLBAR_MODE& UIListBox::getHorizontalScrollMode() {
	return mHScrollMode;
}

bool UIListBox::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "row-height" == name || "rowheight" == name ) {
		setRowHeight( attribute.asDpDimensionI() );
	} else if ( "vscroll-mode" == name || "vscrollmode" == name ) {
		std::string val = attribute.asString();
		if ( "auto" == val ) setVerticalScrollMode( UI_SCROLLBAR_AUTO );
		else if ( "on" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "off" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_OFF );
	} else if ( "hscroll-mode" == name || "hscrollmode" == name ) {
		std::string val = attribute.asString();
		if ( "auto" == val ) setHorizontalScrollMode( UI_SCROLLBAR_AUTO );
		else if ( "on" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "off" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_OFF );
	} else if ( "selected-index" == name || "selectedindex" == name ) {
		setSelected( attribute.asUint() );
	} else if ( "selected-text" == name || "selectedtext" == name ) {
		setSelected( attribute.asString() );
	} else if ( "scrollbar-type" == name || "scrollbartype" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );

		if ( "no-buttons" == val || "nobuttons" == val ) {
			mVScrollBar->setScrollBarType( UIScrollBar::NoButtons );
			mHScrollBar->setScrollBarType( UIScrollBar::NoButtons );
		} else if ( "two-buttons" == val || "twobuttons" == val ) {
			mVScrollBar->setScrollBarType( UIScrollBar::TwoButtons );
			mHScrollBar->setScrollBarType( UIScrollBar::NoButtons );
		}
	} else {
		return UITouchDragableWidget::setAttribute( attribute, state );
	}

	return true;
}

void UIListBox::loadFromXmlNode(const pugi::xml_node & node) {
	beginAttributesTransaction();

	UITouchDragableWidget::loadFromXmlNode( node );

	loadItemsFromXmlNode( node );

	endAttributesTransaction();
}

void UIListBox::loadItemsFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	std::vector<String> items;
	for ( pugi::xml_node item = node.child("item"); item; item = item.next_sibling("item") ) {
		std::string data = item.text().as_string();

		if ( data.size() ) {
			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				items.push_back( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( data ) );
		}
	}

	if ( items.size() ) {
		addListBoxItems( items );
	}

	endAttributesTransaction();
}

}}
