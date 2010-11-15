#include "cuilistbox.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIListBox::cUIListBox( cUIListBox::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mRowHeight( Params.RowHeight ),
	mScrollAlwaysVisible( Params.ScrollAlwaysVisible ),
	mSoftScroll( Params.SoftScroll ),
	mPaddingContainer( Params.PaddingContainer ),
	mContainer( NULL ),
	mScrollBar( NULL ),
	mHScrollBar( NULL ),
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontOverColor( Params.FontOverColor ),
	mFontSelectedColor( Params.FontSelectedColor ),
	mLastPos( 0xFFFFFFFF ),
	mDisableScrollUpdate( false ),
	mMaxTextWidth(0),
	mAllowHorizontalScroll( Params.AllowHorizontalScroll ),
	mHScrollInit(0),
	mItemsNotVisible(0)
{
	mType |= UI_TYPE_LISTBOX;

	cUIControl::CreateParams CParams;
	CParams.Parent( this );
	CParams.PosSet( mPaddingContainer.Left, mPaddingContainer.Top );
	CParams.Size = eeSize( mSize.Width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.Height() - mPaddingContainer.Top - mPaddingContainer.Bottom );
	CParams.Flags = Params.Flags;
	mContainer = eeNew( cUIControl, ( CParams ) );
	mContainer->Visible( true );
	mContainer->Enabled( true );

	if ( mFlags & UI_CLIP_ENABLE )
		mFlags &= ~UI_CLIP_ENABLE;

	cUIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( this );
	ScrollBarP.Size = eeSize( 15, mSize.Height() );
	ScrollBarP.PosSet( mSize.Width() - 15, 0 );
	ScrollBarP.Flags = UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar = true;
	mScrollBar = eeNew( cUIScrollBar, ( ScrollBarP ) );

	ScrollBarP.Size = eeSize( mSize.Width() - mScrollBar->Size().Width(), 15 );
	ScrollBarP.PosSet( 0, mSize.Height() - 15 );
	ScrollBarP.VerticalScrollBar = false;
	mHScrollBar = eeNew( cUIScrollBar, ( ScrollBarP ) );
	mHScrollBar->Visible( false );
	mHScrollBar->Enabled( false );

	if ( mScrollAlwaysVisible ) {
		mScrollBar->Visible( true );
		mScrollBar->Enabled( true );
	}

	mScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIListBox::OnScrollValueChange ) );
	mHScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIListBox::OnHScrollValueChange ) );

	SetRowHeight();

	ApplyDefaultTheme();
}

cUIListBox::~cUIListBox() {
}

void cUIListBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "listbox" );

	AutoPadding();

	OnSizeChange();
}

void cUIListBox::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPaddingContainer = MakePadding();
	}
}

cUIScrollBar * cUIListBox::ScrollBar() const {
	return mScrollBar;
}

cUIScrollBar * cUIListBox::HScrollBar() const {
	return mHScrollBar;
}

void cUIListBox::AddListBoxItems( std::vector<std::wstring> Texts ) {
	mDisableScrollUpdate = true;

	for ( Uint32 i = 0; i < Texts.size(); i++ )
		AddListBoxItem( Texts[i] );

	mDisableScrollUpdate = false;

	UpdateScroll();
}

Uint32 cUIListBox::AddListBoxItem( cUIListBoxItem * Item ) {
	mItems.push_back( Item );

	if ( Item->Parent() != mContainer )
		Item->Parent( mContainer );

	if ( !mDisableScrollUpdate )
		UpdateScroll();

	Uint32 tMaxTextWidth = mMaxTextWidth;

	ItemUpdateSize( Item );

	if ( tMaxTextWidth != mMaxTextWidth ) {
		UpdateListBoxItemsSize();

		if ( !mDisableScrollUpdate )
			UpdateScroll();
	}

	return mItems.size() - 1;
}

Uint32 cUIListBox::AddListBoxItem( const std::string& Text ) {
	return AddListBoxItem( stringTowstring( Text ) );
}

Uint32 cUIListBox::AddListBoxItem( const std::wstring& Text ) {
	cUITextBox::CreateParams TextParams;
	TextParams.Parent( mContainer );
	TextParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	TextParams.Font = mFont;
	TextParams.FontColor = mFontColor;
	cUIListBoxItem * tItem = eeNew( cUIListBoxItem, ( TextParams ) );
	tItem->Text( Text );

	return AddListBoxItem( tItem );
}

Uint32 cUIListBox::RemoveListBoxItem( const std::wstring& Text ) {
	return RemoveListBoxItem( GetListBoxItemIndex( Text ) );
}

Uint32 cUIListBox::RemoveListBoxItem( cUIListBoxItem * Item ) {
	return RemoveListBoxItem( GetListBoxItemIndex( Item ) );
}

void cUIListBox::RemoveListBoxItems( std::vector<Uint32> ItemsIndex ) {
	if ( ItemsIndex.size() && 0xFFFFFFFF != ItemsIndex[0] ) {
		std::vector<cUIListBoxItem*> ItemsCpy;
		bool erase = false;

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
			} else {
				mItems[i]->Close();
			}
		}

		mItems = ItemsCpy;

		UpdateScroll();
		FindMaxWidth();
		UpdateListBoxItemsSize();
	}
}

Uint32 cUIListBox::RemoveListBoxItem( Uint32 ItemIndex ) {
	RemoveListBoxItems( std::vector<Uint32>( 1, ItemIndex ) );

	return ItemIndex;
}

Uint32 cUIListBox::GetListBoxItemIndex( const std::wstring& Name ) {
	Uint32 size = mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Name == mItems[i]->Text() )
			return i;
	}

	return 0xFFFFFFFF;
}

Uint32 cUIListBox::GetListBoxItemIndex( cUIListBoxItem * Item ) {
	Uint32 size = mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		if ( Item == mItems[i] )
			return i;
	}

	return 0xFFFFFFFF;
}

void cUIListBox::OnScrollValueChange( const cUIEvent * Event ) {
	UpdateScroll( true );
}

void cUIListBox::OnHScrollValueChange( const cUIEvent * Event ) {
	UpdateScroll( true );
}

void cUIListBox::OnSizeChange() {
	mScrollBar->Pos( mSize.Width() - mScrollBar->Size().Width(), 0 );
	mScrollBar->Size( mScrollBar->Size().Width(), mSize.Height() );

	mHScrollBar->Pos( 0, mSize.Height() - mHScrollBar->Size().Height() );
	mHScrollBar->Size( mSize.Width() - mScrollBar->Size().Width(), mHScrollBar->Size().Height() );

	if ( mContainer->IsClipped() && mAllowHorizontalScroll ) {
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

void cUIListBox::SetRowHeight() {
	Uint32 tOldRowHeight = mRowHeight;

	if ( 0 == mRowHeight ) {
		Uint32 FontSize = 12;

		if ( NULL != cUIThemeManager::instance()->DefaultFont() )
			FontSize = cUIThemeManager::instance()->DefaultFont()->GetFontSize();

		if ( NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->Theme() && NULL != mSkinState->GetSkin()->Theme()->DefaultFont() )
			FontSize = mSkinState->GetSkin()->Theme()->DefaultFont()->GetFontSize();

		if ( NULL != mFont )
			FontSize = mFont->GetFontSize();

		mRowHeight = (Uint32)( FontSize * 1.5f );
	}

	if ( tOldRowHeight != mRowHeight ) {
		UpdateScroll();
		UpdateListBoxItemsSize();
	}
}

void cUIListBox::FindMaxWidth() {
	Uint32 size = mItems.size();
	Int32 width;

	mMaxTextWidth = 0;

	for ( Uint32 i = 0; i < size; i++ ) {
		width = mItems[i]->GetTextCache().GetTextWidth();

		if ( width > (Int32)mMaxTextWidth )
			mMaxTextWidth = (Uint32)width;
	}
}

void cUIListBox::UpdateListBoxItemsSize() {
	Uint32 size = mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		ItemUpdateSize( mItems[i] );
	}
}

void cUIListBox::ItemUpdateSize( cUIListBoxItem * Item ) {
	Int32 width = Item->GetTextCache().GetTextWidth();

	if ( width > (Int32)mMaxTextWidth )
		mMaxTextWidth = (Uint32)width;

	if ( !mHScrollBar->Visible() ) {
		if ( width < mContainer->Size().Width() )
			width = mContainer->Size().Width();

		if ( ( mItemsNotVisible > 0 || mScrollAlwaysVisible ) )
			width -= mScrollBar->Size().Width();
	} else {
		width = mMaxTextWidth;
	}

	Item->Size( width, mRowHeight );
}

void cUIListBox::ContainerResize() {
	mContainer->Pos( mPaddingContainer.Left, mPaddingContainer.Top );

	if( mHScrollBar->Visible() )
		mContainer->Size( mSize.Width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.Height() - mPaddingContainer.Top - mPaddingContainer.Bottom - mHScrollBar->Size().Height() );
	else
		mContainer->Size( mSize.Width() - mPaddingContainer.Right - mPaddingContainer.Left, mSize.Height() - mPaddingContainer.Top - mPaddingContainer.Bottom );
}

void cUIListBox::UpdateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	cUIListBoxItem * Item;
	Uint32 i, RelPos = 0;
	Int32 ItemPos;

	Int32 tHLastScroll 		= mHScrollInit;

	Uint32 VisibleItems 	= mContainer->Size().Height() / mRowHeight;
	mItemsNotVisible 		= mItems.size() - VisibleItems;

	bool wasScrollVisible 	= mScrollBar->Visible();
	bool wasHScrollVisible 	= mHScrollBar->Visible();

	bool Clipped 			= mContainer->IsClipped();

	if ( mItemsNotVisible <= 0 ) {
		if ( mScrollAlwaysVisible ) {
			mScrollBar->Visible( true );
			mScrollBar->Enabled( true );
		} else {
			mScrollBar->Visible( false );
			mScrollBar->Enabled( false );
		}
	} else {
		mScrollBar->Visible( true );
		mScrollBar->Enabled( true );
	}

	if ( Clipped && mAllowHorizontalScroll ) {
		if ( ( mScrollBar->Visible() && mContainer->Size().Width() - mScrollBar->Size().Width() < (Int32)mMaxTextWidth ) ||
			( !mScrollBar->Visible() && mContainer->Size().Width() < (Int32)mMaxTextWidth ) ) {
				mHScrollBar->Visible( true );
				mHScrollBar->Enabled( true );

				ContainerResize();

				Int32 ScrollH;

				if ( mScrollBar->Visible() )
					ScrollH = mMaxTextWidth - mContainer->Size().Width() + mScrollBar->Size().Width();
				else
					ScrollH = mMaxTextWidth - mContainer->Size().Width();

				Int32 HScrolleable = (Uint32)( mHScrollBar->Value() * ScrollH );

				mHScrollInit = -HScrolleable;
		} else {
			mHScrollBar->Visible( false );
			mHScrollBar->Enabled( false );

			mHScrollInit = 0;

			ContainerResize();
		}
	}

	VisibleItems 		= mContainer->Size().Height() / mRowHeight;
	mItemsNotVisible 	= mItems.size() - VisibleItems;
	Uint32 Scrolleable 	= mItems.size() * mRowHeight - mContainer->Size().Height();
	bool isScrollVisible = mScrollBar->Visible();
	bool isHScrollVisible = mHScrollBar->Visible();

	if ( Clipped && mSoftScroll ) {
		RelPos = (Uint32)( mScrollBar->Value() * Scrolleable );

		if ( ( FromScrollChange && 0xFFFFFFFF != mLastPos && mLastPos == RelPos ) && ( tHLastScroll == mHScrollInit ) )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * i;

			if ( ItemPos >= (Int32)RelPos || ItemPos + mRowHeight >= RelPos ) {
				Item->Pos( mHScrollInit, ItemPos - RelPos );
				Item->Enabled( true );
				Item->Visible( true );
			} else {
				Item->Enabled( false );
				Item->Visible( false );
			}

			if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
				ItemUpdateSize( Item );
		}
	} else {
		Uint32 RelPosMax		= mItems.size();

		if ( mItemsNotVisible > 0 ) {
			RelPos 				= (Uint32)( mScrollBar->Value() * mItemsNotVisible );
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
			} else {
				Item->Enabled( false );
				Item->Visible( false );
			}

			if ( ( !wasScrollVisible && isScrollVisible ) || ( wasScrollVisible && !isScrollVisible ) ||( !wasHScrollVisible && isHScrollVisible ) || ( wasHScrollVisible && !isHScrollVisible ) )
				ItemUpdateSize( Item );
		}
	}
}

void cUIListBox::ItemClicked( cUIListBoxItem * Item ) {
	if ( !( IsMultiSelect() && cUIManager::instance()->GetInput()->IsKeyDown( KEY_LCTRL ) ) )
		ResetItemsStates();
}

Uint32 cUIListBox::OnSelected() {
	SendCommonEvent( cUIEvent::EventOnSelected );

	return 1;
}

void cUIListBox::ResetItemsStates() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->Unselect();
	}
}

bool cUIListBox::IsMultiSelect() const {
	return 0 != ( mFlags & UI_MULTI_SELECT );
}

cUIListBoxItem * cUIListBox::GetItem( const Uint32& Index ) const {
	eeASSERT( Index < mItems.size() )

	return mItems[ Index ];
}

cUIListBoxItem * cUIListBox::GetItemSelected() {
	if ( mSelected.size() )
		return mItems[ mSelected.front() ];

	return NULL;
}

Uint32 cUIListBox::GetItemSelectedIndex() const {
	if ( mSelected.size() )
		return mSelected.front();

	return 0xFFFFFFFF;
}

std::list<Uint32> cUIListBox::GetItemsSelectedIndex() const {
	return mSelected;
}

std::list<cUIListBoxItem *> cUIListBox::GetItemsSelected() {
	std::list<cUIListBoxItem *> tItems;
	std::list<Uint32>::iterator it;

	for ( it = mSelected.begin(); it != mSelected.end(); it++ )
		tItems.push_back( mItems[ *it ] );

	return tItems;
}

Uint32 cUIListBox::GetItemIndex( cUIListBoxItem * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Item )
			return i;
	}

	return 0xFFFFFFFF;
}

void cUIListBox::FontColor( const eeColorA& Color ) {
	mFontColor = Color;

	for ( Uint32 i = 0; i < mItems.size(); i++ )
		mItems[i]->Color( mFontColor );
}

const eeColorA& cUIListBox::FontColor() const {
	return mFontColor;
}

void cUIListBox::FontOverColor( const eeColorA& Color ) {
	mFontOverColor = Color;
}

const eeColorA& cUIListBox::FontOverColor() const {
	return mFontOverColor;
}

void cUIListBox::FontSelectedColor( const eeColorA& Color ) {
	mFontSelectedColor = Color;
}

const eeColorA& cUIListBox::FontSelectedColor() const {
	return mFontSelectedColor;
}

void cUIListBox::Font( cFont * Font ) {
	mFont = Font;

	for ( Uint32 i = 0; i < mItems.size(); i++ )
		mItems[i]->Font( mFont );

	FindMaxWidth();
	UpdateListBoxItemsSize();
	UpdateScroll();
}

cFont * cUIListBox::Font() const {
	return mFont;
}

void cUIListBox::PaddingContainer( const eeRecti& Padding ) {
	if ( Padding != mPaddingContainer ) {
		mPaddingContainer = Padding;

		ContainerResize();
		UpdateScroll();
	}
}

const eeRecti& cUIListBox::PaddingContainer() const {
	return mPaddingContainer;
}

void cUIListBox::SoftScroll( const bool& soft ) {
	if ( soft != mSoftScroll ) {
		mSoftScroll = soft;

		UpdateScroll();
	}
}

const bool& cUIListBox::SoftScroll() const {
	return mSoftScroll;
}

void cUIListBox::ScrollAlwaysVisible( const bool& visible ) {
	if ( visible != mScrollAlwaysVisible ) {
		mScrollAlwaysVisible = visible;

		UpdateScroll();
	}
}

const bool& cUIListBox::ScrollAlwaysVisible() const {
	return mScrollAlwaysVisible;
}

void cUIListBox::RowHeight( const Uint32& height ) {
	if ( mRowHeight != height ) {
		mRowHeight = height;

		UpdateListBoxItemsSize();
		UpdateScroll();
	}
}

const Uint32& cUIListBox::RowHeight() const {
	return mRowHeight;
}


void cUIListBox::AllowHorizontalScroll( const bool& allow ) {
	if ( allow != mAllowHorizontalScroll ) {
		mAllowHorizontalScroll = allow;

		UpdateScroll();
	}
}

const bool& cUIListBox::AllowHorizontalScroll() const {
	return mAllowHorizontalScroll;
}

Uint32 cUIListBox::Size() {
	return mItems.size();
}

}}
