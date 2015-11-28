#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwinmenu.hpp>

namespace EE { namespace UI {

UISelectButton::UISelectButton( const UIPushButton::CreateParams& Params ) :
	UIPushButton( Params )
{
}

UISelectButton::~UISelectButton() {
}

Uint32 UISelectButton::Type() const {
	return UI_TYPE_SELECTBUTTON;
}

bool UISelectButton::IsType( const Uint32& type ) const {
	return UISelectButton::Type() == type ? true : UIPushButton::IsType( type );
}

void UISelectButton::Select() {
	bool wasSelected = Selected();

	SetSkinState( UISkinState::StateSelected );

	mControlFlags |= UI_CTRL_FLAG_SELECTED;

	if ( !wasSelected ) {
		UIMessage tMsg( this, UIMessage::MsgSelected, 0 );
		MessagePost( &tMsg );
	}
}

void UISelectButton::Unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	SetSkinState( UISkinState::StateNormal );
}

bool UISelectButton::Selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

void UISelectButton::OnStateChange() {
	if ( mSkinState->GetState() != UISkinState::StateSelected && Selected() ) {
		if ( mSkinState->StateExists( UISkinState::StateSelected ) ) {
			SetSkinState( UISkinState::StateSelected );
		}
	}

	if ( Parent()->Type() & UI_TYPE_WINMENU ) {
		UIWinMenu * Menu = reinterpret_cast<UIWinMenu*> ( Parent() );

		if ( mSkinState->GetState() == UISkinState::StateSelected ) {
			TextBox()->Color( Menu->FontSelectedColor() );
		} else if ( mSkinState->GetState() == UISkinState::StateMouseEnter ) {
			TextBox()->Color( Menu->FontOverColor() );
		} else {
			TextBox()->Color( Menu->FontColor() );
		}
	}
}

}}
