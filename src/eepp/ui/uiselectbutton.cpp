#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UISelectButton * UISelectButton::New() {
	return eeNew( UISelectButton, () );
}

UISelectButton::UISelectButton( const std::string& tag ) :
	UIPushButton( tag )
{}

UISelectButton::UISelectButton() :
	UISelectButton( "selectbutton" )
{}

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
	UIWidget::onStateChange();

	if ( NULL == mSkinState )
		return;

	if ( !( mSkinState->getState() & UIState::StateFlagSelected ) && selected() ) {
		pushState( UIState::StateSelected, false );
	}

	mTextBox->setAlpha( mAlpha );
}

void UISelectButton::setFontSelectedColor(const Color & color) {
	mStyleConfig.FontSelectedColor = color;
}

const Color &UISelectButton::getFontSelectedColor() const {
	return mStyleConfig.FontSelectedColor;
}

bool UISelectButton::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "textselectedcolor" == name ) {
		setFontSelectedColor( attribute.asColor() );
	} else {
		return UIPushButton::setAttribute( attribute, state );
	}

	return true;
}

}}
