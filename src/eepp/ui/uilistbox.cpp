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
	if ( NULL == Params.Font && NULL != UIThemeManager::instance()->DefaultFont() )
		mFont = UIThemeManager::instance()->DefaultFont();

	UIControl::CreateParams CParams;
	CParams.Parent( this );
	CParams.PosSet( mPaddingContainer.Left, mPaddingContainer.Top );
	CParams.Size = Sizei( mSize.Width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.Height() - mPaddingContainer.Top - mPaddingContainer.Bottom );
	CParams.Flags = Params.Flags;
	mContainer = eeNew( UIItemContainer<UIListBox>, ( CParams ) );
	mContainer->Visible( true );
	mContainer->Enabled( true );

	if ( mFlags & UI_CLIP_ENABLE )
		mFlags &= ~UI_CLIP_ENABLE;

	UIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( this );
	ScrollBarP.Size = Sizei( 15, mSize.Height() );
	ScrollBarP.PosSet( mSize.Width() - 15, 0 );
	ScrollBarP.Flags = UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar = true;
	mVScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );

	ScrollBarP.Size = Sizei( mSize.Width() - mVScrollBar->Size().Width(), 15 );
	ScrollBarP.PosSet( 0, mSize.Height() - 15 );
	ScrollBarP.VerticalScrollBar = false;
	mHScrollBar = eeNew( UIScrollBar, ( ScrollBarP ) );

	if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
		mHScrollBar->Visible( true );
		mHScrollBar->Enabled( true );
	}

	if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
		mVScrollBar->Visible( true );
		mVScrollBar->Enabled( true );
	}

	mVScrollBar->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIListBox::OnScrollValueChange ) );
	mHScrollBar->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIListBox::OnHScrollValueChange ) );

	SetRowHeight();

	ApplyDefaultTheme();
}

UIListBox::~UIListBox() {
}

Uint32 UIListBox::Type() const {
	return UI_TYPE_LISTBOX;
}

bool UIListBox::IsType( const Uint32& type ) const {
	return UIListBox::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIListBox::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "listbox" );

	if ( NULL == mFont && NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->Theme() && NULL != mSkinState->GetSkin()->Theme()->Font() )
		mFont = mSkinState->GetSkin()->Theme()->Font();

	AutoPadding();

	OnSizeChange();
}

void UIListBox::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPaddingContainer = MakePadding();
	}
}

UIScrollBar * UIListBox::VerticalScrollBar() const {
	return mVScrollBar;
}

UIScrollBar * UIListBox::HorizontalScrollBar() const {
	return mHScrollBar;
}

void UIListBox::AddListBoxItems( std::vector<String> Texts ) {
	mItems.reserve( mItems.size() + Texts.size() );
	mTexts.reserve( mTexts.size() + Texts.size() );

	for ( Uint32 i = 0; i < Texts.size(); i++ ) {
		AddListBoxItem( Texts[i] );
	}

	UpdateScroll();
}

Uint32 UIListBox::AddListBoxItem( UIListBoxItem * Item ) {
	mItems.push_back( Item );
	mTexts.push_back( Item->Text() );

	if ( Item->Parent() != mContainer )
		Item->Parent( mContainer );


	UpdateScroll();

	Uint32 tMaxTextWidth = mMaxTextWidth;

	ItemUpdateSize( Item );

	if ( tMaxTextWidth != mMaxTextWidth ) {
		UpdateListBoxItemsSize();
		UpdateScroll();
	}

	return (Uint32)(mItems.size() - 1);
}

Uint32 UIListBox::AddListBoxItem( const String& Text ) {
	mTexts.push_back( Text );
	mItems.push_back( NULL );

	if ( NULL != mFont ) {
		Uint32 twidth = mFont->GetTextWidth( Text );

		if ( twidth > mMaxTextWidth ) {
			mMaxTextWidth = twidth;

			UpdateListBoxItemsSize();
		}
	}

	UpdateScroll();

	return (Uint32)(mItems.size() - 1);
}

UIListBoxItem * UIListBox::CreateListBoxItem( const String& Name ) {
	UITextBox::CreateParams TextParams;
	TextParams.Parent( mContainer );
	TextParams.Flags 		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	TextParams.Font 		= mFont;
	TextParams.FontColor 	= mFontColor;
	UIListBoxItem * tItem 	= eeNew( UIListBoxItem, ( TextParams ) );
	tItem->Text( Name );

	return tItem;
}

Uint32 UIListBox::RemoveListBoxItem( const String& Text ) {
	return RemoveListBoxItem( GetListBoxItemIndex( Text ) );
}

Uint32 UIListBox::RemoveListBoxItem( UIListBoxItem * Item ) {
	return RemoveListBoxItem( GetListBoxItemIndex( Item ) );
}

void UIListBox::RemoveListBoxItems( std::vector<Uint32> ItemsIndex ) {
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
				mTexts.push_back( mItems[i]->Text() );
			} else {
				eeSAFE_DELETE( mItems[i] ); // doesn't call to mItems[i]->Close(); because is not checking for close.
			}
		}

		mItems = ItemsCpy;

		UpdateScroll();
		FindMaxWidth();
		UpdateListBoxItemsSize();
	}
}

void UIListBox::Clear() {
	mTexts.clear();
	mItems.clear();
	mSelected.clear();
	mVScrollBar->Value(0);

	UpdateScroll();
	FindMaxWidth();
	UpdateListBoxItemsSize();

	SendCommonEvent( UIEvent::EventOnControlClear );
}

Uint32 UIListBox::RemoveListBoxItem( Uint32 ItemIndex ) {
	RemoveListBoxItems( std::vector<Uint32>( 1, ItemIndex ) );

	return ItemIndex;
}

Uint32 UIListBox::GetListBoxItemIndex( const String& Name ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Name == mItems[i]->Text() )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIListBox::GetListBoxItemIndex( UIListBoxItem * Item ) {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

void UIListBox::OnScrollValueChange( const UIEvent * Event ) {
	UpdateScroll( true );
}

void UIListBox::OnHScrollValueChange( const UIEvent * Event ) {
	UpdateScroll( true );
}

void UIListBox::OnSizeChange() {
	mVScrollBar->Pos( mSize.Width() - mVScrollBar->Size().Width() + mVScrollPadding.Left, mVScrollPadding.Top );
	mVScrollBar->Size( mVScrollBar->Size().Width() + mVScrollPadding.Right, mSize.Height() + mVScrollPadding.Bottom );

	mHScrollBar->Pos( mHScrollPadding.Left, mSize.Height() - mHScrollBar->Size().Height() + mHScrollPadding.Top );
	mHScrollBar->Size( mSize.Width() - mVScrollBar->Size().Width() + mHScrollPadding.Right, mHScrollBar->Size().Height() + mHScrollPadding.Bottom );

	if ( mContainer->IsClipped() && UI_SCROLLBAR_AUTO == mHScrollMode ) {
		if ( (Int32)mMaxTextWidth <= mContainer->Size().Width() ) {
			mHScrollBar->Visible( false );
			mHScrollBar->Enabled( false );
			mHScrollInit = 0;
		}
	}

	ContainerResize();
	UpdateListBoxItemsSize();
	UpdateScroll();
}

void UIListBox::SetRowHeight() {
	Uint32 tOldRowHeight = mRowHeight;

	if ( 0 == mRowHeight ) {
		Uint32 FontSize = 12;

		if ( NULL != UIThemeManager::instance()->DefaultFont() )
			FontSize = UIThemeManager::instance()->DefaultFont()->GetFontHeight();

		if ( NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->Theme() && NULL != mSkinState->GetSkin()->Theme()->Font() )
			FontSize = mSkinState->GetSkin()->Theme()->Font()->GetFontHeight();

		if ( NULL != mFont )
			FontSize = mFont->GetFontHeight();

		mRowHeight = (Uint32)( FontSize + 4 );
	}

	if ( tOldRowHeight != mRowHeight ) {
		UpdateScroll();
		UpdateListBoxItemsSize();
	}
}

void UIListBox::FindMaxWidth() {
	Uint32 size = (Uint32)mItems.size();
	Int32 width;

	mMaxTextWidth = 0;

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( NULL != mItems[i] )
			width = (Int32)mItems[i]->GetTextWidth();
		else
			width = mFont->GetTextWidth( mTexts[i] );

		if ( width > (Int32)mMaxTextWidth )
			mMaxTextWidth = (Uint32)width;
	}
}

void UIListBox::UpdateListBoxItemsSize() {
	Uint32 size = (Uint32)mItems.size();

	for ( Uint32 i = 0; i < size; i++ )
		ItemUpdateSize( mItems[i] );
}

void UIListBox::ItemUpdateSize( UIListBoxItem * Item ) {
	if ( NULL != Item ) {
		Int32 width = (Int32)Item->GetTextWidth();

		if ( width > (Int32)mMaxTextWidth )
			mMaxTextWidth = (Uint32)width;

		if ( !mHScrollBar->Visible() ) {
			if ( width < mContainer->Size().Width() )
				width = mContainer->Size().Width();

			if ( ( mItemsNotVisible > 0 && UI_SCROLLBAR_AUTO == mVScrollMode ) || UI_SCROLLBAR_ALWAYS_ON == mVScrollMode )
				width -= mVScrollBar->Size().Width();
		} else {
			width = mMaxTextWidth;
		}

		Item->Size( width, mRowHeight );
	}
}

void UIListBox::ContainerResize() {
	mContainer->Pos( mPaddingContainer.Left, mPaddingContainer.Top );

	if( mHScrollBar->Visible() )
		mContainer->Size( mSize.Width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.Height() - mPaddingContainer.Top - mHScrollBar->Size().Height() );
	else
		mContainer->Size( mSize.Width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.Height() - mPaddingContainer.Bottom - mPaddingContainer.Top );
}

void UIListBox::CreateItemIndex( const Uint32& i ) {
	if ( NULL == mItems[i] ) {
		mItems[i] = CreateListBoxItem( mTexts[i] );

		ItemUpdateSize( mItems[i] );

		for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); it++ ) {
			if ( *it == i ) {
				mItems[i]->Select();

				break;
			}
		}
	}
}

void UIListBox::UpdateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	UIListBoxItem * Item;
	Uint32 i, RelPos = 0, RelPosMax;
	Int32 ItemPos, ItemPosMax;
	Int32 tHLastScroll 		= mHScrollInit;

	Uint32 VisibleItems 	= mContainer->Size().Height() / mRowHeight;
	mItemsNotVisible 		= (Int32)mItems.size() - VisibleItems;

	bool wasScrollVisible 	= mVScrollBar->Visible();
	bool wasHScrollVisible 	= mHScrollBar->Visible();

	bool Clipped 			= 0 != mContainer->IsClipped();

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

	if ( Clipped && ( UI_SCROLLBAR_AUTO == mHScrollMode || UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) ) {
		if ( ( mVScrollBar->Visible() && mContainer->Size().Width() - mVScrollBar->Size().Width() < (Int32)mMaxTextWidth ) ||
			( !mVScrollBar->Visible() && mContainer->Size().Width() < (Int32)mMaxTextWidth ) ) {
				mHScrollBar->Visible( true );
				mHScrollBar->Enabled( true );

				ContainerResize();

				Int32 ScrollH;

				if ( mVScrollBar->Visible() )
					ScrollH = mMaxTextWidth - mContainer->Size().Width() + mVScrollBar->Size().Width();
				else
					ScrollH = mMaxTextWidth - mContainer->Size().Width();

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

	VisibleItems 			= mContainer->Size().Height() / mRowHeight;
	mItemsNotVisible 		= (Uint32)mItems.size() - VisibleItems;
	Int32 Scrolleable 		= (Int32)mItems.size() * mRowHeight - mContainer->Size().Height();
	bool isScrollVisible 	= mVScrollBar->Visible();
	bool isHScrollVisible 	= mHScrollBar->Visible();
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
				if ( NULL == Item ) {
					CreateItemIndex( i );
					Item = mItems[i];
				}

				Item->Pos( mHScrollInit, ItemPos - RelPos );
				Item->Enabled( true );
				Item->Visible( true );

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
					ItemUpdateSize( Item );
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
				if ( NULL == Item ) {
					CreateItemIndex( i );
					Item = mItems[i];
				}

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
				eeSAFE_DELETE( mItems[i] );
				Item = NULL;
			}

			if ( NULL != Item ) {
				if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
					ItemUpdateSize( Item );
			}
		}
	}
	
	
	if ( mHScrollBar->Visible() && !mVScrollBar->Visible() ) {
		mHScrollBar->Pos( mHScrollPadding.Left, mSize.Height() - mHScrollBar->Size().Height() + mHScrollPadding.Top );
		mHScrollBar->Size( mSize.Width() + mHScrollPadding.Right, mHScrollBar->Size().Height() + mHScrollPadding.Bottom );
	} else {
		mHScrollBar->Pos( mHScrollPadding.Left, mSize.Height() - mHScrollBar->Size().Height() + mHScrollPadding.Top );
		mHScrollBar->Size( mSize.Width() - mVScrollBar->Size().Width() + mHScrollPadding.Right, mHScrollBar->Size().Height() + mHScrollPadding.Bottom );
	}
}

void UIListBox::ItemKeyEvent( const UIEventKey &Event ) {
	UIEventKey ItemEvent( Event.Ctrl(), UIEvent::EventOnItemKeyDown, Event.KeyCode(), Event.Char(), Event.Mod() );
	SendEvent( &ItemEvent );
}

void UIListBox::ItemClicked( UIListBoxItem * Item ) {
	UIEvent ItemEvent( Item, UIEvent::EventOnItemClicked );
	SendEvent( &ItemEvent );

	if ( !( IsMultiSelect() && UIManager::instance()->GetInput()->IsKeyDown( KEY_LCTRL ) ) )
		ResetItemsStates();
}

Uint32 UIListBox::OnSelected() {
	UIMessage tMsg( this, UIMessage::MsgSelected, 0 );
	MessagePost( &tMsg );

	SendCommonEvent( UIEvent::EventOnItemSelected );

	return 1;
}

void UIListBox::ResetItemsStates() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( NULL != mItems[i] )
			mItems[i]->Unselect();
	}
}

bool UIListBox::IsMultiSelect() const {
	return 0 != ( mFlags & UI_MULTI_SELECT );
}

UIListBoxItem * UIListBox::GetItem( const Uint32& Index ) const {
	eeASSERT( Index < mItems.size() )

	return mItems[ Index ];
}

UIListBoxItem * UIListBox::GetItemSelected() {
	if ( mSelected.size() ) {
		if ( NULL == mItems[ mSelected.front() ] )
			CreateItemIndex( mSelected.front() );

		return mItems[ mSelected.front() ];
	}

	return NULL;
}

Uint32 UIListBox::GetItemSelectedIndex() const {
	if ( mSelected.size() )
		return mSelected.front();

	return eeINDEX_NOT_FOUND;
}

String UIListBox::GetItemSelectedText() const {
	String tstr;

	if ( mSelected.size() )
		return mTexts[ mSelected.front() ];

	return tstr;
}

std::list<Uint32> UIListBox::GetItemsSelectedIndex() const {
	return mSelected;
}

std::list<UIListBoxItem *> UIListBox::GetItemsSelected() {
	std::list<UIListBoxItem *> tItems;
	std::list<Uint32>::iterator it;

	for ( it = mSelected.begin(); it != mSelected.end(); it++ ) {
		if ( NULL == mItems[ *it ] )
			CreateItemIndex( *it );

		tItems.push_back( mItems[ *it ] );
	}

	return tItems;
}

Uint32 UIListBox::GetItemIndex( UIListBoxItem * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIListBox::GetItemIndex( const String& Text ) {
	for ( Uint32 i = 0; i < mTexts.size(); i++ ) {
		if ( Text == mTexts[i] )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

void UIListBox::FontColor( const ColorA& Color ) {
	mFontColor = Color;

	for ( Uint32 i = 0; i < mItems.size(); i++ )
		mItems[i]->Color( mFontColor );
}

const ColorA& UIListBox::FontColor() const {
	return mFontColor;
}

void UIListBox::FontOverColor( const ColorA& Color ) {
	mFontOverColor = Color;
}

const ColorA& UIListBox::FontOverColor() const {
	return mFontOverColor;
}

void UIListBox::FontSelectedColor( const ColorA& Color ) {
	mFontSelectedColor = Color;
}

const ColorA& UIListBox::FontSelectedColor() const {
	return mFontSelectedColor;
}

void UIListBox::Font( Graphics::Font * Font ) {
	mFont = Font;

	for ( Uint32 i = 0; i < mItems.size(); i++ )
		mItems[i]->Font( mFont );

	FindMaxWidth();
	UpdateListBoxItemsSize();
	UpdateScroll();
}

Graphics::Font * UIListBox::Font() const {
	return mFont;
}

void UIListBox::PaddingContainer( const Recti& Padding ) {
	if ( Padding != mPaddingContainer ) {
		mPaddingContainer = Padding;

		ContainerResize();
		UpdateScroll();
	}
}

const Recti& UIListBox::PaddingContainer() const {
	return mPaddingContainer;
}

void UIListBox::SmoothScroll( const bool& soft ) {
	if ( soft != mSmoothScroll ) {
		mSmoothScroll = soft;

		UpdateScroll();
	}
}

const bool& UIListBox::SmoothScroll() const {
	return mSmoothScroll;
}

void UIListBox::RowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		UpdateListBoxItemsSize();
		UpdateScroll();
	}
}

const Uint32& UIListBox::RowHeight() const {
	return mRowHeight;
}

Uint32 UIListBox::Count() {
	return (Uint32)mItems.size();
}

void UIListBox::SetSelected( const String& Text ) {
	SetSelected( GetItemIndex( Text ) );
}

void UIListBox::SetSelected( Uint32 Index ) {
	if ( Index < mItems.size() ) {
		if ( IsMultiSelect() ) {
			for ( std::list<Uint32>::iterator it = mSelected.begin(); it != mSelected.end(); it++ ) {
				if ( *it == Index )
					return;
			}
		} else {
			if ( mSelected.size() ) {
				if ( NULL != mItems[ mSelected.front() ] ) {
					mItems[ mSelected.front() ]->Unselect();
				}

				mSelected.clear();
			}
		}

		mSelected.push_back( Index );

		if ( NULL != mItems[ Index ] ) {
			mItems[ Index ]->Select();
		} else {
			UpdateScroll();
		}
	}
}

void UIListBox::SelectPrev() {
	if ( !IsMultiSelect() && mSelected.size() ) {
		Int32 SelIndex = mSelected.front() - 1;

		if ( SelIndex >= 0 ) {
			if ( NULL == mItems[ mSelected.front() ] )
				CreateItemIndex( mSelected.front() );

			if ( NULL == mItems[ SelIndex ] )
				CreateItemIndex( SelIndex );

			if ( mItems[ SelIndex ]->Pos().y < 0 ) {
				mVScrollBar->Value( (Float)( SelIndex * mRowHeight ) / (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[ SelIndex ]->SetFocus();
			}

			SetSelected( SelIndex );
		}
	}
}

void UIListBox::SelectNext() {
	if ( !IsMultiSelect() && mSelected.size() ) {
		Int32 SelIndex = mSelected.front() + 1;

		if ( SelIndex < (Int32)mItems.size() ) {
			if ( NULL == mItems[ mSelected.front() ] )
				CreateItemIndex( mSelected.front() );

			if ( NULL == mItems[ SelIndex ] )
				CreateItemIndex( SelIndex );

			if ( mItems[ SelIndex ]->Pos().y + (Int32)RowHeight() > mContainer->Size().Height() ) {
				mVScrollBar->Value( (Float)( SelIndex * mRowHeight ) / (Float)( ( mItems.size() - 1 ) * mRowHeight ) );

				mItems[ SelIndex ]->SetFocus();
			}

			SetSelected( SelIndex );
		}
	}
}

Uint32 UIListBox::OnKeyDown( const UIEventKey &Event ) {
	UIControlAnim::OnKeyDown( Event );

	if ( !mSelected.size() || mFlags & UI_MULTI_SELECT )
		return 0;

	if ( Sys::GetTicks() - mLastTickMove > 100 ) {
		if ( KEY_DOWN == Event.KeyCode() ) {
			mLastTickMove = Sys::GetTicks();

			SelectNext();
		} else if ( KEY_UP == Event.KeyCode() ) {
			mLastTickMove = Sys::GetTicks();

			SelectPrev();
		} else if ( KEY_HOME == Event.KeyCode() ) {
			mLastTickMove = Sys::GetTicks();

			if ( mSelected.front() != 0 ) {
				mVScrollBar->Value( 0 );

				mItems[ 0 ]->SetFocus();

				SetSelected( 0 );
			}
		} else if ( KEY_END == Event.KeyCode() ) {
			mLastTickMove = Sys::GetTicks();

			if ( mSelected.front() != Count() - 1 ) {
				mVScrollBar->Value( 1 );

				mItems[ Count() - 1 ]->SetFocus();

				SetSelected( Count() - 1 );
			}
		}
	}

	ItemKeyEvent( Event );

	return 1;
}

Uint32 UIListBox::OnMessage( const UIMessage * Msg ) {
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

void UIListBox::OnAlphaChange() {
	UIComplexControl::OnAlphaChange();

	if ( mItems.size() ) {
		for ( Uint32 i = mVisibleFirst; i <= mVisibleLast; i++ ) {
			if ( NULL != mItems[i] )
				mItems[i]->Alpha( mAlpha );
		}
	}

	mVScrollBar->Alpha( mAlpha );
	mHScrollBar->Alpha( mAlpha );
}

void UIListBox::VerticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;

		UpdateScroll();
	}
}

const UI_SCROLLBAR_MODE& UIListBox::VerticalScrollMode() {
	return mVScrollMode;
}

void UIListBox::HorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
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

const UI_SCROLLBAR_MODE& UIListBox::HorizontalScrollMode() {
	return mHScrollMode;
}

bool UIListBox::TouchDragEnable() const {
	return 0 != ( mFlags & UI_TOUCH_DRAG_ENABLED );
}

void UIListBox::TouchDragEnable( const bool& enable ) {
	WriteFlag( UI_TOUCH_DRAG_ENABLED, true == enable );
}

bool UIListBox::TouchDragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_TOUCH_DRAGGING );
}

void UIListBox::TouchDragging( const bool& dragging ) {
	WriteCtrlFlag( UI_CTRL_FLAG_TOUCH_DRAGGING, true == dragging );
}

void UIListBox::Update() {
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
