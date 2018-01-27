#include <eepp/ui/uiborder.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/graphics/primitives.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI {

UIBorder * UIBorder::New( UINode * control ) {
	return eeNew( UIBorder, ( control ) );
}

UIBorder::UIBorder( UINode * control ) :
	mControl( control ),
	mColor( Color::Black ),
	mWidth( 1 )
{}

const Color& UIBorder::getColor() const	{
	return	mColor;
}

UIBorder * UIBorder::setColor( const Color& Col )	{
	mColor = Col;
	mControl->invalidateDraw();
	return this;
}

const unsigned int& UIBorder::getWidth() const {
	return	mWidth;
}

UIBorder * UIBorder::setWidth( const unsigned int& width )	{
	mWidth = width;
	mControl->invalidateDraw();
	return this;
}

void UIBorder::draw( Rectf R , const Float& alpha, const int& corners ) {
	Primitives P;
	P.setFillMode( DRAW_LINE );
	P.setLineWidth( PixelDensity::dpToPx( mWidth ) );
	P.setColor( 255 == alpha ? mColor : Color( mColor.r, mColor.g, mColor.b, static_cast<Uint8>( (Float)mColor.a * alpha / 255.f ) ) );

	if ( corners ) {
		P.drawRoundedRectangle( R, 0.f, Vector2f::One, corners );
	} else {
		P.drawRectangle( R );
	}
}

}}
