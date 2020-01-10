#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwinmenu.hpp>

namespace EE { namespace UI {

UISelectButton* UISelectButton::New() {
	return eeNew( UISelectButton, () );
}

UISelectButton* UISelectButton::NewWithTag( const std::string& tag ) {
	return eeNew( UISelectButton, ( tag ) );
}

UISelectButton::UISelectButton( const std::string& tag ) : UIPushButton( tag ) {}

UISelectButton::UISelectButton() : UISelectButton( "selectbutton" ) {}

UISelectButton::~UISelectButton() {}

Uint32 UISelectButton::getType() const {
	return UI_TYPE_SELECTBUTTON;
}

bool UISelectButton::isType( const Uint32& type ) const {
	return UISelectButton::getType() == type ? true : UIPushButton::isType( type );
}

void UISelectButton::select() {
	bool wasSelected = isSelected();

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

bool UISelectButton::isSelected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

void UISelectButton::onStateChange() {
	UIWidget::onStateChange();

	if ( NULL == mSkinState )
		return;

	if ( !( mSkinState->getState() & UIState::StateFlagSelected ) && isSelected() ) {
		pushState( UIState::StateSelected, false );
	}

	mTextBox->setAlpha( mAlpha );
}

}} // namespace EE::UI
