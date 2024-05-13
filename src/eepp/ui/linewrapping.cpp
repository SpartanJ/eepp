#include <eepp/graphics/text.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/linewrapping.hpp>

namespace EE { namespace UI {

LineWrapping::LineWrapInfo LineWrapping::computeLineBreaks( const String& string,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	LineWrapInfo info;
	info.wraps.push_back( 0 );
	if ( string.empty() || nullptr == fontStyle.Font || mode == LineWrapMode::NoWrap )
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

		if ( !isMonospace && curChar != '\r' ) {
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

LineWrapping::LineWrapInfo LineWrapping::computeLineBreaks( const TextDocument& doc, size_t line,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	if ( nullptr == fontStyle.Font || mode == LineWrapMode::NoWrap ||
		 nullptr == doc.getHighlighter() || doc.getSyntaxDefinition().getPatterns().empty() ) {
		return computeLineBreaks( doc.line( line ).getText(), fontStyle, maxWidth, mode,
								  keepIndentation, tabWidth );
	}

	LineWrapInfo info;
	info.wraps.push_back( 0 );
	const auto& string = doc.line( line ).getText();
	const auto tokens = doc.getHighlighter()->getLine( line );

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
			for ( const auto& curChar : text ) {
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
			}
		} else {
			xoffset += w;
			idx += text.size();
		}
	}

	return info;
}

LineWrapping::LineWrapping( std::shared_ptr<TextDocument> doc, FontStyleConfig fontStyle,
							Config config ) :
	mDoc( std::move( doc ) ),
	mFontStyle( std::move( fontStyle ) ),
	mConfig( std::move( config ) ) {}

bool LineWrapping::isWrapEnabled() const {
	return mConfig.mode != LineWrapMode::NoWrap;
}

void LineWrapping::setMaxWidth( Float maxWidth ) {
	if ( maxWidth != mMaxWidth ) {
		mMaxWidth = maxWidth;
		reconstructBreaks();
	}
}

void LineWrapping::setFontStyle( FontStyleConfig fontStyle ) {
	if ( fontStyle != mFontStyle ) {
		mFontStyle = std::move( fontStyle );
		reconstructBreaks();
	}
}

void LineWrapping::setLineWrapMode( LineWrapMode mode ) {
	if ( mode != mConfig.mode ) {
		mConfig.mode = mode;
		reconstructBreaks();
	}
}

TextPosition LineWrapping::getDocumentLine( Int64 visibleIndex ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return { visibleIndex, 0 };
	return mWrappedLines[eeclamp( visibleIndex, 0ll,
								  static_cast<Int64>( mWrappedLines.size() - 1 ) )];
}

Float LineWrapping::getLineOffset( Int64 docIdx ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return 0;
	return mWrappedLinesOffset[eeclamp( docIdx, 0ll,
										static_cast<Int64>( mWrappedLinesOffset.size() - 1 ) )];
}

void LineWrapping::setConfig( Config config ) {
	if ( config != mConfig ) {
		mConfig = std::move( config );
		reconstructBreaks();
	}
}

void LineWrapping::reconstructBreaks() {
	if ( 0 == mMaxWidth )
		return;

	mWrappedLines.clear();
	mWrappedLineToIndex.clear();
	mWrappedLinesOffset.clear();

	if ( mConfig.mode == LineWrapMode::NoWrap )
		return;

	Int64 linesCount = mDoc->linesCount();
	mWrappedLines.reserve( linesCount );
	mWrappedLinesOffset.reserve( linesCount );

	for ( auto i = 0; i < linesCount; i++ ) {
		auto lb = computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
									 mConfig.keepIndentation, mConfig.tabWidth );
		mWrappedLinesOffset.emplace_back( lb.paddingStart );
		for ( const auto& col : lb.wraps )
			mWrappedLines.emplace_back( i, col );
	}

	std::optional<Int64> lastWrap;
	size_t i = 0;
	mWrappedLineToIndex.reserve( linesCount );

	for ( const auto& wl : mWrappedLines ) {
		if ( !lastWrap || *lastWrap != wl.line() ) {
			mWrappedLineToIndex.emplace_back( i );
			lastWrap = wl.line();
		}
		i++;
	}
}

Int64 LineWrapping::toWrappedIndex( Int64 docIdx, bool retLast ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLineToIndex.empty() )
		return docIdx;
	auto idx = mWrappedLineToIndex[eeclamp( docIdx, 0ll,
											static_cast<Int64>( mWrappedLineToIndex.size() - 1 ) )];
	if ( retLast ) {
		Int64 lastOfLine = mWrappedLines[idx].line();
		Int64 wrappedCount = mWrappedLines.size();
		for ( auto i = idx + 1; i < wrappedCount; i++ ) {
			if ( mWrappedLines[i].line() == lastOfLine )
				idx = i;
			else
				break;
		}
	}
	return idx;
}

bool LineWrapping::isWrappedLine( Int64 docIdx ) const {
	return isWrapEnabled() && toWrappedIndex( docIdx ) != toWrappedIndex( docIdx, true );
}

LineWrapping::VisualLine LineWrapping::getVisualLine( Int64 docIdx ) const {
	VisualLine line;
	if ( mConfig.mode == LineWrapMode::NoWrap ) {
		line.visualLines.push_back( { docIdx, 0 } );
		line.visualIndex = docIdx;
		return line;
	}
	Int64 fromIdx = toWrappedIndex( docIdx );
	Int64 toIdx = toWrappedIndex( docIdx, true );
	line.visualLines.reserve( toIdx - fromIdx + 1 );
	for ( Int64 i = fromIdx; i <= toIdx; i++ )
		line.visualLines.emplace_back( mWrappedLines[i] );
	line.visualIndex = fromIdx;
	line.paddingStart = mWrappedLinesOffset[docIdx];
	return line;
}

LineWrapping::VisualLineInfo LineWrapping::getVisualLineInfo( const TextPosition& pos ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap ) {
		LineWrapping::VisualLineInfo info;
		info.visualIndex = pos.line();
		info.range = mDoc->getLineRange( pos.line() );
		return info;
	}
	Int64 fromIdx = toWrappedIndex( pos.line() );
	Int64 toIdx = toWrappedIndex( pos.line(), true );
	LineWrapping::VisualLineInfo info;
	for ( Int64 i = fromIdx; i < toIdx; i++ ) {
		Int64 fromCol = mWrappedLines[i].column();
		Int64 toCol =
			i + 1 <= toIdx ? mWrappedLines[i + 1].column() - 1 : mDoc->line( pos.line() ).size();
		if ( pos.column() >= fromCol && pos.column() <= toCol ) {
			info.visualIndex = i;
			info.range = { { pos.line(), fromCol }, { pos.line(), toCol } };
			return info;
		}
	}
	eeASSERT( toIdx - 1 >= 0 );
	info.visualIndex = toIdx;
	info.range = { { pos.line(), mWrappedLines[toIdx].column() },
				   mDoc->endOfLine( { pos.line(), 0ll } ) };
	return info;
}

TextRange LineWrapping::getVisualLineRange( Int64 visualLine ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return mDoc->getLineRange( visualLine );
	auto start = getDocumentLine( visualLine );
	auto end = start;
	if ( visualLine + 1 < static_cast<Int64>( mWrappedLines.size() ) ) {
		end.setColumn( mWrappedLines[visualLine + 1].column() );
	} else {
		end.setColumn( mDoc->line( start.line() ).size() );
	}
	return { start, end };
}

void LineWrapping::updateBreaks( Int64 fromLine, Int64 toLine, Int64 numLines ) {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return;
	// Get affected wrapped range
	Int64 oldIdxFrom = toWrappedIndex( fromLine, false );
	Int64 oldIdxTo = toWrappedIndex( toLine, true );

	// Remove old wrapped lines
	mWrappedLines.erase( mWrappedLines.begin() + oldIdxFrom, mWrappedLines.begin() + oldIdxTo );

	// Remove old offsets
	mWrappedLinesOffset.erase( mWrappedLinesOffset.begin() + fromLine,
							   mWrappedLinesOffset.begin() + toLine );

	// Shift the line numbers
	if ( numLines != 0 ) {
		Int64 wrappedLines = mWrappedLines.size();
		for ( Int64 i = oldIdxFrom; i < wrappedLines; i++ )
			mWrappedLines[i].setLine( mWrappedLines[i].line() + numLines );
	}

	// Recompute line breaks
	for ( auto i = fromLine; i <= toLine; i++ ) {
		auto lb = computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
									 mConfig.keepIndentation, mConfig.tabWidth );
		mWrappedLinesOffset.insert( mWrappedLinesOffset.begin() + i, lb.paddingStart );
		Int64 loffset = 0;
		for ( const auto& col : lb.wraps ) {
			mWrappedLines.insert( mWrappedLines.begin() + i + loffset, { i, col } );
			loffset++;
		}
	}

	// Recompute wrapped line to index
	Int64 line = fromLine;
	Int64 wrappedLines = mWrappedLines.size();
	for ( Int64 wrappedIdx = oldIdxFrom; wrappedIdx < wrappedLines; wrappedIdx++ ) {
		if ( mWrappedLines[wrappedIdx].column() == 0 ) {
			mWrappedLineToIndex[line] = wrappedIdx;
			line++;
		}
	}
	mWrappedLineToIndex.resize( mDoc->linesCount() );
}

size_t LineWrapping::getTotalLines() const {
	return mConfig.mode == LineWrapMode::NoWrap ? mDoc->linesCount() : mWrappedLines.size();
}

}} // namespace EE::UI
