#include <eepp/ui/uiseparator.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UISeparator::UISeparator( UIControlAnim::CreateParams Params ) :
	UIControlAnim( Params )
{
	applyDefaultTheme();
}

UISeparator::~UISeparator() {
}

Uint32 UISeparator::getType() const {
	return UI_TYPE_SEPARATOR;
}

bool UISeparator::isType( const Uint32& type ) const {
	return UISeparator::getType() == type ? true : UIControlAnim::isType( type );
}

void UISeparator::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "separator" );
	
	if ( NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->getSubTexture( UISkinState::StateNormal ) ) {
		size( mSize.width(), mSkinState->getSkin()->getSubTexture( UISkinState::StateNormal )->realSize().height() );
	}
}

}}
