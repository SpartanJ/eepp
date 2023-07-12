#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uistyle.hpp>

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

		sendCommonEvent( Event::OnSelectionChanged );
		onValueChange();
	}
}

void UISelectButton::toggleSelection() {
	setSelected( !isSelected() );
}

void UISelectButton::unselect() {
	if ( mNodeFlags & NODE_FLAG_SELECTED ) {
		mNodeFlags &= ~NODE_FLAG_SELECTED;

		sendCommonEvent( Event::OnSelectionChanged );
		onValueChange();
	}

	popState( UIState::StateSelected );
}

bool UISelectButton::isSelected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

void UISelectButton::onStateChange() {
	UIWidget::onStateChange();

	mTextBox->setAlpha( mAlpha );

	if ( NULL == mSkinState )
		return;

	if ( !( mSkinState->getState() & UIState::StateFlagSelected ) && isSelected() ) {
		pushState( UIState::StateSelected, false );
	}
}

bool UISelectButton::applyProperty( const StyleSheetProperty& attribute ) {
	bool attributeSet = true;

	if ( attribute.getPropertyDefinition() == NULL ) {
		return false;
	}

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::SelectOnClick:
			setSelectOnClick( attribute.asBool() );
			break;
		case PropertyId::Selected:
		case PropertyId::Value:
			setSelected( attribute.asBool() );
			break;
		default:
			attributeSet = UIPushButton::applyProperty( attribute );
			break;
	}

	return attributeSet;
}

std::string UISelectButton::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::SelectOnClick:
			return hasSelectOnClick() ? "true" : "false";
		case PropertyId::Selected:
		case PropertyId::Value:
			return isSelected() ? "true" : "false";
		default:
			return UIPushButton::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UISelectButton::getPropertiesImplemented() const {
	auto props = UIPushButton::getPropertiesImplemented();
	auto local = { PropertyId::SelectOnClick, PropertyId::Selected };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UISelectButton::setSelected( bool set ) {
	if ( set ) {
		select();
	} else {
		unselect();
	}
}

void UISelectButton::setSelectOnClick( bool set ) {
	if ( set ) {
		if ( mSelectOnClickCbId == 0 )
			mSelectOnClickCbId = addEventListener( Event::MouseClick,
												   [this]( const Event* ) { toggleSelection(); } );
	} else {
		if ( mSelectOnClickCbId != 0 )
			removeEventListener( mSelectOnClickCbId );
	}
}

bool UISelectButton::hasSelectOnClick() const {
	return 0 != mSelectOnClickCbId;
}

}} // namespace EE::UI
