#include "cuiseparator.hpp"
 
namespace EE { namespace UI {

cUISeparator::cUISeparator( cUIControlAnim::CreateParams Params ) :
	cUIControlAnim( Params )
{
	mType = UI_TYPE_SEPARATOR;
	
	ApplyDefaultTheme();
}

cUISeparator::~cUISeparator() {
}
		
void cUISeparator::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "separator" );
	
	if ( NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->GetShape( cUISkinState::StateNormal ) ) {
		Size( mSize.Width(), mSkinState->GetSkin()->GetShape( cUISkinState::StateNormal )->RealSize().Height() );
	}
}

}}
