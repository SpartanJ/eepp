#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwinmenu.hpp>

namespace EE { namespace UI {

UISelectButton *UISelectButton::New() {
	return eeNew( UISelectButton, () );
}

UISelectButton::UISelectButton( const UIPushButton::CreateParams& Params ) :
	UIPushButton( Params )
{
}

UISelectButton::UISelectButton() :
	UIPushButton()
{
}

UISelectButton::~UISelectButton() {
}

Uint32 UISelectButton::getType() const {
	return UI_TYPE_SELECTBUTTON;
}

bool UISelectButton::isType( const Uint32& type ) const {
	return UISelectButton::getType() == type ? true : UIPushButton::isType( type );
}

void UISelectButton::select() {
	bool wasSelected = selected();

	setSkinState( UISkinState::StateSelected );

	mControlFlags |= UI_CTRL_FLAG_SELECTED;

	if ( !wasSelected ) {
		UIMessage tMsg( this, UIMessage::MsgSelected, 0 );
		messagePost( &tMsg );
	}
}

void UISelectButton::unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	setSkinState( UISkinState::StateNormal );
}

bool UISelectButton::selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

void UISelectButton::onStateChange() {
	if ( mSkinState->getState() != UISkinState::StateSelected && selected() ) {
		if ( mSkinState->stateExists( UISkinState::StateSelected ) ) {
			setSkinState( UISkinState::StateSelected );
		}
	}

	if ( getParent()->getType() & UI_TYPE_WINMENU ) {
		UIWinMenu * Menu = reinterpret_cast<UIWinMenu*> ( getParent() );

		if ( mSkinState->getState() == UISkinState::StateSelected ) {
			getTextBox()->setFontColor( Menu->getFontStyleConfig().getFontSelectedColor() );
		} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
			getTextBox()->setFontColor( Menu->getFontStyleConfig().getFontOverColor() );
		} else {
			getTextBox()->setFontColor( Menu->getFontStyleConfig().getFontColor() );
		}
	} else {
		if ( mSkinState->getState() == UISkinState::StateSelected ) {
			getTextBox()->setFontColor( mFontStyleConfig.fontSelectedColor );
		} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
			getTextBox()->setFontColor( mFontStyleConfig.fontOverColor );
		} else {
			getTextBox()->setFontColor( mFontStyleConfig.fontColor );
		}
	}
}

void UISelectButton::setFontSelectedColor(const ColorA & color) {
	mFontStyleConfig.fontSelectedColor = color;
}

const ColorA &UISelectButton::getFontSelectedColor() const {
	return mFontStyleConfig.fontSelectedColor;
}

}}
