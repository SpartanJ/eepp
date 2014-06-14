#include <eepp/ui/cuilistboxitem.hpp>
#include <eepp/ui/cuilistbox.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace UI {

cUIListBoxItem::cUIListBoxItem( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params )
{
	ApplyDefaultTheme();
}

cUIListBoxItem::~cUIListBoxItem() {
	if ( cUIManager::instance()->FocusControl() == this )
		mParentCtrl->SetFocus();

	if ( cUIManager::instance()->OverControl() == this )
		cUIManager::instance()->OverControl( mParentCtrl );
}

Uint32 cUIListBoxItem::Type() const {
	return UI_TYPE_LISTBOXITEM;
}

bool cUIListBoxItem::IsType( const Uint32& type ) const {
	return cUIListBoxItem::Type() == type ? true : cUITextBox::IsType( type );
}

void cUIListBoxItem::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "listboxitem" );
}

Uint32 cUIListBoxItem::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTONS_LRM ) {
		reinterpret_cast<cUIListBox*> ( Parent()->Parent() )->ItemClicked( this );

		Select();
	}

	return 1;
}

void cUIListBoxItem::Select() {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent()->Parent() );

	bool wasSelected = 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );

	if ( LBParent->IsMultiSelect() ) {
		if ( !wasSelected ) {
			SetSkinState( cUISkinState::StateSelected );

			mControlFlags |= UI_CTRL_FLAG_SELECTED;

			LBParent->mSelected.push_back( LBParent->GetItemIndex( this ) );

			LBParent->OnSelected();
		} else {
			mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

			LBParent->mSelected.remove( LBParent->GetItemIndex( this ) );
		}
	} else {
		SetSkinState( cUISkinState::StateSelected );

		mControlFlags |= UI_CTRL_FLAG_SELECTED;

		LBParent->mSelected.clear();
		LBParent->mSelected.push_back( LBParent->GetItemIndex( this ) );

		if ( !wasSelected ) {
			LBParent->OnSelected();
		}
	}
}

void cUIListBoxItem::Update() {
	cUITextBox::Update();

	if ( mEnabled && mVisible ) {
		cUIListBox * LBParent 	= reinterpret_cast<cUIListBox*> ( Parent()->Parent() );
		Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

		if ( IsMouseOver() ) {
			if ( Flags & EE_BUTTONS_WUWD && LBParent->VerticalScrollBar()->Visible() ) {
				LBParent->VerticalScrollBar()->Slider()->ManageClick( Flags );
			}
		}
	}
}

Uint32 cUIListBoxItem::OnMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	cUIControl::OnMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		SetSkinState( cUISkinState::StateSelected );

	return 1;
}

void cUIListBoxItem::Unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	SetSkinState( cUISkinState::StateNormal );
}

bool cUIListBoxItem::Selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

void cUIListBoxItem::OnStateChange() {
	cUIListBox * LBParent = reinterpret_cast<cUIListBox*> ( Parent()->Parent() );

	if ( Selected() && mSkinState->GetState() != cUISkinState::StateSelected ) {
		SetSkinState( cUISkinState::StateSelected );
	}

	if ( mSkinState->GetState() == cUISkinState::StateSelected ) {
		Color( LBParent->FontSelectedColor() );
	} else if ( mSkinState->GetState() == cUISkinState::StateMouseEnter ) {
		Color( LBParent->FontOverColor() );
	} else {
		Color( LBParent->FontColor() );
	}
}

}}
