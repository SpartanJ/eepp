#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/math.hpp>
#include <eepp/graphics/pixeldensity.hpp>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace CSS {

StyleSheetLength::Unit StyleSheetLength::unitFromString( std::string unitStr ) {
	String::toLowerInPlace( unitStr );
	if ( "%" == unitStr ) return Unit::Percentage;
	else if ( "dp" == unitStr ) return Unit::Dp;
	else if ( "px" == unitStr ) return Unit::Px;
	else if ( "in" == unitStr ) return Unit::In;
	else if ( "cm" == unitStr ) return Unit::Cm;
	else if ( "mm" == unitStr ) return Unit::Mm;
	else if ( "em" == unitStr ) return Unit::Em;
	else if ( "ex" == unitStr ) return Unit::Ex;
	else if ( "pt" == unitStr ) return Unit::Pt;
	else if ( "pc" == unitStr ) return Unit::Pc;
	else if ( "px" == unitStr ) return Unit::Px;
	else if ( "dpi" == unitStr ) return Unit::Dpi;
	else if ( "dpcm" == unitStr ) return Unit::Dpcm;
	else if ( "vw" == unitStr ) return Unit::Vw;
	else if ( "vh" == unitStr ) return Unit::Vh;
	else if ( "vmin" == unitStr ) return Unit::Vmin;
	else if ( "vmax" == unitStr ) return Unit::Vmax;
	else if ( "rem" == unitStr ) return Unit::Dp;
	return Unit::Px;
}

StyleSheetLength::StyleSheetLength() :
	mUnit(Px),
	mValue(0)
{}

StyleSheetLength::StyleSheetLength( std::string val, const Float& defaultValue ) :
	StyleSheetLength( fromString( val, defaultValue ) ) {
}

StyleSheetLength::StyleSheetLength(const StyleSheetLength& val) {
	mUnit = val.mUnit;
	mValue = val.mValue;
}

void StyleSheetLength::setValue(const Float& val, const StyleSheetLength::Unit& units) {
	mValue = val;
	mUnit = units;
}

const Float& StyleSheetLength::getValue() const {
	return mValue;
}

const StyleSheetLength::Unit& StyleSheetLength::getUnit() const {
	return mUnit;
}

Float StyleSheetLength::asPixels( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi, const Float& elFontSize, const Float& globalFontSize ) const {
	Float ret = 0;
	switch( mUnit ) {
		case Unit::Percentage:
			ret = parentSize * mValue / 100.f;
			break;
		case Unit::Dp:
			ret = PixelDensity::dpToPx( mValue );
			break;
		case Unit::Em:
			ret = Math::round( mValue * elFontSize );
			break;
		case Unit::Pt:
			ret = ( mValue * displayDpi / 72.f );
			break;
		case Unit::Pc:
			ret = ( mValue * displayDpi / 72.f ) * 12.f;
			break;
		case Unit::In:
			ret = mValue * displayDpi;
			break;
		case Unit::Cm:
			ret = mValue * displayDpi * 0.3937f;
			break;
		case Unit::Mm:
			ret = mValue * displayDpi * 0.3937f / 10.f;
			break;
		case Unit::Vw:
			ret = viewSize.getWidth() * mValue / 100.0;
			break;
		case Unit::Vh:
			ret = viewSize.getHeight() * mValue / 100.f;
			break;
		case Unit::Vmin:
			ret = std::min( viewSize.getHeight(), viewSize.getWidth() ) * mValue / 100.f;
			break;
		case Unit::Vmax:
			ret = std::max( viewSize.getHeight(), viewSize.getWidth() ) * mValue / 100.f;
			break;
		case Unit::Rem:
			ret = globalFontSize * mValue;
			break;
		default:
			ret = mValue;
			break;
	}
	return ret;
}

Float StyleSheetLength::asDp( const Float& parentSize, const Sizef& viewSize, const Float& displayDpi, const Float& elFontSize, const Float& globalFontSize ) const {
	return PixelDensity::pxToDp( asPixels( parentSize, viewSize, displayDpi, elFontSize, globalFontSize ) );
}

bool StyleSheetLength::operator==( const StyleSheetLength& length ) {
	return mValue == length.mValue && mUnit == length.mUnit;
}

StyleSheetLength& StyleSheetLength::operator=( const Float& val ) {
	mValue = val;
	mUnit = Unit::Px;
	return *this;
}

StyleSheetLength& StyleSheetLength::operator=( const StyleSheetLength& val) {
	mUnit = val.mUnit;
	mValue = val.mValue;
	return *this;
}

static std::string positionToPercentage( const std::string& pos ) {
	if ( pos == "center" ) return "50%";
	if ( pos == "left" || pos == "top" ) return "0%";
	if ( pos == "right" || pos == "bottom" ) return "100%";
	return pos;
}

StyleSheetLength StyleSheetLength::fromString( std::string str , const Float& defaultValue ) {
	StyleSheetLength length;
	length.setValue( defaultValue, Unit::Px );
	std::string num;
	std::string unit;
	str = positionToPercentage( str );

	for ( std::size_t i = 0; i < str.size(); i++ ) {
		if ( String::isNumber( str[i], true ) || ( '-' == str[i] && i == 0 ) || ( '+' == str[i] && i == 0 ) ) {
			num += str[i];
		} else {
			unit = str.substr( i );
			break;
		}
	}

	if ( !num.empty() ) {
		Float val = 0;
		bool res = String::fromString<Float>( val, num );

		if ( res ) {
			length.setValue( val, unitFromString( unit ) );
		}
	}

	return length;
}

}}}
