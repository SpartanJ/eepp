#include <eepp/ui/cuiselectbutton.hpp>
#include <eepp/ui/cuiwinmenu.hpp>

namespace EE { namespace UI {

cUISelectButton::cUISelectButton( const cUIPushButton::CreateParams& Params ) :
	cUIPushButton( Params )
{
}

cUISelectButton::~cUISelectButton() {
}

Uint32 cUISelectButton::Type() const {
	return UI_TYPE_SELECTBUTTON;
}

bool cUISelectButton::IsType( const Uint32& type ) const {
	return cUISelectButton::Type() == type ? true : cUIPushButton::IsType( type );
}

void cUISelectButton::Select() {
	bool wasSelected = Selected();

	SetSkinState( cUISkinState::StateSelected );

	mControlFlags |= UI_CTRL_FLAG_SELECTED;

	if ( !wasSelected ) {
		cUIMessage tMsg( this, cUIMessage::MsgSelected, 0 );
		MessagePost( &tMsg );
	}
}

void cUISelectButton::Unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	SetSkinState( cUISkinState::StateNormal );
}

bool cUISelectButton::Selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

void cUISelectButton::OnStateChange() {
	if ( mSkinState->GetState() != cUISkinState::StateSelected && Selected() ) {
		if ( mSkinState->StateExists( cUISkinState::StateSelected ) ) {
			SetSkinState( cUISkinState::StateSelected );
		}
	}

	if ( Parent()->Type() & UI_TYPE_WINMENU ) {
		cUIWinMenu * Menu = reinterpret_cast<cUIWinMenu*> ( Parent() );

		if ( mSkinState->GetState() == cUISkinState::StateSelected ) {
			TextBox()->Color( Menu->FontSelectedColor() );
		} else if ( mSkinState->GetState() == cUISkinState::StateMouseEnter ) {
			TextBox()->Color( Menu->FontOverColor() );
		} else {
			TextBox()->Color( Menu->FontColor() );
		}
	}
}

}}
