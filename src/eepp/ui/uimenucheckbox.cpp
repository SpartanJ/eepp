#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

UIMenuCheckBox* UIMenuCheckBox::New() {
	return eeNew( UIMenuCheckBox, () );
}

UIMenuCheckBox::UIMenuCheckBox() :
	UIMenuItem( "menu::checkbox" ), mActive( false ), mSkinActive( NULL ), mSkinInactive( NULL ) {
	mTextBox->setElementTag( mTag + "::text" );
	applyDefaultTheme();
	mIcon->setFlags( UI_SKIN_KEEP_SIZE_ON_DRAW );
}

UIMenuCheckBox::~UIMenuCheckBox() {}

Uint32 UIMenuCheckBox::getType() const {
	return UI_TYPE_MENUCHECKBOX;
}

bool UIMenuCheckBox::isType( const Uint32& type ) const {
	return UIMenuCheckBox::getType() == type ? true : UIMenuItem::isType( type );
}

void UIMenuCheckBox::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "menuitem" );

	mSkinActive = Theme->getSkin( "menucheckbox_active" );
	mSkinInactive = Theme->getSkin( "menucheckbox_inactive" );

	setActive( mActive );

	onThemeLoaded();
}

const bool& UIMenuCheckBox::isActive() const {
	return mActive;
}

UIMenuCheckBox* UIMenuCheckBox::setActive( const bool& active ) {
	bool oActive = mActive;
	mActive = active;

	if ( mActive ) {
		mIcon->pushState( UIState::StateSelected );
	} else {
		mIcon->popState( UIState::StateSelected );
	}

	if ( mActive ) {
		if ( NULL != mSkinActive ) {
			if ( NULL == mIcon->getSkin() || mIcon->getSkin()->getName() != mSkinActive->getName() )
				mIcon->setSkin( mSkinActive );

			if ( NULL != mSkinState ) {
				if ( mSkinState->getState() & UIState::StateFlagSelected ) {
					mIcon->pushState( UIState::StateHover );
				} else {
					mIcon->popState( UIState::StateHover );
				}
			}
		} else {
			mIcon->removeSkin();
		}
	} else {
		if ( NULL != mSkinInactive ) {
			if ( NULL == mIcon->getSkin() ||
				 mIcon->getSkin()->getName() != mSkinInactive->getName() )
				mIcon->setSkin( mSkinInactive );

			if ( NULL != mSkinState ) {
				if ( mSkinState->getState() & UIState::StateFlagSelected ) {
					mIcon->pushState( UIState::StateHover );
				} else {
					mIcon->popState( UIState::StateHover );
				}
			}
		} else {
			mIcon->removeSkin();
		}
	}

	if ( oActive != active ) {
		if ( getParent()->isType( UI_TYPE_MENU ) ) {
			UIMenu* menu = getParent()->asType<UIMenu>();

			if ( !menu->widgetCheckSize( this ) ) {
				if ( NULL != getIcon()->getDrawable() ) {
					setPadding( Rectf( 0, 0, 0, 0 ) );
				}
			}
		}

		onValueChange();
	}

	return this;
}

void UIMenuCheckBox::switchActive() {
	setActive( !mActive );
}

Uint32 UIMenuCheckBox::onMessage( const NodeMessage* msg ) {
	switch ( msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			if ( msg->getFlags() & EE_BUTTON_LMASK )
				switchActive();
			break;
		}
	}
	return 0;
}

void UIMenuCheckBox::onStateChange() {
	UIMenuItem::onStateChange();

	setActive( mActive );
}

bool UIMenuCheckBox::applyProperty( const StyleSheetProperty& attribute ) {
	bool attributeSet = true;

	if ( attribute.getPropertyDefinition() == NULL ) {
		return false;
	}

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Checked:
		case PropertyId::Value:
			setActive( attribute.asBool() );
			break;
		default:
			attributeSet = UIPushButton::applyProperty( attribute );
			break;
	}

	return attributeSet;
}

std::string UIMenuCheckBox::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Checked:
		case PropertyId::Value:
			return isActive() ? "true" : "false";
		default:
			return UIPushButton::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIMenuCheckBox::getPropertiesImplemented() const {
	auto props = UIPushButton::getPropertiesImplemented();
	auto local = { PropertyId::Checked };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

}} // namespace EE::UI
