#include <eepp/ui/uibackground.hpp>

namespace EE { namespace UI {

UIBackground::UIBackground() :
	mBlendMode( ALPHA_NORMAL ),
	mCorners(0)
{
	mColor.push_back( ColorA(0xFF404040) );
}

UIBackground::UIBackground( const UIBackground& Back ) :
	mBlendMode( ALPHA_NORMAL ),
	mCorners( Back.Corners() )
{
	UIBackground * b = const_cast<UIBackground *> ( &Back ); // cheating
	mColor = b->Colors();
}

UIBackground::UIBackground( const ColorA& Color, const unsigned int& Corners, const EE_BLEND_MODE& BlendMode ) :
	mBlendMode( BlendMode ),
	mCorners( Corners )
{
	mColor.push_back( Color );
}

UIBackground::UIBackground( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor, const unsigned int& Corners, const EE_BLEND_MODE& BlendMode ) :
	mBlendMode( BlendMode ),
	mCorners( Corners )
{
	Colors( TopLeftColor, BottomLeftColor, BottomRightColor, TopRightColor );
}

ColorA& UIBackground::Color( const unsigned int& index  ) {
	if ( index < mColor.size() )
		return	mColor[ index ];

	return mColor[ 0 ];
}

void UIBackground::ColorsTo( const ColorA& Color ) {
	for ( unsigned int i = 0; i < mColor.size(); i++ )
		mColor[i] = Color;
}

void UIBackground::Colors( const ColorA& TopLeftColor, const ColorA& BottomLeftColor, const ColorA& BottomRightColor, const ColorA& TopRightColor ) {
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

const std::vector<ColorA>& UIBackground::Colors() {
	return	mColor;
}

void UIBackground::Color( const ColorA& Col ) {
	mColor[0] = Col;
}

const EE_BLEND_MODE& UIBackground::Blend() const {
	return mBlendMode;
}

void UIBackground::Blend( const EE_BLEND_MODE& blend ) {
	mBlendMode = blend;
}

const unsigned int& UIBackground::Corners() const {
	return mCorners;
}

void UIBackground::Corners( const unsigned int& corners ) {
	mCorners = corners;
}

}}
