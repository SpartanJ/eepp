#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

UIMenuCheckBox::UIMenuCheckBox( UIMenuCheckBox::CreateParams& Params ) :
	UIMenuItem( Params ),
	mActive( false ),
	mSkinActive( NULL ),
	mSkinInactive( NULL )
{
	applyDefaultTheme();
}

UIMenuCheckBox::~UIMenuCheckBox() {
}

Uint32 UIMenuCheckBox::getType() const {
	return UI_TYPE_MENUCHECKBOX;
}

bool UIMenuCheckBox::isType( const Uint32& type ) const {
	return UIMenuCheckBox::getType() == type ? true : UIMenuItem::isType( type );
}

void UIMenuCheckBox::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "menuitem" );

	mSkinActive		= Theme->getByName( Theme->abbr() + "_" + "menucheckbox_active" );
	mSkinInactive	= Theme->getByName( Theme->abbr() + "_" + "menucheckbox_inactive" );

	active( mActive );

	doAftersetTheme();
}

const bool& UIMenuCheckBox::active() const {
	return mActive;
}

const bool& UIMenuCheckBox::isActive() const {
	return active();
}

void UIMenuCheckBox::active( const bool& active ) {
	bool oActive = mActive;
	mActive = active;

	if ( mActive ) {
		if ( NULL != mSkinActive ) {
			if ( mSkinState->getState() == UISkinState::StateSelected )
				icon( mSkinActive->getSubTexture( UISkinState::StateMouseEnter ) );
			else
				icon( mSkinActive->getSubTexture( UISkinState::StateNormal ) );
		} else
			mIcon->subTexture( NULL );
	} else {
		if ( NULL != mSkinInactive )
			if ( mSkinState->getState() == UISkinState::StateSelected )
				icon( mSkinInactive->getSubTexture( UISkinState::StateMouseEnter ) );
			else
				icon( mSkinInactive->getSubTexture( UISkinState::StateNormal ) );
		else
			mIcon->subTexture( NULL );
	}

	if ( oActive != active ) {
		UIMenu * Menu = reinterpret_cast<UIMenu*> ( getParent() );

		if ( !Menu->CheckControlSize( this ) ) {
			if ( NULL != icon()->subTexture() ) {
				padding( Recti( 0, 0, 0, 0 ) );
			}
		}

		onValueChange();
	}
}

void UIMenuCheckBox::switchActive() {
	active( !mActive );
}

Uint32 UIMenuCheckBox::onMouseUp( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::onMouseUp( Pos, Flags );

	if ( getParent()->isVisible() && ( Flags & EE_BUTTONS_LRM ) )
		switchActive();

	return 1;
}

void UIMenuCheckBox::onStateChange() {
	UIMenuItem::onStateChange();

	active( mActive );
}

bool UIMenuCheckBox::inheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

}}
