#include "cuibackground.hpp"

namespace EE { namespace UI {

cUIBackground::cUIBackground() : 
	mColor( 0xFF404040 ), 
	mBlendMode( ALPHA_NORMAL )
{
}

cUIBackground::cUIBackground( const cUIBackground& Back ) : 
	mColor( Back.Color() ), 
	mBlendMode( ALPHA_NORMAL )
{
}

const eeColorA& cUIBackground::Color() const {
	return	mColor;
}

void cUIBackground::Color( const eeColorA& Col ) {
	mColor = Col;
}

const EE_RENDERALPHAS& cUIBackground::Blend() const {
	return mBlendMode;
}

void cUIBackground::Blend( const EE_RENDERALPHAS& blend ) {
	mBlendMode = blend;
}

}}
