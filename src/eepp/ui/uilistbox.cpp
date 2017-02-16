#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uiitemcontainer.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UIListBox::UIListBox( UIListBox::CreateParams& Params ) :
	UIComplexControl( Params ),
	mRowHeight( Params.RowHeight ),
	mVScrollMode( Params.VScrollMode ),
	mHScrollMode( Params.HScrollMode ),
	mSmoothScroll( Params.SmoothScroll ),
	mPaddingContainer( Params.PaddingContainer ),
	mHScrollPadding( Params.HScrollPadding ),
	mVScrollPadding( Params.VScrollPadding ),
	mContainer( NULL ),
	mVScrollBar( NULL ),
	mHScrollBar( NULL ),
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontOverColor( Params.FontOverColor ),
	mFontSelectedColor( Params.FontSelectedColor ),
	mLastPos( eeINDEX_NOT_FOUND ),
	mMaxTextWidth(0),
	mHScrollInit(0),
	mItemsNotVisible(0),
	mLastTickMove(0),
	mVisibleFirst(0),
	mVisibleLast(0),
	mTouchDragAcceleration(0),
	mTouchDragDeceleration( Params.TouchDragDeceleration )
{
	if ( NULL == Params.Font && NULL != UIThemeManager::instance()->defaultFont() )
		mFont = UIThemeManager::instance()->defaultFont();

	UIControl::CreateParams CParams;
	CParams.Parent( this );
	CParams.PosSet( mPaddingContainer.Left, mPaddingContainer.Top );
	CParams.Size = Sizei( mSize.width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.height() - mPaddingContainer.Top - mPaddingContainer.Bottom );
	CParams.Flags = Params.Flags;
	mContainer = eeNew( UIItemContainer<UIListBox>, ( CParams ) );
	mContainer->visible( true );
	mContainer->enabled( true );

	if ( mFlags & UI_CLIP_ENABLE )
		mFlags &= ~UI_CLIP_ENABLE;

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( this );
	ScrollBarP.Size = Sizei( 15, mSize.height() );
	ScrollBarP.PosSet( mSize.width() - 15, 0 );
	ScrollBarP.Flags = UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar = true;
	mVScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );

	ScrollBarP.Size = Sizei( mSize.width() - mVScrollBar->size().width(), 15 );
	ScrollBarP.PosSet( 0, mSize.height() - 15 );
	ScrollBarP.VerticalScrollBar = false;
	mHScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );

	if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
		mHScrollBar->visible( true );
		mHScrollBar->enabled( true );
	}

	if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
		mVScrollBar->visible( true );
		mVScrollBar->enabled( true );
	}

	mVScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIListBox::onScrollValueChange ) );
	mHScrollBar->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIListBox::onHScrollValueChange ) );

	setRowHeight();

	applyDefaultTheme();
}

UIListBox::~UIListBox() {
}

Uint32 UIListBox::getType() const {
	return UI_TYPE_LISTBOX;
}

bool UIListBox::isType( const Uint32& type ) const {
	return UIListBox::getType() == type ? true : UIComplexControl::isType( type );
}

void UIListBox::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "listbox" );

	if ( NULL == mFont && NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->theme() && NULL != mSkinState->getSkin()->theme()->font() )
		mFont = mSkinState->getSkin()->theme()->font();

	autoPadding();

	onSizeChange();
}

void UIListBox::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPaddingContainer = makePadding();
	}
}

UIScrollBar * UIListBox::verticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar * UIListBox::horizontalScrollBar() const {
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
	mTexts.push_back( Item->text() );

	if ( Item->parent() != mContainer )
		Item->parent( mContainer );


	updateScroll();

	Uint32 tMaxTextWidth = mMaxTextWidth;

	itemUpdateSize( Item );

	if ( tMaxTextWidth != mMaxTextWidth ) {
		updateListBoxItemsSize();
		updateScroll();
	}

	return (Uint32)(mItems.size() - 1);
}

Uint32 UIListBox::addListBoxItem( const String& Text ) {
	mTexts.push_back( Text );
	mItems.push_back( NULL );

	if ( NULL != mFont ) {
		Uint32 twidth = mFont->getTextWidth( Text );

		if ( twidth > mMaxTextWidth ) {
			mMaxTextWidth = twidth;

			updateListBoxItemsSize();
		}
	}

	updateScroll();

	return (Uint32)(mItems.size() - 1);
}

UIListBoxItem * UIListBox::createListBoxItem( const String& Name ) {
	UITextBox::CreateParams TextParams;
	TextParams.Parent( mContainer );
	TextParams.Flags 		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	TextParams.Font 		= mFont;
	TextParams.FontColor 	= mFontColor;
	UIListBoxItem * tItem 	= eeNew( UIListBoxItem, ( TextParams ) );
	tItem->text( Name );

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
		bool erase;
		mTexts.clear();

		for ( Uint32 i = 0; i < mItems.size(); i++ ) {
			erase = false;

			for ( Uint32 z = 0; z < ItemsIndex.size(); z++ ) {
				if ( ItemsIndex[z] == i ) {
					for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); it++ ) {
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
				mTexts.push_back( mItems[i]->text() );
			} else {
				eeSAFE_DELETE( mItems[i] ); // doesn't call to mItems[i]->Close(); because is not checking for close.
			}
		}

		mItems = ItemsCpy;

		updateScroll();
		findMaxWidth();
		updateListBoxItemsSize();
	}
}

void UIListBox::clear() {
	mTexts.clear();
	mItems.clear();
	mSelected.clear();
	mVScrollBar->value(0);

	updateScroll();
	findMaxWidth();
	updateListBoxItemsSize();

	sendCommonEvent( UIEvent::EventOnControlClear );
}

Uint32 UIListBox::removeListBoxItem( Uint32 ItemIndex ) {
	removeListBoxItems( std::vector<Uint32>( 1, ItemIndex ) );

	return ItemIndex;
}

Uint32 UIListBox::getListBoxItemIndex( const String& Name ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Name == mItems[i]->text() )
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

void UIListBox::onScrollValueChange( const UIEvent * Event ) {
	updateScroll( true );
}

void UIListBox::onHScrollValueChange( const UIEvent * Event ) {
	updateScroll( true );
}

void UIListBox::onSizeChange() {
	mVScrollBar->position( mSize.width() - mVScrollBar->size().width() + mVScrollPadding.Left, mVScrollPadding.Top );
	mVScrollBar->size( mVScrollBar->size().width() + mVScrollPadding.Right, mSize.height() + mVScrollPadding.Bottom );

	mHScrollBar->position( mHScrollPadding.Left, mSize.height() - mHScrollBar->size().height() + mHScrollPadding.Top );
	mHScrollBar->size( mSize.width() - mVScrollBar->size().width() + mHScrollPadding.Right, mHScrollBar->size().height() + mHScrollPadding.Bottom );

	if ( mContainer->isClipped() && UI_SCROLLBAR_AUTO == mHScrollMode ) {
		if ( (Int32)mMaxTextWidth <= mContainer->size().width() ) {
			mHScrollBar->visible( false );
			mHScrollBar->enabled( false );
			mHScrollInit = 0;
		}
	}

	containerResize();
	updateListBoxItemsSize();
	updateScroll();
}

void UIListBox::setRowHeight() {
	Uint32 tOldRowHeight = mRowHeight;

	if ( 0 == mRowHeight ) {
		Uint32 FontSize = 12;

		if ( NULL != UIThemeManager::instance()->defaultFont() )
			FontSize = UIThemeManager::instance()->defaultFont()->getFontHeight();

		if ( NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->theme() && NULL != mSkinState->getSkin()->theme()->font() )
			FontSize = mSkinState->getSkin()->theme()->font()->getFontHeight();

		if ( NULL != mFont )
			FontSize = mFont->getFontHeight();

		mRowHeight = (Uint32)( FontSize + 4 );
	}

	if ( tOldRowHeight != mRowHeight ) {
		updateScroll();
		updateListBoxItemsSize();
	}
}

void UIListBox::findMaxWidth() {
	Uint32 size = (Uint32)mItems.size();
	Int32 width;

	mMaxTextWidth = 0;

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( NULL != mItems[i] )
			width = (Int32)mItems[i]->getTextWidth();
		else
			width = mFont->getTextWidth( mTexts[i] );

		if ( width > (Int32)mMaxTextWidth )
			mMaxTextWidth = (Uint32)width;
	}
}

void UIListBox::updateListBoxItemsSize() {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ )
		itemUpdateSize( mItems[i] );
}

void UIListBox::itemUpdateSize( UIListBoxItem * Item ) {
	if ( NULL != Item ) {
		Int32 width = (Int32)Item->getTextWidth();

		if ( width > (Int32)mMaxTextWidth )
			mMaxTextWidth = (Uint32)width;

		if ( !mHScrollBar->visible() ) {
			if ( width < mContainer->size().width() )
				width = mContainer->size().width();

			if ( ( mItemsNotVisible > 0 && UI_SCROLLBAR_AUTO == mVScrollMode ) || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode )
				width -= mVScrollBar->size().width();
		} else {
			width = mMaxTextWidth;
		}

		Item->size( width, mRowHeight );
	}
}

void UIListBox::containerResize() {
	mContainer->position( mPaddingContainer.Left, mPaddingContainer.Top );

	if( mHScrollBar->visible() )
		mContainer->size( mSize.width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.height() - mPaddingContainer.Top - mHScrollBar->size().height() );
	else
		mContainer->size( mSize.width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.height() - mPaddingContainer.Bottom - mPaddingContainer.Top );
}

void UIListBox::createItemIndex( const Uint32& i ) {
	if ( NULL == mItems[i] ) {
		mItems[i] = createListBoxItem( mTexts[i] );

		itemUpdateSize( mItems[i] );

		for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); it++ ) {
			if ( *it == i ) {
				mItems[i]->select();

				break;
			}
		}
	}
}

void UIListBox::updateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	UIListBoxItem * Item;
	Uint32 i, RelPos = 0, RelPosMax;
	Int32 ItemPos, ItemPosMax;
	Int32 tHLastScroll 		= mHScrollInit;

	Uint32 VisibleItems 	= mContainer->size().height() / mRowHeight;
	mItemsNotVisible 		= (Int32)mItems.size() - VisibleItems;

	bool wasScrollVisible 	= mVScrollBar->visible();
	bool wasHScrollVisible 	= mHScrollBar->visible();

	bool Clipped 			= 0 != mContainer->isClipped();

	if ( mItemsNotVisible <= 0 ) {
		if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
			mVScrollBar->visible( true );
			mVScrollBar->enabled( true );
		} else {
			mVScrollBar->visible( false );
			mVScrollBar->enabled( false );
		}
	} else {
		if ( UI_SCROLLBAR_AUTO == mVScrollMode || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
			mVScrollBar->visible( true );
			mVScrollBar->enabled( true );
		} else {
			mVScrollBar->visible( false );
			mVScrollBar->enabled( false );
		}
	}

	if ( Clipped && ( UI_SCROLLBAR_AUTO == mHScrollMode || UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) ) {
		if ( ( mVScrollBar->visible() && mContainer->size().width() - mVScrollBar->size().width() < (Int32)mMaxTextWidth ) ||
			( !mVScrollBar->visible() && mContainer->size().width() < (Int32)mMaxTextWidth ) ) {
				mHScrollBar->visible( true );
				mHScrollBar->enabled( true );

				containerResize();

				Int32 ScrollH;

				if ( mVScrollBar->visible() )
					ScrollH = mMaxTextWidth - mContainer->size().width() + mVScrollBar->size().width();
				else
					ScrollH = mMaxTextWidth - mContainer->size().width();

				Int32 HScrolleable = (Uint32)( mHScrollBar->value() * ScrollH );

				mHScrollInit = -HScrolleable;
		} else {
			if ( UI_SCROLLBAR_AUTO == mHScrollMode ) {
				mHScrollBar->visible( false );
				mHScrollBar->enabled( false );

				mHScrollInit = 0;

				containerResize();
			}
		}
	}

	VisibleItems 			= mContainer->size().height() / mRowHeight;
	mItemsNotVisible 		= (Uint32)mItems.size() - VisibleItems;
	Int32 Scrolleable 		= (Int32)mItems.size() * mRowHeight - mContainer->size().height();
	bool isScrollVisible 	= mVScrollBar->visible();
	bool isHScrollVisible 	= mHScrollBar->visible();
	bool FirstVisible 		= false;

	if ( Clipped && mSmoothScroll ) {
		if ( Scrolleable >= 0 )
			RelPos 		= (Uint32)( mVScrollBar->value() * Scrolleable );
		else
			RelPos		= 0;

		RelPosMax 	= RelPos + mContainer->size().height() + mRowHeight;

		if ( ( FromScrollChange && eeINDEX_NOT_FOUND != mLastPos && mLastPos == RelPos ) && ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * i;
			ItemPosMax = ItemPos + mRowHeight;

			if ( ( ItemPos >= (Int32)RelPos || ItemPosMax >= (Int32)RelPos ) && ( ItemPos <= (Int32)RelPosMax ) ) {
				if ( NULL == Item ) {
					createItemIndex( i );
					Item = mItems[i];
				}

				Item->position( mHScrollInit, ItemPos - RelPos );
				Item->enabled( true );
				Item->visible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				eeSAFE_DELETE( mItems[i] );
				Item = NULL;
			}

			if ( NULL != Item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
					itemUpdateSize( Item );
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
				if ( NULL == Item ) {
					createItemIndex( i );
					Item = mItems[i];
				}

				if ( Clipped )
					Item->position( mHScrollInit, ItemPos );
				else
					Item->position( 0, ItemPos );

				Item->enabled( true );
				Item->visible( true );

				if ( !FirstVisible ) {
					mVisibleFirst = i;
					FirstVisible = true;
				}

				mVisibleLast = i;
			} else {
				eeSAFE_DELETE( mItems[i] );
				Item = NULL;
			}

			if ( NULL != Item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
					itemUpdateSize( Item );
			}
		}
	}
	
	
	if ( mHScrollBar->visible() && !mVScrollBar->visible() ) {
		mHScrollBar->position( mHScrollPadding.Left, mSize.height() - mHScrollBar->size().height() + mHScrollPadding.Top );
		mHScrollBar->size( mSize.width() + mHScrollPadding.Right, mHScrollBar->size().height() + mHScrollPadding.Bottom );
	} else {
		mHScrollBar->position( mHScrollPadding.Left, mSize.height() - mHScrollBar->size().height() + mHScrollPadding.Top );
		mHScrollBar->size( mSize.width() - mVScrollBar->size().width() + mHScrollPadding.Right, mHScrollBar->size().height() + mHScrollPadding.Bottom );
	}
}

void UIListBox::itemKeyEvent( const UIEventKey &Event ) {
	UIEventKey ItemEvent( Event.getControl(), UIEvent::EventOnItemKeyDown, Event.getKeyCode(), Event.getChar(), Event.getMod() );
	sendEvent( &ItemEvent );
}

void UIListBox::itemClicked( UIListBoxItem * Item ) {
	UIEvent ItemEvent( Item, UIEvent::EventOnItemClicked );
	sendEvent( &ItemEvent );

	if ( !( isMultiSelect() && UIManager::instance()->getInput()->isKeyDown( KEY_LCTRL ) ) )
		resetItemsStates();
}

Uint32 UIListBox::onSelected() {
	UIMessage tMsg( this, UIMessage::MsgSelected, 0 );
	messagePost( &tMsg );

	sendCommonEvent( UIEvent::EventOnItemSelected );

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

	for ( it = mSelected.begin(); it != mSelected.end(); it++ ) {
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

void UIListBox::fontColor( const ColorA& Color ) {
	mFontColor = Color;

	for ( Uint32 i = 0; i < mItems.size(); i++ )
		mItems[i]->color( mFontColor );
}

const ColorA& UIListBox::fontColor() const {
	return mFontColor;
}

void UIListBox::fontOverColor( const ColorA& Color ) {
	mFontOverColor = Color;
}

const ColorA& UIListBox::fontOverColor() const {
	return mFontOverColor;
}

void UIListBox::fontSelectedColor( const ColorA& Color ) {
	mFontSelectedColor = Color;
}

const ColorA& UIListBox::fontSelectedColor() const {
	return mFontSelectedColor;
}

void UIListBox::font( Graphics::Font * Font ) {
	mFont = Font;

	for ( Uint32 i = 0; i < mItems.size(); i++ )
		mItems[i]->font( mFont );

	findMaxWidth();
	updateListBoxItemsSize();
	updateScroll();
}

Graphics::Font * UIListBox::font() const {
	return mFont;
}

void UIListBox::paddingContainer( const Recti& Padding ) {
	if ( Padding != mPaddingContainer ) {
		mPaddingContainer = Padding;

		containerResize();
		updateScroll();
	}
}

const Recti& UIListBox::paddingContainer() const {
	return mPaddingContainer;
}

void UIListBox::smoothScroll( const bool& soft ) {
	if ( soft != mSmoothScroll ) {
		mSmoothScroll = soft;

		updateScroll();
	}
}

const bool& UIListBox::smoothScroll() const {
	return mSmoothScroll;
}

void UIListBox::rowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		updateListBoxItemsSize();
		updateScroll();
	}
}

const Uint32& UIListBox::rowHeight() const {
	return mRowHeight;
}

Uint32 UIListBox::count() {
	return (Uint32)mItems.size();
}

void UIListBox::setSelected( const String& Text ) {
	setSelected( getItemIndex( Text ) );
}

void UIListBox::setSelected( Uint32 Index ) {
	if ( Index < mItems.size() ) {
		if ( isMultiSelect() ) {
			for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); it++ ) {
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

			if ( mItems[ SelIndex ]->position().y < 0 ) {
				mVScrollBar->value( (Float)( SelIndex * mRowHeight ) / (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

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

			if ( mItems[ SelIndex ]->position().y + (Int32)rowHeight() > mContainer->size().height() ) {
				mVScrollBar->value( (Float)( SelIndex * mRowHeight ) / (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[ SelIndex ]->setFocus();
			}

			setSelected( SelIndex );
		}
	}
}

Uint32 UIListBox::onKeyDown( const UIEventKey &Event ) {
	UIControlAnim::onKeyDown( Event );

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
				mVScrollBar->value( 0 );

				mItems[ 0 ]->setFocus();

				setSelected( 0 );
			}
		} else if ( KEY_END == Event.getKeyCode() ) {
			mLastTickMove = Sys::getTicks();

			if ( mSelected.front() != count() - 1 ) {
				mVScrollBar->value( 1 );

				mItems[ count() - 1 ]->setFocus();

				setSelected( count() - 1 );
			}
		}
	}

	itemKeyEvent( Event );

	return 1;
}

Uint32 UIListBox::onMessage( const UIMessage * Msg ) {
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

void UIListBox::onAlphaChange() {
	UIComplexControl::onAlphaChange();

	if ( mItems.size() ) {
		for ( Uint32 i = mVisibleFirst; i <= mVisibleLast; i++ ) {
			if ( NULL != mItems[i] )
				mItems[i]->alpha( mAlpha );
		}
	}

	mVScrollBar->alpha( mAlpha );
	mHScrollBar->alpha( mAlpha );
}

void UIListBox::verticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		updateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIListBox::verticalScrollMode() {
	return mVScrollMode;
}

void UIListBox::horizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;

		if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
			mHScrollBar->visible( true );
			mHScrollBar->enabled( true );
			containerResize();
		} else if ( UI_SCROLLBAR_ALWAYS_OFF == mHScrollMode ) {
			mHScrollBar->visible( false );
			mHScrollBar->enabled( false );
			containerResize();
		}

		updateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIListBox::horizontalScrollMode() {
	return mHScrollMode;
}

bool UIListBox::touchDragEnable() const {
	return 0 != ( mFlags & UI_TOUCH_DRAG_ENABLED );
}

void UIListBox::touchDragEnable( const bool& enable ) {
	writeFlag( UI_TOUCH_DRAG_ENABLED, true == enable );
}

bool UIListBox::touchDragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING );
}

void UIListBox::touchDragging( const bool& dragging ) {
	writeCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, true == dragging );
}

void UIListBox::update() {
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
