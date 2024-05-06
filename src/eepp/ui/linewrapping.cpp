#include <eepp/graphics/text.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/linewrapping.hpp>

namespace EE { namespace UI {

LineWrapInfo LineWrapping::computeLineBreaks( const String& string,
											  const FontStyleConfig& fontStyle, Float maxWidth,
											  LineWrapMode mode, bool keepIndentation,
											  Uint32 tabWidth ) {
	LineWrapInfo info;
	if ( string.empty() || nullptr == fontStyle.Font )
		return info;

	if ( keepIndentation ) {
		auto nonIndentPos = string.find_first_not_of( " \t\n\v\f\r" );
		if ( nonIndentPos != String::InvalidPos )
			info.paddingStart =
				Text::getTextWidth( string.view().substr( 0, nonIndentPos ), fontStyle, tabWidth );
	}

	Float xoffset = 0.f;
	Float lastWidth = 0.f;
	bool bold = ( fontStyle.Style & Text::Style::Bold ) != 0;
	bool italic = ( fontStyle.Style & Text::Style::Italic ) != 0;
	bool isMonospace = fontStyle.Font->isMonospace();
	Float outlineThickness = fontStyle.OutlineThickness;

	auto tChar = &string[0];
	size_t lastSpace = 0;
	Uint32 prevChar = 0;

	Float hspace = static_cast<Float>(
		fontStyle.Font->getGlyph( L' ', fontStyle.CharacterSize, bold, italic, outlineThickness )
			.advance );
	size_t idx = 0;

	while ( *tChar ) {
		Uint32 curChar = *tChar;
		Float w = !isMonospace ? fontStyle.Font
									 ->getGlyph( curChar, fontStyle.CharacterSize, bold, italic,
												 outlineThickness )
									 .advance
							   : hspace;

		if ( curChar == '\t' )
			w += hspace * tabWidth;
		else if ( ( curChar ) == '\r' )
			w = 0;

		if ( curChar != '\r' && !isMonospace ) {
			w += fontStyle.Font->getKerning( prevChar, curChar, fontStyle.CharacterSize, bold,
											 italic, outlineThickness );
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
		} else if ( curChar == ' ' ) {
			lastSpace = idx;
			lastWidth = xoffset;
		}

		idx++;
		tChar++;
	}

	return info;
}

LineWrapInfo LineWrapping::computeLineBreaks( const TextDocument& doc, size_t line,
											  const FontStyleConfig& fontStyle, Float maxWidth,
											  LineWrapMode mode, bool keepIndentation,
											  Uint32 tabWidth ) {
	if ( nullptr == fontStyle.Font || fontStyle.Font->isMonospace() ||
		 nullptr == doc.getHighlighter() || doc.getSyntaxDefinition().getPatterns().empty() ) {
		return computeLineBreaks( doc.line( line ).getText(), fontStyle, maxWidth, mode,
								  keepIndentation, tabWidth );
	}

	LineWrapInfo info;
	const auto& string = doc.line( line ).getText();
	const auto& tokens = doc.getHighlighter()->getLine( line );

	Float xoffset = 0.f;
	Float lastWidth = 0.f;
	bool bold = ( fontStyle.Style & Text::Style::Bold ) != 0;
	bool italic = ( fontStyle.Style & Text::Style::Italic ) != 0;
	Float outlineThickness = fontStyle.OutlineThickness;

	size_t lastSpace = 0;
	Uint32 prevChar = 0;

	Float hspace = static_cast<Float>(
		fontStyle.Font->getGlyph( L' ', fontStyle.CharacterSize, bold, italic, outlineThickness )
			.advance );
	size_t idx = 0;

	for ( const auto& token : tokens ) {
		const auto text = string.view().substr( token.pos, token.len );

		if ( idx == 0 && keepIndentation ) {
			auto nonIndentPos = string.find_first_not_of( " \t\n\v\f\r" );
			if ( nonIndentPos != String::InvalidPos )
				info.paddingStart = Text::getTextWidth( string.view().substr( 0, nonIndentPos ),
														fontStyle, tabWidth );
		}

		Float w = Text::getTextWidth( text, fontStyle, tabWidth );

		if ( xoffset + w > maxWidth ) {
			auto tChar = &text[0];

			while ( *tChar ) {
				Uint32 curChar = *tChar;
				Float w = fontStyle.Font
							  ->getGlyph( curChar, fontStyle.CharacterSize, bold, italic,
										  outlineThickness )
							  .advance;

				if ( curChar == '\t' )
					w += hspace * tabWidth;
				else if ( ( curChar ) == '\r' )
					w = 0;

				if ( curChar != '\r' ) {
					w += fontStyle.Font->getKerning( prevChar, curChar, fontStyle.CharacterSize,
													 bold, italic, outlineThickness );
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
				} else if ( curChar == ' ' ) {
					lastSpace = idx;
					lastWidth = xoffset;
				}

				idx++;
				tChar++;
			}
		} else {
			xoffset += w;
			idx += text.size();
		}
	}

	return info;
}

}} // namespace EE::UI
