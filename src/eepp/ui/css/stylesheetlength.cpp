#include <eepp/core/string.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/math/math.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <limits>

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
	Ch = String::hash( "ch" ),
};

enum PercentagePositions : String::HashType {
	Center = String::hash( "center" ),
	Left = String::hash( "left" ),
	Right = String::hash( "right" ),
	Top = String::hash( "top" ),
	Bottom = String::hash( "bottom" ),
	None = 0,
};

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

static size_t numericPrefixLength( std::string_view value ) {
	if ( value.empty() )
		return 0;
	size_t pos = 0;
	if ( value[pos] == '-' || value[pos] == '+' )
		pos++;

	bool hasDigit = false;
	bool hasDot = false;
	while ( pos < value.size() ) {
		const char c = value[pos];
		if ( c >= '0' && c <= '9' ) {
			hasDigit = true;
			pos++;
		} else if ( c == '.' && !hasDot ) {
			hasDot = true;
			pos++;
		} else {
			break;
		}
	}

	if ( !hasDigit )
		return 0;

	if ( pos < value.size() && ( value[pos] == 'e' || value[pos] == 'E' ) ) {
		size_t expPos = pos + 1;
		if ( expPos < value.size() && ( value[expPos] == '-' || value[expPos] == '+' ) )
			expPos++;
		const size_t expDigits = expPos;
		while ( expPos < value.size() && value[expPos] >= '0' && value[expPos] <= '9' )
			expPos++;
		if ( expPos != expDigits )
			pos = expPos;
	}

	return pos;
}

StyleSheetLength::Unit StyleSheetLength::unitFromString( std::string_view unitStr ) {
	switch ( String::hashToLower( unitStr ) ) {
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
		case UnitHashes::Ch:
			return Unit::Ch;
	}
	return Unit::Dp;
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
		case Unit::Ch:
			return "ch";
		case Unit::Clamp:
			return "clamp";
		case Unit::Min:
			return "min";
		case Unit::Max:
			return "max";
		case Unit::Calc:
			return "calc";
	}
	return "px";
}

bool StyleSheetLength::isLength( std::string_view unitStr ) {
	if ( unitStr.empty() )
		return false;

	if ( isFunctionString( unitStr ) )
		return true;

	const size_t pos = numericPrefixLength( unitStr );
	if ( pos == 0 )
		return false;

	std::string_view unit = unitStr.substr( pos );
	if ( unit.empty() )
		return true;

	auto unitType = unitFromString( unit );
	return unitType != StyleSheetLength::Unit::Px || unit == "px";
}

bool StyleSheetLength::isPercentage( std::string_view val ) {
	return !val.empty() && val.back() == '%';
}

StyleSheetLength::StyleSheetLength() : mUnit( Unit::Px ), mValue( 0 ) {}

StyleSheetLength::StyleSheetLength( const Float& val, const StyleSheetLength::Unit& unit ) :
	mUnit( unit ), mValue( val ) {}

StyleSheetLength::StyleSheetLength( const std::string& val, const Float& defaultValue ) :
	StyleSheetLength( fromString( val, defaultValue ) ) {}

StyleSheetLength::StyleSheetLength( const StyleSheetLength& val ) {
	mUnit = val.mUnit;
	mValue = val.mValue;
	mArgs = val.mArgs;
}

void StyleSheetLength::setValue( const Float& val, const StyleSheetLength::Unit& units ) {
	mValue = val;
	mUnit = units;
	mArgs.clear();
}

const Float& StyleSheetLength::getValue() const {
	return mValue;
}

const StyleSheetLength::Unit& StyleSheetLength::getUnit() const {
	return mUnit;
}

Float StyleSheetLength::asPixels( const Float& parentSize, const Sizef& viewSize,
								  const Float& displayDpi, const Float& elFontSize,
								  const Float& globalFontSize, Graphics::Font* font ) const {
	if ( mUnit == Unit::Clamp || mUnit == Unit::Min || mUnit == Unit::Max )
		return resolveFunction( parentSize, viewSize, displayDpi, elFontSize, globalFontSize,
								font );

	if ( mUnit == Unit::Calc )
		return resolveCalc( parentSize, viewSize, displayDpi, elFontSize, globalFontSize, font );

	Float ret = 0;

	// CSS dictates a base 96 DPI for logical pixels.
	// We multiply by the device pixel ratio to get actual physical pixels on screen.
	const Float CSS_DPI = 96.f * PixelDensity::getPixelDensity();

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
		case Unit::Ex:
			if ( font && elFontSize > 0 ) {
				Glyph glyph =
					font->getGlyph( 'x', static_cast<unsigned int>( elFontSize ), false, false );
				Float xHeight = eemax( 0.f, -glyph.bounds.Top );
				if ( xHeight <= 0 )
					xHeight = elFontSize * 0.5f;
				ret = Math::round( mValue * xHeight );
			} else {
				ret = Math::round( mValue * elFontSize * 0.5f );
			}
			break;
		case Unit::Ch:
			if ( font && elFontSize > 0 ) {
				Glyph glyph =
					font->getGlyph( '0', static_cast<unsigned int>( elFontSize ), false, false );
				ret = Math::round( mValue * eemax( 0.f, glyph.advance ) );
			} else {
				ret = Math::round( mValue * elFontSize * 0.5f );
			}
			break;
		case Unit::Pt:
			ret = mValue * CSS_DPI / 72.f;
			break;
		case Unit::Pc:
			ret = ( mValue * CSS_DPI / 72.f ) * 12.f;
			break;
		case Unit::In:
			ret = mValue * CSS_DPI;
			break;
		case Unit::Cm:
			ret = ( mValue * CSS_DPI ) / 2.54f;
			break;
		case Unit::Mm:
			ret = ( mValue * CSS_DPI ) / 25.4f;
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
		case Unit::Dpi:
		case Unit::Dpcm:
			ret = (int)( mValue * 2.54 );
			break;
		case Unit::Px:
		default:
			ret = mValue;
			break;
	}
	return ret;
}

Float StyleSheetLength::asDp( const Float& parentSize, const Sizef& viewSize,
							  const Float& displayDpi, const Float& elFontSize,
							  const Float& globalFontSize, Graphics::Font* font ) const {
	return PixelDensity::pxToDp(
		asPixels( parentSize, viewSize, displayDpi, elFontSize, globalFontSize, font ) );
}

bool StyleSheetLength::operator==( const StyleSheetLength& length ) const {
	return mValue == length.mValue && mUnit == length.mUnit && mArgs == length.mArgs;
}

bool StyleSheetLength::operator!=( const StyleSheetLength& length ) const {
	return !( *this == length );
}

StyleSheetLength& StyleSheetLength::operator=( const Float& val ) {
	mValue = val;
	mUnit = Unit::Px;
	mArgs.clear();
	return *this;
}

StyleSheetLength& StyleSheetLength::operator=( const StyleSheetLength& val ) {
	mUnit = val.mUnit;
	mValue = val.mValue;
	mArgs = val.mArgs;
	return *this;
}

StyleSheetLength StyleSheetLength::fromString( const std::string& str, const Float& defaultValue,
											   bool pxAsDp ) {
	StyleSheetLength length;
	length.setValue( defaultValue, Unit::Px );
	const std::string_view value = String::trim( std::string_view( str ) );

	if ( isFunctionString( value ) ) {
		Unit funcUnit = Unit::Px;
		Arguments args;
		if ( parseFunction( str, funcUnit, args ) ) {
			length.mUnit = funcUnit;
			length.mArgs = std::move( args );
			length.mValue = 0;
			return length;
		}
		return length;
	}

	if ( !value.empty() ) {
		const char first = value.front();
		const bool startsNumeric =
			( first >= '0' && first <= '9' ) || first == '.' || first == '-' || first == '+';
		if ( startsNumeric ) {
			const size_t numberLength = numericPrefixLength( value );
			if ( numberLength != 0 ) {
				std::string_view number = value.substr( 0, numberLength );
				if ( number.front() == '+' )
					number.remove_prefix( 1 );
				Float parsedValue = 0;
				if ( String::fromString( parsedValue, number ) )
					length.setValue( parsedValue, unitFromString( value.substr( numberLength ) ) );
			}
		} else {
			switch ( isPercentagePosition( String::hashToLower( value ) ) ) {
				case PercentagePositions::Center:
					length.setValue( 50, Unit::Percentage );
					break;
				case PercentagePositions::Right:
				case PercentagePositions::Bottom:
					length.setValue( 100, Unit::Percentage );
					break;
				case PercentagePositions::Left:
				case PercentagePositions::Top:
					length.setValue( 0, Unit::Percentage );
					break;
				case PercentagePositions::None:
					break;
			}
		}
	}

	if ( pxAsDp && length.getUnit() == Unit::Px )
		length.mUnit = Unit::Dp;

	return length;
}

std::string StyleSheetLength::toString() const {
	if ( mUnit == Unit::Clamp || mUnit == Unit::Min || mUnit == Unit::Max || mUnit == Unit::Calc ) {
		std::string res( unitToString( mUnit ) );
		res += "(";
		for ( size_t i = 0; i < mArgs.size(); i++ ) {
			if ( i > 0 )
				res += ", ";
			res += mArgs[i];
		}
		res += ")";
		return res;
	}
	std::string res( String::format( "%.2f", mValue ) );
	String::replace( res, ",", "." );
	String::numberCleanInPlace( res );
	res += unitToString( mUnit );
	return res;
}

const StyleSheetLength::Arguments& StyleSheetLength::getArgs() const {
	return mArgs;
}

void StyleSheetLength::setArgs( const Arguments& args ) {
	mArgs = args;
}

bool StyleSheetLength::isFunctionString( std::string_view str ) {
	str = String::trim( str );
	return String::istartsWith( str, "clamp(" ) || String::istartsWith( str, "min(" ) ||
		   String::istartsWith( str, "max(" ) || String::istartsWith( str, "calc(" );
}

bool StyleSheetLength::parseFunction( const std::string& str, Unit& outUnit, Arguments& outArgs ) {
	std::string_view sv( str );
	std::string_view trimmed = String::trim( sv );

	if ( String::istartsWith( trimmed, "clamp(" ) ) {
		outUnit = Unit::Clamp;
	} else if ( String::istartsWith( trimmed, "min(" ) ) {
		outUnit = Unit::Min;
	} else if ( String::istartsWith( trimmed, "max(" ) ) {
		outUnit = Unit::Max;
	} else if ( String::istartsWith( trimmed, "calc(" ) ) {
		outUnit = Unit::Calc;
	} else {
		return false;
	}

	size_t parenOpen = trimmed.find( '(' );
	if ( parenOpen == std::string_view::npos )
		return false;

	size_t parenClose = trimmed.find_last_of( ')' );
	if ( parenClose == std::string_view::npos || parenClose <= parenOpen )
		return false;

	std::string_view argsSv = trimmed.substr( parenOpen + 1, parenClose - parenOpen - 1 );

	outArgs.clear();

	if ( outUnit == Unit::Calc ) {
		std::string expr( String::trim( argsSv ) );
		if ( !expr.empty() )
			outArgs.push_back( std::move( expr ) );
		return !outArgs.empty();
	}

	size_t depth = 0;
	size_t start = 0;
	for ( size_t i = 0; i < argsSv.size(); i++ ) {
		char c = argsSv[i];
		if ( c == '(' ) {
			depth++;
		} else if ( c == ')' ) {
			if ( depth > 0 )
				depth--;
		} else if ( c == ',' && depth == 0 ) {
			std::string_view arg = argsSv.substr( start, i - start );
			std::string trimmedArg( String::trim( arg ) );
			if ( !trimmedArg.empty() )
				outArgs.push_back( std::move( trimmedArg ) );
			start = i + 1;
		}
	}
	std::string_view lastArg = argsSv.substr( start );
	std::string trimmedLast( String::trim( lastArg ) );
	if ( !trimmedLast.empty() )
		outArgs.push_back( std::move( trimmedLast ) );

	return !outArgs.empty();
}

Float StyleSheetLength::resolveFunction( const Float& parentSize, const Sizef& viewSize,
										 const Float& displayDpi, const Float& elFontSize,
										 const Float& globalFontSize, Graphics::Font* font ) const {
	if ( mArgs.empty() )
		return 0;

	switch ( mUnit ) {
		case Unit::Clamp: {
			if ( mArgs.size() < 3 )
				return 0;
			StyleSheetLength minVal = fromString( mArgs[0] );
			StyleSheetLength prefVal = fromString( mArgs[1] );
			StyleSheetLength maxVal = fromString( mArgs[2] );
			Float resolvedMin = minVal.asPixels( parentSize, viewSize, displayDpi, elFontSize,
												 globalFontSize, font );
			Float resolvedPref = prefVal.asPixels( parentSize, viewSize, displayDpi, elFontSize,
												   globalFontSize, font );
			Float resolvedMax = maxVal.asPixels( parentSize, viewSize, displayDpi, elFontSize,
												 globalFontSize, font );
			return eemin( eemax( resolvedPref, resolvedMin ), resolvedMax );
		}
		case Unit::Min: {
			Float result = std::numeric_limits<Float>::max();
			for ( size_t i = 0; i < mArgs.size(); i++ ) {
				StyleSheetLength arg = fromString( mArgs[i] );
				Float resolved = arg.asPixels( parentSize, viewSize, displayDpi, elFontSize,
											   globalFontSize, font );
				result = eemin( result, resolved );
			}
			return result;
		}
		case Unit::Max: {
			Float result = -std::numeric_limits<Float>::max();
			for ( size_t i = 0; i < mArgs.size(); i++ ) {
				StyleSheetLength arg = fromString( mArgs[i] );
				Float resolved = arg.asPixels( parentSize, viewSize, displayDpi, elFontSize,
											   globalFontSize, font );
				result = eemax( result, resolved );
			}
			return result;
		}
		default:
			return 0;
	}
}

Float StyleSheetLength::resolveCalc( const Float& parentSize, const Sizef& viewSize,
									 const Float& displayDpi, const Float& elFontSize,
									 const Float& globalFontSize, Graphics::Font* font ) const {
	if ( mArgs.empty() )
		return 0;

	const std::string& expr = mArgs[0];

	enum class TokenType { Number, Unit, Op, ParenOpen, ParenClose };
	struct Token {
		TokenType type;
		std::string val;
	};

	SmallVector<Token, 16> tokens;

	auto isOpChar = []( char c ) {
		return c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')';
	};

	size_t i = 0;
	while ( i < expr.size() ) {
		char c = expr[i];
		if ( c == ' ' || c == '\t' || c == '\n' ) {
			i++;
			continue;
		}
		if ( c == '(' ) {
			tokens.push_back( { TokenType::ParenOpen, std::string( expr.substr( i, 1 ) ) } );
			i++;
		} else if ( c == ')' ) {
			tokens.push_back( { TokenType::ParenClose, std::string( expr.substr( i, 1 ) ) } );
			i++;
		} else if ( c == '+' || c == '*' || c == '/' ) {
			tokens.push_back( { TokenType::Op, std::string( expr.substr( i, 1 ) ) } );
			i++;
		} else if ( c == '-' ) {
			bool isNegNum = false;
			if ( tokens.empty() || ( tokens.back().type == TokenType::Op ||
									 tokens.back().type == TokenType::ParenOpen ) ) {
				size_t j = i + 1;
				while ( j < expr.size() && expr[j] == ' ' )
					j++;
				if ( j < expr.size() && ( String::isNumber( expr[j], false ) || expr[j] == '.' ) )
					isNegNum = true;
			}
			if ( isNegNum ) {
				size_t start = i;
				i++;
				while ( i < expr.size() &&
						( String::isNumber( expr[i], false ) || expr[i] == '.' ) )
					i++;
				tokens.push_back(
					{ TokenType::Number, std::string( expr.substr( start, i - start ) ) } );
				if ( i < expr.size() && !isOpChar( expr[i] ) && expr[i] != ' ' ) {
					size_t unitStart = i;
					while ( i < expr.size() && !isOpChar( expr[i] ) && expr[i] != ' ' &&
							expr[i] != '(' && expr[i] != ')' )
						i++;
					tokens.push_back( { TokenType::Unit,
										std::string( expr.substr( unitStart, i - unitStart ) ) } );
				}
			} else {
				tokens.push_back( { TokenType::Op, std::string( expr.substr( i, 1 ) ) } );
				i++;
			}
		} else if ( String::isNumber( c, false ) || c == '.' ) {
			size_t start = i;
			while ( i < expr.size() && ( String::isNumber( expr[i], false ) || expr[i] == '.' ) )
				i++;
			tokens.push_back(
				{ TokenType::Number, std::string( expr.substr( start, i - start ) ) } );
			if ( i < expr.size() && !isOpChar( expr[i] ) && expr[i] != ' ' ) {
				size_t unitStart = i;
				while ( i < expr.size() && !isOpChar( expr[i] ) && expr[i] != ' ' &&
						expr[i] != '(' && expr[i] != ')' )
					i++;
				tokens.push_back(
					{ TokenType::Unit, std::string( expr.substr( unitStart, i - unitStart ) ) } );
			}
		} else {
			i++;
		}
	}

	if ( tokens.empty() )
		return 0;

	struct Evaluator {
		const SmallVector<Token, 16>& tokens;
		const Float& parentSize;
		const Sizef& viewSize;
		const Float& displayDpi;
		const Float& elFontSize;
		const Float& globalFontSize;
		Graphics::Font* font;
		size_t pos = 0;

		Float resolveAtom( const Token& numTok, const Token* unitTok ) const {
			if ( unitTok && !unitTok->val.empty() ) {
				std::string valStr( numTok.val );
				valStr += unitTok->val;
				StyleSheetLength len = StyleSheetLength::fromString( valStr );
				return len.asPixels( parentSize, viewSize, displayDpi, elFontSize, globalFontSize,
									 font );
			}
			Float v = 0;
			String::fromString( v, numTok.val );
			return v;
		}

		Float evalFactor() {
			if ( pos >= tokens.size() )
				return 0;

			auto& tok = tokens[pos];

			if ( tok.type == TokenType::ParenOpen ) {
				pos++;
				Float result = evalExpr();
				if ( pos < tokens.size() && tokens[pos].type == TokenType::ParenClose )
					pos++;
				return result;
			}

			if ( tok.type == TokenType::Op && tok.val == "-" ) {
				pos++;
				return -evalFactor();
			}
			if ( tok.type == TokenType::Op && tok.val == "+" ) {
				pos++;
				return evalFactor();
			}

			if ( tok.type == TokenType::Number ) {
				const Token* unitTok =
					( pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::Unit )
						? &tokens[pos + 1]
						: nullptr;
				Float v = resolveAtom( tok, unitTok );
				pos += unitTok ? 2 : 1;
				return v;
			}

			pos++;
			return 0;
		}

		Float evalTerm() {
			Float left = evalFactor();
			while ( pos < tokens.size() && tokens[pos].type == TokenType::Op &&
					( tokens[pos].val == "*" || tokens[pos].val == "/" ) ) {
				char op = tokens[pos].val[0];
				pos++;
				Float right = evalFactor();
				if ( op == '*' )
					left = left * right;
				else if ( right != 0 )
					left = left / right;
			}
			return left;
		}

		Float evalExpr() {
			Float left = evalTerm();
			while ( pos < tokens.size() && tokens[pos].type == TokenType::Op &&
					( tokens[pos].val == "+" || tokens[pos].val == "-" ) ) {
				char op = tokens[pos].val[0];
				pos++;
				Float right = evalTerm();
				if ( op == '+' )
					left = left + right;
				else
					left = left - right;
			}
			return left;
		}
	};

	Evaluator ev{ tokens, parentSize, viewSize, displayDpi, elFontSize, globalFontSize, font, 0 };
	return ev.evalExpr();
}
}}} // namespace EE::UI::CSS
