#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>

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

	if ( getParent()->isType( UI_TYPE_WINMENU ) ) {
		UIWinMenu * Menu = reinterpret_cast<UIWinMenu*> ( getParent() );

		if ( mSkinState->getState() == UISkinState::StateSelected ) {
			getTextBox()->setFontColor( Menu->getStyleConfig().getFontSelectedColor() );
		} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
			getTextBox()->setFontColor( Menu->getStyleConfig().getFontOverColor() );
		} else {
			getTextBox()->setFontColor( Menu->getStyleConfig().getFontColor() );
		}
	} else {
		if ( mSkinState->getState() == UISkinState::StateSelected ) {
			getTextBox()->setFontColor( mStyleConfig.FontSelectedColor );
		} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
			getTextBox()->setFontColor( mStyleConfig.FontOverColor );
		} else {
			getTextBox()->setFontColor( mStyleConfig.FontColor );
		}
	}
}

void UISelectButton::setFontSelectedColor(const Color & color) {
	mStyleConfig.FontSelectedColor = color;
}

const Color &UISelectButton::getFontSelectedColor() const {
	return mStyleConfig.FontSelectedColor;
}

void UISelectButton::loadFromXmlNode(const pugi::xml_node & node) {
	UIPushButton::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "textselectedcolor" == name ) {
			setFontSelectedColor( Color::fromString( ait->as_string() ) );
		}
	}
}

}}
