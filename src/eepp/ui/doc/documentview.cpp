#include <eepp/graphics/text.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/documentview.hpp>

namespace EE { namespace UI { namespace Doc {

LineWrapMode DocumentView::toLineWrapMode( std::string mode ) {
	String::toLowerInPlace( mode );
	if ( mode == "word" )
		return LineWrapMode::Word;
	if ( mode == "letter" )
		return LineWrapMode::Letter;
	return LineWrapMode::NoWrap;
}

std::string DocumentView::fromLineWrapMode( LineWrapMode mode ) {
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

LineWrapType DocumentView::toLineWrapType( std::string type ) {
	String::toLowerInPlace( type );
	if ( "line_breaking_column" == type )
		return LineWrapType::LineBreakingColumn;
	return LineWrapType::Viewport;
}

std::string DocumentView::fromLineWrapType( LineWrapType type ) {
	switch ( type ) {
		case LineWrapType::LineBreakingColumn:
			return "line_breaking_column";
		case LineWrapType::Viewport:
		default:
			return "viewport";
	}
}

Float DocumentView::computeOffsets( const String& string, const FontStyleConfig& fontStyle,
									Uint32 tabWidth ) {

	auto nonIndentPos = string.find_first_not_of( " \t\n\v\f\r" );
	if ( nonIndentPos != String::InvalidPos )
		return Text::getTextWidth( string.view().substr( 0, nonIndentPos ), fontStyle, tabWidth );
	return 0.f;
}

DocumentView::LineWrapInfo DocumentView::computeLineBreaks( const String::View& string,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	LineWrapInfo info;
	info.wraps.push_back( 0 );
	if ( string.empty() || nullptr == fontStyle.Font || mode == LineWrapMode::NoWrap )
		return info;

	if ( keepIndentation ) {
		static const String nofOf = " \t\n\v\f\r";
		auto nonIndentPos = string.find_first_not_of( nofOf.data() );
		if ( nonIndentPos != String::InvalidPos )
			info.paddingStart =
				Text::getTextWidth( string.substr( 0, nonIndentPos ), fontStyle, tabWidth );
	}

	Float xoffset = 0.f;
	Float lastWidth = 0.f;
	bool bold = ( fontStyle.Style & Text::Style::Bold ) != 0;
	bool italic = ( fontStyle.Style & Text::Style::Italic ) != 0;
	bool isMonospace = fontStyle.Font->isMonospace();
	Float outlineThickness = fontStyle.OutlineThickness;

	size_t lastSpace = 0;
	Uint32 prevChar = 0;

	Float hspace = static_cast<Float>(
		fontStyle.Font->getGlyph( L' ', fontStyle.CharacterSize, bold, italic, outlineThickness )
			.advance );
	size_t idx = 0;

	for ( const auto& curChar : string ) {
		Float w = !isMonospace ? fontStyle.Font
									 ->getGlyph( curChar, fontStyle.CharacterSize, bold, italic,
												 outlineThickness )
									 .advance
							   : hspace;

		if ( curChar == '\t' )
			w = hspace * tabWidth;
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
		} else if ( curChar == ' ' || curChar == '.' || curChar == '-' || curChar == ',' ) {
			lastSpace = idx;
			lastWidth = xoffset;
		}

		idx++;
	}

	return info;
}

DocumentView::LineWrapInfo DocumentView::computeLineBreaks( const String& string,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	return computeLineBreaks( string.view(), fontStyle, maxWidth, mode, keepIndentation, tabWidth );
}

DocumentView::LineWrapInfo DocumentView::computeLineBreaks( const TextDocument& doc, size_t line,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	const auto& text = doc.line( line ).getText();
	return computeLineBreaks( text.substr( 0, text.size() - 1 ), fontStyle, maxWidth, mode,
							  keepIndentation, tabWidth );
}

DocumentView::DocumentView( std::shared_ptr<TextDocument> doc, FontStyleConfig fontStyle,
							Config config ) :
	mDoc( std::move( doc ) ),
	mFontStyle( std::move( fontStyle ) ),
	mConfig( std::move( config ) ) {}

bool DocumentView::isWrapEnabled() const {
	return mConfig.mode != LineWrapMode::NoWrap;
}

void DocumentView::setMaxWidth( Float maxWidth, bool forceReconstructBreaks ) {
	if ( maxWidth != mMaxWidth ) {
		mMaxWidth = maxWidth;
		invalidateCache();
	} else if ( forceReconstructBreaks || mPendingReconstruction ) {
		invalidateCache();
	}
}

void DocumentView::setFontStyle( FontStyleConfig fontStyle ) {
	if ( fontStyle != mFontStyle ) {
		mFontStyle = std::move( fontStyle );
		invalidateCache();
	}
}

void DocumentView::setLineWrapMode( LineWrapMode mode ) {
	if ( mode != mConfig.mode ) {
		mConfig.mode = mode;
		invalidateCache();
	}
}

TextPosition DocumentView::getVisibleIndexPosition( VisibleIndex visibleIndex ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLines.empty() )
		return { static_cast<Int64>( visibleIndex ), 0 };
	return mWrappedLines[eeclamp( static_cast<Int64>( visibleIndex ), 0ll,
								  eemax( static_cast<Int64>( mWrappedLines.size() ) - 1, 0ll ) )];
}

Float DocumentView::getLinePadding( Int64 docIdx ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLinesOffset.empty() )
		return 0;
	return mWrappedLinesOffset[eeclamp(
		docIdx, 0ll, eemax( static_cast<Int64>( mWrappedLinesOffset.size() ) - 1, 0ll ) )];
}

void DocumentView::setConfig( Config config ) {
	if ( config != mConfig ) {
		mConfig = std::move( config );
		invalidateCache();
	}
}

void DocumentView::invalidateCache() {
	if ( 0 == mMaxWidth || !mDoc )
		return;

	if ( mDoc->isLoading() ) {
		mPendingReconstruction = mConfig.mode != LineWrapMode::NoWrap;
		return;
	}

	BoolScopedOp op( mUnderConstruction, true );

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

	mPendingReconstruction = false;
}

VisibleIndex DocumentView::toVisibleIndex( Int64 docIdx, bool retLast ) const {
	eeASSERT( isLineVisible( docIdx ) );
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLineToIndex.empty() )
		return static_cast<VisibleIndex>( docIdx );
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
	return static_cast<VisibleIndex>( idx );
}

bool DocumentView::isWrappedLine( Int64 docIdx ) const {
	if ( isWrapEnabled() && mConfig.mode != LineWrapMode::NoWrap ) {
		Int64 wrappedIndex = static_cast<Int64>( toVisibleIndex( docIdx ) );
		return wrappedIndex + 1 < static_cast<Int64>( mWrappedLines.size() ) &&
			   mWrappedLines[wrappedIndex].line() == mWrappedLines[wrappedIndex + 1].line();
	}
	return false;
}

DocumentView::VisibleLineInfo DocumentView::getVisibleLineInfo( Int64 docIdx ) const {
	eeASSERT( isLineVisible( docIdx ) );
	VisibleLineInfo line;
	if ( mConfig.mode == LineWrapMode::NoWrap ) {
		line.visualLines.push_back( { docIdx, 0 } );
		line.visibleIndex = static_cast<VisibleIndex>( docIdx );
		return line;
	}
	Int64 fromIdx = static_cast<Int64>( toVisibleIndex( docIdx ) );
	Int64 toIdx = static_cast<Int64>( toVisibleIndex( docIdx, true ) );
	line.visualLines.reserve( toIdx - fromIdx + 1 );
	for ( Int64 i = fromIdx; i <= toIdx; i++ )
		line.visualLines.emplace_back( mWrappedLines[i] );
	line.visibleIndex = static_cast<VisibleIndex>( fromIdx );
	line.paddingStart = mWrappedLinesOffset[docIdx];
	return line;
}

DocumentView::VisibleLineRange DocumentView::getVisibleLineRange( const TextPosition& pos,
																  bool allowVisualLineEnd ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap ) {
		DocumentView::VisibleLineRange info;
		info.visibleIndex = static_cast<VisibleIndex>( pos.line() );
		info.range = mDoc->getLineRange( pos.line() );
		return info;
	}
	Int64 fromIdx = static_cast<Int64>( toVisibleIndex( pos.line() ) );
	Int64 toIdx = static_cast<Int64>( toVisibleIndex( pos.line(), true ) );
	DocumentView::VisibleLineRange info;
	for ( Int64 i = fromIdx; i < toIdx; i++ ) {
		Int64 fromCol = mWrappedLines[i].column();
		Int64 toCol = i + 1 <= toIdx
						  ? mWrappedLines[i + 1].column() - ( allowVisualLineEnd ? 0 : 1 )
						  : mDoc->line( pos.line() ).size();
		if ( pos.column() >= fromCol && pos.column() <= toCol ) {
			info.visibleIndex = static_cast<VisibleIndex>( i );
			info.range = { { pos.line(), fromCol }, { pos.line(), toCol } };
			return info;
		}
	}
	eeASSERT( toIdx >= 0 );
	info.visibleIndex = static_cast<VisibleIndex>( toIdx );
	info.range = { { pos.line(), mWrappedLines[toIdx].column() },
				   mDoc->endOfLine( { pos.line(), 0ll } ) };
	return info;
}

TextRange DocumentView::getVisibleIndexRange( VisibleIndex visibleIndex ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return mDoc->getLineRange( static_cast<Int64>( visibleIndex ) );
	Int64 idx = static_cast<Int64>( visibleIndex );
	auto start = getVisibleIndexPosition( visibleIndex );
	auto end = start;
	if ( idx + 1 < static_cast<Int64>( mWrappedLines.size() ) &&
		 mWrappedLines[idx + 1].line() == start.line() ) {
		end.setColumn( mWrappedLines[idx + 1].column() );
	} else {
		end.setColumn( mDoc->line( start.line() ).size() );
	}
	return { start, end };
}

std::shared_ptr<TextDocument> DocumentView::getDocument() const {
	return mDoc;
}

void DocumentView::setDocument( const std::shared_ptr<TextDocument>& doc ) {
	if ( mDoc != doc ) {
		mDoc = doc;
		invalidateCache();
	}
}

bool DocumentView::isPendingReconstruction() const {
	return mPendingReconstruction;
}

void DocumentView::setPendingReconstruction( bool pendingReconstruction ) {
	mPendingReconstruction = pendingReconstruction;
}

void DocumentView::clear() {
	mWrappedLines.clear();
	mWrappedLineToIndex.clear();
	mWrappedLinesOffset.clear();
}

Float DocumentView::getLineYOffset( VisibleIndex visibleIndex, Float lineHeight ) const {
	return static_cast<Float>( visibleIndex ) * lineHeight;
}

Float DocumentView::getLineYOffset( Int64 docIdx, Float lineHeight ) const {
	return static_cast<Float>( toVisibleIndex( docIdx ) ) * lineHeight;
}

bool DocumentView::isLineVisible( Int64 ) const {
	return true;
}

void DocumentView::updateCache( Int64 fromLine, Int64 toLine, Int64 numLines ) {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return;
	// Get affected wrapped range
	Int64 oldIdxFrom = static_cast<Int64>( toVisibleIndex( fromLine, false ) );
	Int64 oldIdxTo = static_cast<Int64>( toVisibleIndex( toLine, true ) );

	// Remove old wrapped lines
	mWrappedLines.erase( mWrappedLines.begin() + oldIdxFrom, mWrappedLines.begin() + oldIdxTo + 1 );

	// Remove old offsets
	mWrappedLinesOffset.erase( mWrappedLinesOffset.begin() + fromLine,
							   mWrappedLinesOffset.begin() + toLine + 1 );

	// Shift the line numbers
	if ( numLines != 0 ) {
		Int64 wrappedLines = mWrappedLines.size();
		for ( Int64 i = oldIdxFrom; i < wrappedLines; i++ )
			mWrappedLines[i].setLine( mWrappedLines[i].line() + numLines );
	}

	// Recompute line breaks
	auto netLines = toLine + numLines;
	auto idxOffset = oldIdxFrom;
	for ( auto i = fromLine; i <= netLines; i++ ) {
		auto lb = computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
									 mConfig.keepIndentation, mConfig.tabWidth );
		mWrappedLinesOffset.insert( mWrappedLinesOffset.begin() + i, lb.paddingStart );
		for ( const auto& col : lb.wraps ) {
			mWrappedLines.insert( mWrappedLines.begin() + idxOffset, { i, col } );
			idxOffset++;
		}
	}

	// Recompute wrapped line to index
	Int64 line = fromLine;
	Int64 wrappedLinesCount = mWrappedLines.size();
	mWrappedLineToIndex.resize( mDoc->linesCount() );
	for ( Int64 wrappedIdx = oldIdxFrom; wrappedIdx < wrappedLinesCount; wrappedIdx++ ) {
		if ( mWrappedLines[wrappedIdx].column() == 0 ) {
			mWrappedLineToIndex[line] = wrappedIdx;
			line++;
		}
	}
	mWrappedLineToIndex.resize( mDoc->linesCount() );

#ifdef EE_DEBUG
	if ( mConfig.keepIndentation ) {
		auto wrappedOffset = mWrappedLinesOffset;

		for ( auto i = fromLine; i <= toLine; i++ ) {
			mWrappedLinesOffset[i] =
				computeOffsets( mDoc->line( i ).getText(), mFontStyle, mConfig.tabWidth );
		}

		eeASSERT( wrappedOffset == mWrappedLinesOffset );
	}

	auto wrappedLines = mWrappedLines;
	auto wrappedLinesToIndex = mWrappedLineToIndex;
	auto wrappedOffset = mWrappedLinesOffset;

	invalidateCache();

	eeASSERT( wrappedLines == mWrappedLines );
	eeASSERT( wrappedLinesToIndex == mWrappedLineToIndex );
	eeASSERT( wrappedOffset == mWrappedLinesOffset );
#endif
}

size_t DocumentView::getVisibleLinesCount() const {
	return mConfig.mode == LineWrapMode::NoWrap ? mDoc->linesCount() : mWrappedLines.size();
}

}}} // namespace EE::UI::Doc
