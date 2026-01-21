#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/linewrap.hpp>
#include <eepp/graphics/text.hpp>

namespace EE::Graphics {

LineWrapMode LineWrap::toLineWrapMode( std::string mode ) {
	String::toLowerInPlace( mode );
	if ( mode == "word" )
		return LineWrapMode::Word;
	if ( mode == "letter" )
		return LineWrapMode::Letter;
	return LineWrapMode::NoWrap;
}

std::string LineWrap::fromLineWrapMode( LineWrapMode mode ) {
	switch ( mode ) {
		case LineWrapMode::Letter:
			return "letter";
		case LineWrapMode::Word:
			return "word";
		case LineWrapMode::NoWrap:
		default:
			return "nowrap";
	}
}

LineWrapType LineWrap::toLineWrapType( std::string type ) {
	String::toLowerInPlace( type );
	if ( "line_breaking_column" == type )
		return LineWrapType::LineBreakingColumn;
	return LineWrapType::Viewport;
}

std::string LineWrap::fromLineWrapType( LineWrapType type ) {
	switch ( type ) {
		case LineWrapType::LineBreakingColumn:
			return "line_breaking_column";
		case LineWrapType::Viewport:
		default:
			return "viewport";
	}
}

Float LineWrap::computeOffsets( const String::View& string, const FontStyleConfig& fontStyle,
								Uint32 tabWidth, Float maxWidth, bool tabStops ) {
	static const String sepSpaces = " \t\n\v\f\r";
	auto nonIndentPos = string.find_first_not_of( sepSpaces.data() );
	if ( nonIndentPos != String::View::npos ) {
		Float w = Text::getTextWidth( string.substr( 0, nonIndentPos ), fontStyle, tabWidth,
									  TextHints::AllAscii, TextDirection::LeftToRight,
									  tabStops ? 0 : std::optional<Float>{} );
		return maxWidth != 0.f ? ( w > maxWidth ? 0.f : w ) : w;
	}
	return 0.f;
}

LineWrapInfo LineWrap::computeLineBreaks( const String::View& string,
										  const FontStyleConfig& fontStyle, Float maxWidth,
										  LineWrapMode mode, bool keepIndentation, Uint32 tabWidth,
										  Float whiteSpaceWidth, Uint32 textDrawHints,
										  bool tabStops, Float initialXOffset ) {
	LineWrapInfo info;
	info.wraps.push_back( 0 );
	if ( string.empty() || nullptr == fontStyle.Font || mode == LineWrapMode::NoWrap )
		return info;

	bool bold = ( fontStyle.Style & Text::Style::Bold ) != 0;
	bool italic = ( fontStyle.Style & Text::Style::Italic ) != 0;
	Float outlineThickness = fontStyle.OutlineThickness;
	Float hspace = whiteSpaceWidth == 0.f ? fontStyle.Font
												->getGlyph( L' ', fontStyle.CharacterSize, bold,
															italic, outlineThickness )
												.advance
										  : whiteSpaceWidth;

	if ( keepIndentation ) {
		info.paddingStart = computeOffsets( string, fontStyle, tabWidth,
											eemax( maxWidth - hspace, hspace ), tabStops );
	}

	Float xoffset = initialXOffset;
	Float lastWidth = 0.f;
	bool isMonospace =
		fontStyle.Font &&
		( fontStyle.Font->isMonospace() ||
		  ( fontStyle.Font->getType() == FontType::TTF &&
			static_cast<FontTrueType*>( fontStyle.Font )->isIdentifiedAsMonospace() &&
			Text::canSkipShaping( textDrawHints ) ) );

	size_t lastSpace = 0;
	Uint32 prevChar = 0;
	size_t idx = 0;

	for ( const auto& curChar : string ) {
		Float w = !isMonospace ? fontStyle.Font
									 ->getGlyph( curChar, fontStyle.CharacterSize, bold, italic,
												 outlineThickness )
									 .advance
							   : hspace;

		if ( curChar == '\t' )
			w = Text::tabAdvance( hspace, tabWidth, tabStops ? xoffset : std::optional<Float>{} );

		if ( !isMonospace && curChar != '\r' ) {
			if ( !( textDrawHints & TextHints::NoKerning ) ) {
				w += fontStyle.Font->getKerning( prevChar, curChar, fontStyle.CharacterSize, bold,
												 italic, outlineThickness );
			}
			prevChar = curChar;
		}

		xoffset += w;

		if ( xoffset > maxWidth ) {
			if ( mode == LineWrapMode::Word && lastSpace ) {
				info.wraps.push_back( lastSpace + 1 );
				xoffset = w + info.paddingStart + ( xoffset - lastWidth );
			} else {
				info.wraps.push_back( idx );
				xoffset = w + info.paddingStart;
			}
			lastSpace = 0;
		} else if ( curChar == ' ' || curChar == '.' || curChar == '-' || curChar == ',' ) {
			lastSpace = idx;
			lastWidth = xoffset;
		}

		idx++;
	}

	return info;
}

LineWrapInfo LineWrap::computeLineBreaks( const String& string, const FontStyleConfig& fontStyle,
										  Float maxWidth, LineWrapMode mode, bool keepIndentation,
										  Uint32 tabWidth, Float whiteSpaceWidth, Uint32 textHints,
										  bool tabStops, Float initialXOffset ) {
	return computeLineBreaks( string.view(), fontStyle, maxWidth, mode, keepIndentation, tabWidth,
							  whiteSpaceWidth, textHints, tabStops, initialXOffset );
}

} // namespace EE::Graphics
