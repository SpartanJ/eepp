#include <eepp/ui/uiborder.hpp>
#include <eepp/graphics/primitives.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI {

UIBorder * UIBorder::New() {
	return eeNew( UIBorder, () );
}

UIBorder::UIBorder() :
	mColor( Color::Black ),
	mWidth( 1 )
{}

const Color& UIBorder::getColor() const	{
	return	mColor;
}

UIBorder * UIBorder::setColor( const Color& Col )	{
	mColor = Col;
	return this;
}

const unsigned int& UIBorder::getWidth() const {
	return	mWidth;
}

UIBorder * UIBorder::setWidth( const unsigned int& width )	{
	mWidth = width;
	return this;
}

void UIBorder::draw( Rectf R , const int& corners, const bool& clipping ) {
	Primitives P;
	P.setFillMode( DRAW_LINE );
	P.setLineWidth( PixelDensity::dpToPx( mWidth ) );
	P.setColor( mColor );

	if ( clipping ) {
		Rectf r( Vector2f( R.Left + 0.1f, R.Top + 0.1f ), Sizef( R.getWidth() - 0.1f, R.getHeight() - 0.1f ) );

		if ( corners ) {
			P.drawRoundedRectangle( r, 0.f, Vector2f::One, corners );
		} else {
			P.drawRectangle( R );
		}
	} else {
		if ( corners ) {
			P.drawRoundedRectangle( R, 0.f, Vector2f::One, corners );
		} else {
			P.drawRectangle( R );
		}
	}
}

}}
