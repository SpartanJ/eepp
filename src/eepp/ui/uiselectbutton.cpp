#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UISelectButton *UISelectButton::New() {
	return eeNew( UISelectButton, () );
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

	pushState( UIState::StateSelected );

	mNodeFlags |= NODE_FLAG_SELECTED;

	if ( !wasSelected ) {
		NodeMessage tMsg( this, NodeMessage::Selected, 0 );
		messagePost( &tMsg );
	}
}

void UISelectButton::unselect() {
	if ( mNodeFlags & NODE_FLAG_SELECTED )
		mNodeFlags &= ~NODE_FLAG_SELECTED;

	popState( UIState::StateSelected );
}

bool UISelectButton::selected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

void UISelectButton::onStateChange() {
	if ( NULL == mSkinState )
		return;

	if ( !( mSkinState->getState() & UIState::StateFlagSelected ) && selected() && mSkinState->stateExists( UIState::StateSelected ) ) {
		pushState( UIState::StateSelected, false );
	}

	if ( getParent()->isType( UI_TYPE_WINMENU ) ) {
		UIWinMenu * Menu = reinterpret_cast<UIWinMenu*> ( getParent() );

		if ( mSkinState->getState() & UIState::StateFlagSelected ) {
			getTextBox()->setFontColor( Menu->getStyleConfig().getFontSelectedColor() );
		} else if ( mSkinState->getState() & UIState::StateFlagHover ) {
			getTextBox()->setFontColor( Menu->getStyleConfig().getFontOverColor() );
		} else {
			getTextBox()->setFontColor( Menu->getStyleConfig().getFontColor() );
		}
	} else {
		if ( mSkinState->getState() & UIState::StateFlagSelected ) {
			getTextBox()->setFontColor( mStyleConfig.FontSelectedColor );
		} else if ( mSkinState->getState() & UIState::StateFlagHover ) {
			getTextBox()->setFontColor( mStyleConfig.FontOverColor );
		} else {
			getTextBox()->setFontColor( mStyleConfig.FontColor );
		}
	}

	mTextBox->setAlpha( mAlpha );

	UIWidget::onStateChange();
}

void UISelectButton::setFontSelectedColor(const Color & color) {
	mStyleConfig.FontSelectedColor = color;
}

const Color &UISelectButton::getFontSelectedColor() const {
	return mStyleConfig.FontSelectedColor;
}

bool UISelectButton::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "textselectedcolor" == name ) {
		setFontSelectedColor( Color::fromString( attribute.asString() ) );
	} else {
		return UIPushButton::setAttribute( attribute );
	}

	return true;
}

}}
