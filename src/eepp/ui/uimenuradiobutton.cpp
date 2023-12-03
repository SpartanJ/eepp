#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenuradiobutton.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

UIMenuRadioButton* UIMenuRadioButton::New() {
	return eeNew( UIMenuRadioButton, () );
}

UIMenuRadioButton::UIMenuRadioButton() :
	UIMenuItem( "menu::radiobutton" ),
	mActive( false ),
	mSkinActive( NULL ),
	mSkinInactive( NULL ) {
	mTextBox->setElementTag( mTag + "::text" );
	applyDefaultTheme();
	mIcon->setFlags( UI_SKIN_KEEP_SIZE_ON_DRAW );
}

UIMenuRadioButton::~UIMenuRadioButton() {}

Uint32 UIMenuRadioButton::getType() const {
	return UI_TYPE_MENURADIOBUTTON;
}

bool UIMenuRadioButton::isType( const Uint32& type ) const {
	return UIMenuRadioButton::getType() == type ? true : UIMenuItem::isType( type );
}

void UIMenuRadioButton::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "menuitem" );

	mSkinActive = Theme->getSkin( "menuradiobutton_active" );
	mSkinInactive = Theme->getSkin( "menuradiobutton_inactive" );

	mActive = !mActive;
	setActive( !mActive );

	onThemeLoaded();
}

const bool& UIMenuRadioButton::isActive() const {
	return mActive;
}

UIMenuRadioButton* UIMenuRadioButton::setActive( const bool& active ) {
	if ( mActive == active )
		return this;

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

	if ( getParent()->isType( UI_TYPE_MENU ) ) {
		UIMenu* menu = getParent()->asType<UIMenu>();

		if ( !menu->widgetCheckSize( this ) ) {
			if ( NULL != getIcon()->getDrawable() ) {
				setPadding( Rectf( 0, 0, 0, 0 ) );
			}
		}

		if ( mActive ) {
			Node* child = menu->getFirstChild();
			while ( child ) {
				if ( child->isType( UI_TYPE_MENURADIOBUTTON ) && child != this ) {
					child->asType<UIMenuRadioButton>()->setActive( false );
				}
				child = child->getNextNode();
			}
		}
	}

	onValueChange();
	return this;
}

void UIMenuRadioButton::switchActive() {
	setActive( !mActive );
}

Uint32 UIMenuRadioButton::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	UIMenuItem::onMouseUp( Pos, Flags );

	if ( getParent()->isVisible() && ( Flags & EE_BUTTONS_LRM ) )
		switchActive();

	return 1;
}

void UIMenuRadioButton::onStateChange() {
	UIMenuItem::onStateChange();

	setActive( mActive );
}

std::string UIMenuRadioButton::getPropertyString( const PropertyDefinition* propertyDef,
												  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Value:
			return isActive() ? "true" : "false";
			break;
		default:
			return UIMenuItem::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIMenuRadioButton::getPropertiesImplemented() const {
	auto props = UIMenuItem::getPropertiesImplemented();
	auto local = { PropertyId::Selected };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIMenuRadioButton::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Value:
			setActive( attribute.asBool() );
			break;
		default:
			return UIMenuItem::applyProperty( attribute );
	}

	return true;
}

}} // namespace EE::UI
