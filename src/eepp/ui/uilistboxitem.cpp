#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIListBoxItem::UIListBoxItem( const UITextBox::CreateParams& Params ) :
	UITextBox( Params )
{
	ApplyDefaultTheme();
}

UIListBoxItem::~UIListBoxItem() {
	if ( UIManager::instance()->FocusControl() == this )
		mParentCtrl->SetFocus();

	if ( UIManager::instance()->OverControl() == this )
		UIManager::instance()->OverControl( mParentCtrl );
}

Uint32 UIListBoxItem::Type() const {
	return UI_TYPE_LISTBOXITEM;
}

bool UIListBoxItem::IsType( const Uint32& type ) const {
	return UIListBoxItem::Type() == type ? true : UITextBox::IsType( type );
}

void UIListBoxItem::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "listboxitem" );
}

Uint32 UIListBoxItem::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTONS_LRM ) {
		reinterpret_cast<UIListBox*> ( Parent()->Parent() )->ItemClicked( this );

		Select();
	}

	return 1;
}

void UIListBoxItem::Select() {
	UIListBox * LBParent = reinterpret_cast<UIListBox*> ( Parent()->Parent() );

	bool wasSelected = 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );

	if ( LBParent->IsMultiSelect() ) {
		if ( !wasSelected ) {
			SetSkinState( UISkinState::StateSelected );

			mControlFlags |= UI_CTRL_FLAG_SELECTED;

			LBParent->mSelected.push_back( LBParent->GetItemIndex( this ) );

			LBParent->OnSelected();
		} else {
			mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

			LBParent->mSelected.remove( LBParent->GetItemIndex( this ) );
		}
	} else {
		SetSkinState( UISkinState::StateSelected );

		mControlFlags |= UI_CTRL_FLAG_SELECTED;

		LBParent->mSelected.clear();
		LBParent->mSelected.push_back( LBParent->GetItemIndex( this ) );

		if ( !wasSelected ) {
			LBParent->OnSelected();
		}
	}
}

void UIListBoxItem::Update() {
	UITextBox::Update();

	if ( mEnabled && mVisible ) {
		UIListBox * LBParent 	= reinterpret_cast<UIListBox*> ( Parent()->Parent() );
		Uint32 Flags 			= UIManager::instance()->GetInput()->clickTrigger();

		if ( IsMouseOver() ) {
			if ( Flags & EE_BUTTONS_WUWD && LBParent->VerticalScrollBar()->Visible() ) {
				LBParent->VerticalScrollBar()->Slider()->ManageClick( Flags );
			}
		}
	}
}

Uint32 UIListBoxItem::OnMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::OnMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		SetSkinState( UISkinState::StateSelected );

	return 1;
}

void UIListBoxItem::Unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	SetSkinState( UISkinState::StateNormal );
}

bool UIListBoxItem::Selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

void UIListBoxItem::OnStateChange() {
	UIListBox * LBParent = reinterpret_cast<UIListBox*> ( Parent()->Parent() );

	if ( Selected() && mSkinState->GetState() != UISkinState::StateSelected ) {
		SetSkinState( UISkinState::StateSelected );
	}

	if ( mSkinState->GetState() == UISkinState::StateSelected ) {
		Color( LBParent->FontSelectedColor() );
	} else if ( mSkinState->GetState() == UISkinState::StateMouseEnter ) {
		Color( LBParent->FontOverColor() );
	} else {
		Color( LBParent->FontColor() );
	}
}

}}
