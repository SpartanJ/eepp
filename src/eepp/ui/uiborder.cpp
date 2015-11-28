#include <eepp/ui/uiborder.hpp>

namespace EE { namespace UI {

UIBorder::UIBorder() :  mColor( 0xFF404040 ), mWidth( 1 ) {}
UIBorder::UIBorder( const UIBorder& border ) : mColor( border.Color() ), mWidth( border.Width() ) {}

const ColorA& UIBorder::Color() const	{
	return	mColor;
}

void UIBorder::Color( const ColorA& Col )	{
	mColor = Col;
}

const unsigned int& UIBorder::Width() const {
	return	mWidth;
}

void UIBorder::Width( const unsigned int& width )	{
	mWidth = width;
}

}}
