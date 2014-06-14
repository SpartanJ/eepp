#include <eepp/ui/cuibackground.hpp>

namespace EE { namespace UI {

cUIBackground::cUIBackground() :
	mBlendMode( ALPHA_NORMAL ),
	mCorners(0)
{
	mColor.push_back( eeColorA(0xFF404040) );
}

cUIBackground::cUIBackground( const cUIBackground& Back ) :
	mBlendMode( ALPHA_NORMAL ),
	mCorners( Back.Corners() )
{
	cUIBackground * b = const_cast<cUIBackground *> ( &Back ); // cheating
	mColor = b->Colors();
}

cUIBackground::cUIBackground( const eeColorA& Color, const unsigned int& Corners, const EE_BLEND_MODE& BlendMode ) :
	mBlendMode( BlendMode ),
	mCorners( Corners )
{
	mColor.push_back( Color );
}

cUIBackground::cUIBackground( const eeColorA& TopLeftColor, const eeColorA& BottomLeftColor, const eeColorA& BottomRightColor, const eeColorA& TopRightColor, const unsigned int& Corners, const EE_BLEND_MODE& BlendMode ) :
	mBlendMode( BlendMode ),
	mCorners( Corners )
{
	Colors( TopLeftColor, BottomLeftColor, BottomRightColor, TopRightColor );
}

eeColorA& cUIBackground::Color( const unsigned int& index  ) {
	if ( index < mColor.size() )
		return	mColor[ index ];

	return mColor[ 0 ];
}

void cUIBackground::ColorsTo( const eeColorA& Color ) {
	for ( unsigned int i = 0; i < mColor.size(); i++ )
		mColor[i] = Color;
}

void cUIBackground::Colors( const eeColorA& TopLeftColor, const eeColorA& BottomLeftColor, const eeColorA& BottomRightColor, const eeColorA& TopRightColor ) {
	mColor[0] = TopLeftColor;

	if ( mColor.size() < 2 )
		mColor.push_back( BottomLeftColor );
	else
		mColor[1] = BottomLeftColor;

	if ( mColor.size() < 3 )
		mColor.push_back( BottomRightColor );
	else
		mColor[2] = BottomRightColor;

	if ( mColor.size() < 4 )
		mColor.push_back( TopRightColor );
	else
		mColor[3] = TopRightColor;
}

const std::vector<eeColorA>& cUIBackground::Colors() {
	return	mColor;
}

void cUIBackground::Color( const eeColorA& Col ) {
	mColor[0] = Col;
}

const EE_BLEND_MODE& cUIBackground::Blend() const {
	return mBlendMode;
}

void cUIBackground::Blend( const EE_BLEND_MODE& blend ) {
	mBlendMode = blend;
}

const unsigned int& cUIBackground::Corners() const {
	return mCorners;
}

void cUIBackground::Corners( const unsigned int& corners ) {
	mCorners = corners;
}

}}
