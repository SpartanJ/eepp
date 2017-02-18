#include <eepp/ui/uiborder.hpp>

namespace EE { namespace UI {

UIBorder::UIBorder() :  mColor( 0xFF404040 ), mWidth( 1 ) {}
UIBorder::UIBorder( const UIBorder& border ) : mColor( border.getColor() ), mWidth( border.getWidth() ) {}

const ColorA& UIBorder::getColor() const	{
	return	mColor;
}

void UIBorder::setColor( const ColorA& Col )	{
	mColor = Col;
}

const unsigned int& UIBorder::getWidth() const {
	return	mWidth;
}

void UIBorder::setWidth( const unsigned int& width )	{
	mWidth = width;
}

}}
