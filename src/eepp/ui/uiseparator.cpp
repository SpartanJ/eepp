#include <eepp/ui/uiseparator.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UISeparator::UISeparator( UIControlAnim::CreateParams Params ) :
	UIControlAnim( Params )
{
	ApplyDefaultTheme();
}

UISeparator::~UISeparator() {
}

Uint32 UISeparator::Type() const {
	return UI_TYPE_SEPARATOR;
}

bool UISeparator::IsType( const Uint32& type ) const {
	return UISeparator::Type() == type ? true : UIControlAnim::IsType( type );
}

void UISeparator::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "separator" );
	
	if ( NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->GetSubTexture( UISkinState::StateNormal ) ) {
		Size( mSize.width(), mSkinState->GetSkin()->GetSubTexture( UISkinState::StateNormal )->RealSize().height() );
	}
}

}}
