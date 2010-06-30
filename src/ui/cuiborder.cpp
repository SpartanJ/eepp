#include "cuiborder.hpp"

namespace EE { namespace UI {

cUIBorder::cUIBorder() :  mColor( 0xFF404040 ), mWidth( 1 ) {}
cUIBorder::cUIBorder( const cUIBorder& border ) : mColor( border.Color() ), mWidth( border.Width() ) {}

const eeColorA& cUIBorder::Color() const	{
	return	mColor;
}

void cUIBorder::Color( const eeColorA& Col )	{
	mColor = Col;
}

const eeUint& cUIBorder::Width() const {
	return	mWidth;
}

void cUIBorder::Width( const eeUint& width )	{
	mWidth = width;
}

}}