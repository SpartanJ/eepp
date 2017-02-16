#include <eepp/ui/uiborder.hpp>

namespace EE { namespace UI {

UIBorder::UIBorder() :  mColor( 0xFF404040 ), mWidth( 1 ) {}
UIBorder::UIBorder( const UIBorder& border ) : mColor( border.color() ), mWidth( border.width() ) {}

const ColorA& UIBorder::color() const	{
	return	mColor;
}

void UIBorder::color( const ColorA& Col )	{
	mColor = Col;
}

const unsigned int& UIBorder::width() const {
	return	mWidth;
}

void UIBorder::width( const unsigned int& width )	{
	mWidth = width;
}

}}
