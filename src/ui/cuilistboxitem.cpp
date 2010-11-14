#include "cuilistboxitem.hpp"
#include "cuilistbox.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIListBoxItem::cUIListBoxItem( cUITextBox::CreateParams& Params ) :
	cUITextBox( Params ),
	mSelected( false )
{
	mType |= UI_TYPE_LISTBOXITEM;
}

cUIListBoxItem::~cUIListBoxItem() {
}


void cUIListBoxItem::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "listboxitem" );
}

Uint32 cUIListBoxItem::OnMouseClick( const eeVector2i& Pos, const Uint32 Flags ) {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent()->Parent() );

	if ( Flags & EE_BUTTONS_LRM ) {
		bool wasSelected = mSelected;

		LBParent->ItemClicked( this );

		if ( LBParent->IsMultiSelect() ) {
			if ( !wasSelected ) {
				SetSkinState( cUISkinState::StateSelected );

				mSelected = true;

				LBParent->mSelected.push_back( LBParent->GetItemIndex( this ) );

				LBParent->OnSelected();
			} else {
				mSelected = false;

				LBParent->mSelected.remove( LBParent->GetItemIndex( this ) );
			}
		} else {
			SetSkinState( cUISkinState::StateSelected );

			mSelected = true;

			LBParent->mSelected.clear();
			LBParent->mSelected.push_back( LBParent->GetItemIndex( this ) );
			LBParent->OnSelected();
		}
	}

	return 1;
}

void cUIListBoxItem::Update() {
	if ( mEnabled && mVisible && IsMouseOver() ) {
		Uint32 Flags = cUIManager::instance()->GetInput()->ClickTrigger();

		if ( Flags & EE_BUTTONS_WUWD ) {
			cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent()->Parent() );

			LBParent->ScrollBar()->Slider()->ManageClick( Flags );
		}
	}
}

Uint32 cUIListBoxItem::OnMouseExit( const eeVector2i& Pos, const Uint32 Flags ) {
	cUITextBox::OnMouseExit( Pos, Flags );

	if ( mSelected )
		SetSkinState( cUISkinState::StateSelected );

	return 1;
}

void cUIListBoxItem::Unselect() {
	mSelected = false;

	SetSkinState( cUISkinState::StateNormal );
}

const bool& cUIListBoxItem::Selected() const {
	return mSelected;
}

void cUIListBoxItem::OnStateChange() {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent()->Parent() );

	if ( mSkinState->GetState() == cUISkinState::StateSelected ) {
		Color( LBParent->FontSelectedColor() );
	} else if ( mSkinState->GetState() == cUISkinState::StateMouseEnter ) {
		Color( LBParent->FontOverColor() );
	} else {
		Color( LBParent->FontColor() );
	}
}

}}
