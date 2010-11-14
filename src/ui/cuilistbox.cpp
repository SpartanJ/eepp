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
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontOverColor( Params.FontOverColor ),
	mFontSelectedColor( Params.FontSelectedColor ),
	mLastPos( 0xFFFFFFFF ),
	mDisableScrollUpdate( false )
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

	if ( mScrollAlwaysVisible ) {
		mScrollBar->Visible( true );
		mScrollBar->Enabled( true );
	}

	mScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIListBox::OnScrollValueChange ) );

	SetRowHeight();
}

cUIListBox::~cUIListBox() {
}

void cUIListBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "listbox" );

	OnSizeChange();
}

cUIScrollBar * cUIListBox::ScrollBar() const {
	return mScrollBar;
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

	Item->Size( mSize.Width(), mRowHeight );
	Item->Visible( false );
	Item->Enabled( false );

	if ( !mDisableScrollUpdate )
		UpdateScroll();

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

void cUIListBox::OnSizeChange() {
	mScrollBar->Pos( mSize.Width() - mScrollBar->Size().Width(), 0 );
	mScrollBar->Size( mScrollBar->Size().Width(), mSize.Height() );

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
		UpdateListBoxItemsSize();
		UpdateScroll();
	}
}

void cUIListBox::UpdateListBoxItemsSize() {
	Uint32 size = mItems.size();

	for ( Uint32 i = 0; i < size; i++ ) {
		mItems[i]->Size( mContainer->Size().Width(), mRowHeight );
	}
}

void cUIListBox::UpdateScroll( bool FromScrollChange ) {
	if ( !mItems.size() )
		return;

	cUIListBoxItem * Item;
	Uint32 i, RelPos = 0;
	Int32 ItemPos;
	Uint32 Scrolleable = mItems.size() * mRowHeight - mContainer->Size().Height();

	Uint32 VisibleItems 	= mContainer->Size().Height() / mRowHeight;
	Int32 ItemsNotVisible 	= mItems.size() - VisibleItems;

	if ( mContainer->IsClipped() && mSoftScroll ) {
		RelPos = (Uint32)( mScrollBar->Value() * Scrolleable );

		if ( FromScrollChange && 0xFFFFFFFF != mLastPos && mLastPos == RelPos )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * i;

			if ( ItemPos >= (Int32)RelPos || ItemPos + mRowHeight >= RelPos ) {
				Item->Pos( 0, ItemPos - RelPos );
				Item->Enabled( true );
				Item->Visible( true );
			} else {
				Item->Enabled( false );
				Item->Visible( false );
			}
		}
	} else {
		Uint32 RelPosMax		= mItems.size();

		if ( ItemsNotVisible > 0 ) {
			RelPos 				= (Uint32)( mScrollBar->Value() * ItemsNotVisible );
			RelPosMax			= RelPos + VisibleItems;
		}

		if ( FromScrollChange && 0xFFFFFFFF != mLastPos && mLastPos == RelPos )
			return;

		mLastPos = RelPos;

		for ( i = 0; i < mItems.size(); i++ ) {
			Item = mItems[i];
			ItemPos = mRowHeight * ( i - RelPos );

			if ( i >= RelPos && i < RelPosMax ) {
				Item->Pos( 0, ItemPos );

				Item->Enabled( true );
				Item->Visible( true );
			} else {
				Item->Enabled( false );
				Item->Visible( false );
			}
		}
	}

	if ( ItemsNotVisible <= 0 ) {
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
}

void cUIListBox::ItemClicked( cUIListBoxItem * Item ) {
	if ( !( IsMultiSelect() && cUIManager::instance()->GetInput()->IsKeyDown( KEY_LCTRL ) ) )
		ResetItemsStates();
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

}}
