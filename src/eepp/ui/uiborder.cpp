#include <eepp/ui/uiborder.hpp>

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

}}
