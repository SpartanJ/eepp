#include <eepp/ui/uibackground.hpp>

namespace EE { namespace UI {

UIBackground * UIBackground::New() {
	return eeNew( UIBackground, () );
}

UIBackground::UIBackground() :
	mBlendMode( ALPHA_NORMAL ),
	mCorners(0)
{
	mColor.push_back( Color(0xFF404040) );
}

Color& UIBackground::getColor( const unsigned int& index  ) {
	if ( index < mColor.size() )
		return	mColor[ index ];

	return mColor[ 0 ];
}

UIBackground * UIBackground::setColorsTo( const Color& Color ) {
	for ( unsigned int i = 0; i < mColor.size(); i++ )
		mColor[i] = Color;

	return this;
}

UIBackground * UIBackground::setColors( const Color& TopLeftColor, const Color& BottomLeftColor, const Color& BottomRightColor, const Color& TopRightColor ) {
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

	return this;
}

const std::vector<Color>& UIBackground::getColors() {
	return	mColor;
}

UIBackground * UIBackground::setColor( const Color& Col ) {
	mColor[0] = Col;
	return this;
}

const EE_BLEND_MODE& UIBackground::getBlendMode() const {
	return mBlendMode;
}

UIBackground * UIBackground::setBlendMode( const EE_BLEND_MODE& blend ) {
	mBlendMode = blend;
	return this;
}

const unsigned int& UIBackground::getCorners() const {
	return mCorners;
}

UIBackground * UIBackground::setCorners( const unsigned int& corners ) {
	mCorners = corners;
	return this;
}

}}
