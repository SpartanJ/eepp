#include <eepp/system/luapattern.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/math/math.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>

using namespace EE::Graphics;
using namespace std::literals;

namespace EE { namespace UI { namespace CSS {

enum UnitHashes : String::HashType {
	Percentage = String::hash( "%" ),
	In = String::hash( "in" ),
	Cm = String::hash( "cm" ),
	Mm = String::hash( "mm" ),
	Em = String::hash( "em" ),
	Ex = String::hash( "ex" ),
	Pt = String::hash( "pt" ),
	Pc = String::hash( "pc" ),
	Px = String::hash( "px" ),
	Dpi = String::hash( "dpi" ),
	Dp = String::hash( "dp" ),
	Dpcm = String::hash( "dpcm" ),
	Vw = String::hash( "vw" ),
	Vh = String::hash( "vh" ),
	Vmin = String::hash( "vmin" ),
	Vmax = String::hash( "vmax" ),
	Rem = String::hash( "rem" ),
	Dprd = String::hash( "dprd" ),
	Dpru = String::hash( "dpru" ),
	Dpr = String::hash( "dpr" ),
};

enum PercentagePositions : String::HashType {
	Center = String::hash( "center" ),
	Left = String::hash( "left" ),
	Right = String::hash( "right" ),
	Top = String::hash( "top" ),
	Bottom = String::hash( "bottom" ),
	None = 0,
};

static std::string positionToPercentage( const PercentagePositions& pos ) {
	switch ( pos ) {
		case Center:
			return "50%";
		case Left:
		case Top:
			return "0%";
		case Right:
		case Bottom:
			return "100%";
		default:
		case None:
			return "";
	}
}

static PercentagePositions isPercentagePosition( const String::HashType& strHash ) {
	switch ( strHash ) {
		case PercentagePositions::Center:
			return PercentagePositions::Center;
		case PercentagePositions::Left:
			return PercentagePositions::Left;
		case PercentagePositions::Right:
			return PercentagePositions::Right;
		case PercentagePositions::Top:
			return PercentagePositions::Top;
		case PercentagePositions::Bottom:
			return PercentagePositions::Bottom;
	}
	return PercentagePositions::None;
}

StyleSheetLength::Unit StyleSheetLength::unitFromString( std::string unitStr ) {
	String::toLowerInPlace( unitStr );
	switch ( String::hash( unitStr ) ) {
		case UnitHashes::Percentage:
			return Unit::Percentage;
		case UnitHashes::Dp:
			return Unit::Dp;
		case UnitHashes::Px:
			return Unit::Px;
		case UnitHashes::In:
			return Unit::In;
		case UnitHashes::Cm:
			return Unit::Cm;
		case UnitHashes::Mm:
			return Unit::Mm;
		case UnitHashes::Em:
			return Unit::Em;
		case UnitHashes::Ex:
			return Unit::Ex;
		case UnitHashes::Pt:
			return Unit::Pt;
		case UnitHashes::Pc:
			return Unit::Pc;
		case UnitHashes::Dpi:
			return Unit::Dpi;
		case UnitHashes::Dpcm:
			return Unit::Dpcm;
		case UnitHashes::Vw:
			return Unit::Vw;
		case UnitHashes::Vh:
			return Unit::Vh;
		case UnitHashes::Vmin:
			return Unit::Vmin;
		case UnitHashes::Vmax:
			return Unit::Vmax;
		case UnitHashes::Rem:
			return Unit::Rem;
		case UnitHashes::Dprd:
			return Unit::Dprd;
		case UnitHashes::Dpru:
			return Unit::Dpru;
		case UnitHashes::Dpr:
			return Unit::Dpr;
	}
	return Unit::Px;
}

std::string StyleSheetLength::unitToString( const StyleSheetLength::Unit& unit ) {
	switch ( unit ) {
		case Unit::Percentage:
			return "%";
		case Unit::Dp:
			return "dp";
		case Unit::Px:
			return "px";
		case Unit::In:
			return "in";
		case Unit::Cm:
			return "cm";
		case Unit::Mm:
			return "mm";
		case Unit::Em:
			return "em";
		case Unit::Ex:
			return "ex";
		case Unit::Pt:
			return "pt";
		case Unit::Pc:
			return "pc";
		case Unit::Dpi:
			return "dpi";
		case Unit::Dpcm:
			return "dpcm";
		case Unit::Vw:
			return "vw";
		case Unit::Vh:
			return "vh";
		case Unit::Vmin:
			return "vmin";
		case Unit::Vmax:
			return "vmax";
		case Unit::Rem:
			return "rem";
		case Unit::Dprd:
			return "dprd";
		case Unit::Dpru:
			return "dpru";
		case Unit::Dpr:
			return "dpr";
	}
	return "px";
}

bool StyleSheetLength::isLength( const std::string& unitStr ) {
	LuaPattern ptrn( "(-?%d+[%d%.eE]*)(%w*)" );
	PatternMatcher::Range matches[4];
	if ( ptrn.matches( unitStr, matches ) ) {
		if ( matches[2].isValid() ) {
			std::string unit =
				unitStr.substr( matches[2].start, matches[2].end - matches[2].start );
			auto unitType = unitFromString( unit );
			if ( unitType != StyleSheetLength::Unit::Px || unit == "px" )
				return true;
			return false;
		}
		return true;
	}
	return false;
}

StyleSheetLength::StyleSheetLength() : mUnit( Px ), mValue( 0 ) {}

StyleSheetLength::StyleSheetLength( const Float& val, const StyleSheetLength::Unit& unit ) :
	mUnit( unit ), mValue( val ) {}

StyleSheetLength::StyleSheetLength( const std::string& val, const Float& defaultValue ) :
	StyleSheetLength( fromString( val, defaultValue ) ) {}

StyleSheetLength::StyleSheetLength( const StyleSheetLength& val ) {
	mUnit = val.mUnit;
	mValue = val.mValue;
}

void StyleSheetLength::setValue( const Float& val, const StyleSheetLength::Unit& units ) {
	mValue = val;
	mUnit = units;
}

const Float& StyleSheetLength::getValue() const {
	return mValue;
}

const StyleSheetLength::Unit& StyleSheetLength::getUnit() const {
	return mUnit;
}

Float StyleSheetLength::asPixels( const Float& parentSize, const Sizef& viewSize,
								  const Float& displayDpi, const Float& elFontSize,
								  const Float& globalFontSize ) const {
	Float ret = 0;
	switch ( mUnit ) {
		case Unit::Percentage:
			ret = parentSize * mValue / 100.f;
			break;
		case Unit::Dp:
			ret = PixelDensity::dpToPx( mValue );
			break;
		case Unit::Dpr:
			return round( PixelDensity::dpToPx( mValue ) );
		case Unit::Dprd:
			ret = Math::roundDown( PixelDensity::dpToPx( mValue ) );
			break;
		case Unit::Dpru:
			ret = Math::roundUp( PixelDensity::dpToPx( mValue ) );
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
			ret = viewSize.getWidth() * mValue / 100.f;
			break;
		case Unit::Vh:
			ret = viewSize.getHeight() * mValue / 100.f;
			break;
		case Unit::Vmin:
			ret = eemin( viewSize.getHeight(), viewSize.getWidth() ) * mValue / 100.f;
			break;
		case Unit::Vmax:
			ret = eemax( viewSize.getHeight(), viewSize.getWidth() ) * mValue / 100.f;
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

Float StyleSheetLength::asDp( const Float& parentSize, const Sizef& viewSize,
							  const Float& displayDpi, const Float& elFontSize,
							  const Float& globalFontSize ) const {
	return PixelDensity::pxToDp(
		asPixels( parentSize, viewSize, displayDpi, elFontSize, globalFontSize ) );
}

bool StyleSheetLength::operator==( const StyleSheetLength& length ) const {
	return mValue == length.mValue && mUnit == length.mUnit;
}

bool StyleSheetLength::operator!=( const StyleSheetLength& length ) const {
	return !( *this == length );
}

StyleSheetLength& StyleSheetLength::operator=( const Float& val ) {
	mValue = val;
	mUnit = Unit::Px;
	return *this;
}

StyleSheetLength& StyleSheetLength::operator=( const StyleSheetLength& val ) {
	mUnit = val.mUnit;
	mValue = val.mValue;
	return *this;
}

StyleSheetLength StyleSheetLength::fromString( const std::string& str, const Float& defaultValue ) {
	PercentagePositions isPercentage = isPercentagePosition( String::hash( str ) );
	if ( PercentagePositions::None != isPercentage )
		return fromString( positionToPercentage( isPercentage ), defaultValue );

	StyleSheetLength length;
	length.setValue( defaultValue, Unit::Px );
	std::string num;
	std::string unit;

	for ( std::size_t i = 0; i < str.size(); i++ ) {
		if ( String::isNumber( str[i], true ) || ( '-' == str[i] && i == 0 ) ||
			 ( '+' == str[i] && i == 0 ) ) {
			num += str[i];
		} else {
			unit = str.substr( i );
			break;
		}
	}

	if ( !num.empty() ) {
		Float val = 0;
		if ( String::fromString<Float>( val, num ) )
			length.setValue( val, unitFromString( unit ) );
	}

	return length;
}

std::string StyleSheetLength::toString() const {
	std::string res( String::format( "%.2f", mValue ) );
	String::replace( res, ",", "." );
	String::numberCleanInPlace( res );
	res += unitToString( mUnit );
	return res;
}

}}} // namespace EE::UI::CSS
