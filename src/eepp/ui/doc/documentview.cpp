#include <eepp/graphics/text.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/documentview.hpp>

#if 0
#define EE_VERIFY_STRUCTURAL_CONSISTENCY
#endif

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

Float DocumentView::computeOffsets( const String::View& string, const FontStyleConfig& fontStyle,
									Uint32 tabWidth, Float maxWidth ) {

	static const String sepSpaces = " \t\n\v\f\r";
	auto nonIndentPos = string.find_first_not_of( sepSpaces.data() );
	if ( nonIndentPos != String::View::npos ) {
		Float w = Text::getTextWidth( string.substr( 0, nonIndentPos ), fontStyle, tabWidth );
		return maxWidth != 0.f ? ( w > maxWidth ? 0.f : w ) : w;
	}
	return 0.f;
}

DocumentView::LineWrapInfo DocumentView::computeLineBreaks( const String::View& string,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation, Uint32 tabWidth,
															Float whiteSpaceWidth ) {
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
		info.paddingStart =
			computeOffsets( string, fontStyle, tabWidth, eemax( maxWidth - hspace, hspace ) );
	}

	Float xoffset = 0.f;
	Float lastWidth = 0.f;
	bool isMonospace = fontStyle.Font->isMonospace();

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
			w = hspace * tabWidth;

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
															bool keepIndentation, Uint32 tabWidth,
															Float whiteSpaceWidth ) {
	return computeLineBreaks( string.view(), fontStyle, maxWidth, mode, keepIndentation, tabWidth,
							  whiteSpaceWidth );
}

DocumentView::LineWrapInfo DocumentView::computeLineBreaks( const TextDocument& doc, size_t line,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation, Uint32 tabWidth,
															Float whiteSpaceWidth ) {
	const auto& text = doc.line( line ).getText();
	return computeLineBreaks( text.view().substr( 0, text.size() - 1 ), fontStyle, maxWidth, mode,
							  keepIndentation, tabWidth, whiteSpaceWidth );
}

DocumentView::DocumentView( std::shared_ptr<TextDocument> doc, FontStyleConfig fontStyle,
							Config config ) :
	mDoc( std::move( doc ) ), mFontStyle( std::move( fontStyle ) ), mConfig( std::move( config ) ) {
	invalidateCache();
}

bool DocumentView::isWrapEnabled() const {
	return mConfig.mode != LineWrapMode::NoWrap;
}

void DocumentView::setMaxWidth( Float maxWidth, bool forceReconstructBreaks ) {
	if ( maxWidth != mMaxWidth ) {
		mMaxWidth = maxWidth;
		if ( !isOneToOne() )
			invalidateCache();
	} else if ( forceReconstructBreaks || mPendingReconstruction ) {
		invalidateCache();
	}
}

void DocumentView::setFontStyle( FontStyleConfig fontStyle ) {
	if ( fontStyle != mFontStyle ) {
		mFontStyle = std::move( fontStyle );

		mWhiteSpaceWidth = fontStyle.Font
							   ? fontStyle.Font
									 ->getGlyph( L' ', fontStyle.CharacterSize,
												 ( fontStyle.Style & Text::Style::Bold ) != 0,
												 ( fontStyle.Style & Text::Style::Italic ),
												 fontStyle.OutlineThickness )
									 .advance
							   : 0.f;

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
	eeASSERT( mConfig.mode == LineWrapMode::NoWrap || !mVisibleLines.empty() );
	if ( isOneToOne() || mVisibleLines.empty() )
		return { static_cast<Int64>( visibleIndex ), 0 };
	return mVisibleLines[eeclamp( static_cast<Int64>( visibleIndex ), 0ll,
								  eemax( static_cast<Int64>( mVisibleLines.size() ) - 1, 0ll ) )];
}

Float DocumentView::getLinePadding( Int64 docIdx ) const {
	if ( isOneToOne() || mVisibleLinesOffset.empty() )
		return 0;
	return mVisibleLinesOffset[eeclamp(
		docIdx, 0ll, eemax( static_cast<Int64>( mVisibleLinesOffset.size() ) - 1, 0ll ) )];
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
		mPendingReconstruction = !isOneToOne();
		return;
	}

	Clock clock;
	BoolScopedOp op( mUnderConstruction, true );

	mVisibleLines.clear();
	mDocLineToVisibleIndex.clear();
	mVisibleLinesOffset.clear();

	bool wrap = mConfig.mode != LineWrapMode::NoWrap;
	Int64 linesCount = mDoc->linesCount();
	mVisibleLines.reserve( linesCount );
	mVisibleLinesOffset.reserve( linesCount );
	mDocLineToVisibleIndex.reserve( linesCount );

	for ( auto i = 0; i < linesCount; i++ ) {
		if ( isFolded( i, true ) ) {
			mVisibleLinesOffset.emplace_back(
				wrap ? computeOffsets( mDoc->line( i ).getText().view(), mFontStyle,
									   mConfig.tabWidth,
									   eemax( mMaxWidth - mWhiteSpaceWidth, mWhiteSpaceWidth ) )
					 : 0 );
			mDocLineToVisibleIndex.push_back( static_cast<Int64>( VisibleIndex::invalid ) );
		} else {
			auto lb = wrap ? computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
												mConfig.keepIndentation, mConfig.tabWidth,
												mWhiteSpaceWidth )
						   : LineWrapInfo{ { 0 }, 0.f };
			mVisibleLinesOffset.emplace_back( lb.paddingStart );
			bool first = true;
			for ( const auto& col : lb.wraps ) {
				mVisibleLines.emplace_back( i, col );
				if ( first ) {
					mDocLineToVisibleIndex.emplace_back( mVisibleLines.size() - 1 );
					first = false;
				}
			}
		}
	}

	eeASSERT( static_cast<Int64>( mDocLineToVisibleIndex.size() ) == linesCount );

	mPendingReconstruction = false;

	Log::debug( "DocumentView for \"%s\" generated in %s", mDoc->getFilePath(),
				clock.getElapsedTime().toString() );
}

VisibleIndex DocumentView::toVisibleIndex( Int64 docIdx, bool retLast ) const {
	// eeASSERT( isLineVisible( docIdx ) );
	if ( isOneToOne() || mDocLineToVisibleIndex.empty() )
		return static_cast<VisibleIndex>( docIdx );
	auto idx = mDocLineToVisibleIndex[eeclamp(
		docIdx, 0ll, static_cast<Int64>( mDocLineToVisibleIndex.size() - 1 ) )];
	if ( retLast && idx != static_cast<Int64>( VisibleIndex::invalid ) ) {
		Int64 lastOfLine = mVisibleLines[idx].line();
		Int64 visibleLinesCount = mVisibleLines.size();
		for ( auto i = idx + 1; i < visibleLinesCount; i++ ) {
			if ( mVisibleLines[i].line() == lastOfLine )
				idx = i;
			else
				break;
		}
	}
	return static_cast<VisibleIndex>( idx );
}

bool DocumentView::isWrappedLine( Int64 docIdx ) const {
	if ( isWrapEnabled() ) {
		Int64 visibleIndex = static_cast<Int64>( toVisibleIndex( docIdx ) );
		return visibleIndex + 1 < static_cast<Int64>( mVisibleLines.size() ) &&
			   mVisibleLines[visibleIndex].line() == mVisibleLines[visibleIndex + 1].line();
	}
	return false;
}

DocumentView::VisibleLineInfo DocumentView::getVisibleLineInfo( Int64 docIdx ) const {
	eeASSERT( isLineVisible( docIdx ) );
	VisibleLineInfo line;
	if ( isOneToOne() ) {
		line.visualLines.push_back( { docIdx, 0 } );
		line.visibleIndex = static_cast<VisibleIndex>( docIdx );
		return line;
	}
	Int64 fromIdx = static_cast<Int64>( toVisibleIndex( docIdx ) );
	Int64 toIdx = static_cast<Int64>( toVisibleIndex( docIdx, true ) );
	line.visualLines.reserve( toIdx - fromIdx + 1 );
	for ( Int64 i = fromIdx; i <= toIdx; i++ )
		line.visualLines.emplace_back( mVisibleLines[i] );
	line.visibleIndex = static_cast<VisibleIndex>( fromIdx );
	line.paddingStart = mVisibleLinesOffset[docIdx];
	return line;
}

DocumentView::VisibleLineRange DocumentView::getVisibleLineRange( const TextPosition& pos,
																  bool allowVisualLineEnd ) const {
	if ( isOneToOne() ) {
		DocumentView::VisibleLineRange info;
		info.visibleIndex = static_cast<VisibleIndex>( pos.line() );
		info.range = mDoc->getLineRange( pos.line() );
		return info;
	}
	Int64 fromIdx = static_cast<Int64>( toVisibleIndex( pos.line() ) );
	Int64 toIdx = static_cast<Int64>( toVisibleIndex( pos.line(), true ) );
	// TODO: Implement binary search
	DocumentView::VisibleLineRange info;
	for ( Int64 i = fromIdx; i < toIdx; i++ ) {
		Int64 fromCol = mVisibleLines[i].column();
		Int64 toCol = i + 1 <= toIdx
						  ? mVisibleLines[i + 1].column() - ( allowVisualLineEnd ? 0 : 1 )
						  : mDoc->line( pos.line() ).size();
		if ( pos.column() >= fromCol && pos.column() <= toCol ) {
			info.visibleIndex = static_cast<VisibleIndex>( i );
			info.range = { { pos.line(), fromCol }, { pos.line(), toCol } };
			return info;
		}
	}
	eeASSERT( toIdx >= 0 );
	info.visibleIndex = static_cast<VisibleIndex>( toIdx );
	info.range = { { pos.line(), mVisibleLines[toIdx].column() },
				   mDoc->endOfLine( { pos.line(), 0ll } ) };
	return info;
}

TextRange DocumentView::getVisibleIndexRange( VisibleIndex visibleIndex ) const {
	if ( isOneToOne() )
		return mDoc->getLineRange( static_cast<Int64>( visibleIndex ) );
	Int64 idx = static_cast<Int64>( visibleIndex );
	auto start = getVisibleIndexPosition( visibleIndex );
	auto end = start;
	if ( idx + 1 < static_cast<Int64>( mVisibleLines.size() ) &&
		 mVisibleLines[idx + 1].line() == start.line() ) {
		end.setColumn( mVisibleLines[idx + 1].column() );
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

void DocumentView::clearCache() {
	mVisibleLines.clear();
	mDocLineToVisibleIndex.clear();
	mVisibleLinesOffset.clear();
}

void DocumentView::clear() {
	clearCache();
	mFoldedRegions.clear();
	mDoc->getFoldRangeService().clear();
}

double DocumentView::getLineYOffset( VisibleIndex visibleIndex, Float lineHeight ) const {
	return static_cast<double>( visibleIndex ) * lineHeight;
}

double DocumentView::getLineYOffset( Int64 docIdx, Float lineHeight ) const {
	eeASSERT( docIdx >= 0 && docIdx < static_cast<Int64>( mDoc->linesCount() ) );
	return lineHeight * static_cast<double>( toVisibleIndex( docIdx ) );
}

bool DocumentView::isLineVisible( Int64 docIdx ) const {
	return mDocLineToVisibleIndex.empty() || isOneToOne() ||
		   ( docIdx < static_cast<Int64>( mDocLineToVisibleIndex.size() ) &&
			 mDocLineToVisibleIndex[docIdx] != static_cast<Int64>( VisibleIndex::invalid ) );
}

std::vector<TextRange> DocumentView::intersectsFoldedRegions( const TextRange& range ) const {
	std::vector<TextRange> folds;
	for ( const auto& fold : mFoldedRegions ) {
		if ( fold.intersectsLineRange( range ) )
			folds.push_back( fold );
	}
	return folds;
}

Float DocumentView::getWhiteSpaceWidth() const {
	return mWhiteSpaceWidth;
}

void DocumentView::updateCache( Int64 fromLine, Int64 toLine, Int64 numLines ) {
	if ( isOneToOne() )
		return;

	// Unfold ANY modification over a folded range
	if ( numLines < 0 ) {
		auto foldedRegions = intersectsFoldedRegions( { { fromLine, 0 }, { toLine, 0 } } );
		for ( const auto& fold : foldedRegions )
			unfoldRegion( fold.start().line(), false, false, false );
	} else if ( isFolded( fromLine ) ) {
		// Offsets will be recomputed here instead in the unfold operation
		unfoldRegion( fromLine, false, false, false );
	}

	// Get affected visible range
	Int64 oldIdxFrom = static_cast<Int64>( toVisibleIndex( fromLine, false ) );
	Int64 oldIdxTo = static_cast<Int64>( toVisibleIndex( toLine, true ) );

	// Remove old visible lines
	mVisibleLines.erase( mVisibleLines.begin() + oldIdxFrom, mVisibleLines.begin() + oldIdxTo + 1 );

	// Remove old offsets
	mVisibleLinesOffset.erase( mVisibleLinesOffset.begin() + fromLine,
							   mVisibleLinesOffset.begin() + toLine + 1 );

	// Shift the line numbers
	if ( numLines != 0 ) {
		Int64 visibleLinesCount = mVisibleLines.size();
		for ( Int64 i = oldIdxFrom; i < visibleLinesCount; i++ )
			mVisibleLines[i].setLine( mVisibleLines[i].line() + numLines );

		shiftFoldingRegions( fromLine, numLines );
	}

	// Recompute line breaks
	auto netLines = toLine + numLines;
	auto idxOffset = oldIdxFrom;
	for ( auto i = fromLine; i <= netLines; i++ ) {
		if ( isFolded( i ) ) {
			mVisibleLinesOffset.insert(
				mVisibleLinesOffset.begin() + i,
				computeOffsets( mDoc->line( i ).getText().view(), mFontStyle, mConfig.tabWidth,
								eemax( mMaxWidth - mWhiteSpaceWidth, mWhiteSpaceWidth ) ) );
			mDocLineToVisibleIndex[i] = static_cast<Int64>( VisibleIndex::invalid );
		} else {
			auto lb =
				computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
								   mConfig.keepIndentation, mConfig.tabWidth, mWhiteSpaceWidth );
			mVisibleLinesOffset.insert( mVisibleLinesOffset.begin() + i, lb.paddingStart );
			for ( const auto& col : lb.wraps ) {
				mVisibleLines.insert( mVisibleLines.begin() + idxOffset, { i, col } );
				idxOffset++;
			}
		}
	}

	recomputeDocLineToVisibleIndex( oldIdxFrom );

	verifyStructuralConsistency();
}

void DocumentView::recomputeDocLineToVisibleIndex( Int64 fromVisibleIndex, bool ensureDocSize ) {
	// Recompute document line to visible index
	Int64 visibleLinesCount = mVisibleLines.size();
	if ( ensureDocSize )
		mDocLineToVisibleIndex.resize( mDoc->linesCount() );
	Int64 previousLineIdx = mVisibleLines[fromVisibleIndex].line();
	for ( Int64 visibleIdx = fromVisibleIndex; visibleIdx < visibleLinesCount; visibleIdx++ ) {
		const auto& visibleLine = mVisibleLines[visibleIdx];
		if ( visibleLine.column() == 0 ) {
			// Non-contiguos lines means hidden lines
			if ( visibleLine.line() - previousLineIdx > 1 ) {
				for ( Int64 i = previousLineIdx + 1; i < visibleLine.line(); i++ )
					mDocLineToVisibleIndex[i] = static_cast<Int64>( VisibleIndex::invalid );
			}
			mDocLineToVisibleIndex[visibleLine.line()] =
				isFolded( visibleLine.line(), true ) ? static_cast<Int64>( VisibleIndex::invalid )
													 : visibleIdx;
			previousLineIdx = visibleLine.line();
		}
	}
}

size_t DocumentView::getVisibleLinesCount() const {
	return isOneToOne() ? mDoc->linesCount() : mVisibleLines.size();
}

void DocumentView::foldRegion( Int64 foldDocIdx ) {
	auto foldRegion = mDoc->getFoldRangeService().find( foldDocIdx );
	if ( !foldRegion )
		return;
	if ( isOneToOne() && mDocLineToVisibleIndex.empty() )
		invalidateCache();
	Int64 toDocIdx = foldRegion->end().line();
	changeVisibility( foldDocIdx + 1, toDocIdx, false );
	mFoldedRegions.push_back( *foldRegion );
	std::sort( mFoldedRegions.begin(), mFoldedRegions.end() );
	moveCursorToVisibleArea();
	verifyStructuralConsistency();
}

void DocumentView::unfoldRegion( Int64 foldDocIdx ) {
	return unfoldRegion( foldDocIdx, true );
}

void DocumentView::unfoldRegion( Int64 foldDocIdx, bool verifyConsistency, bool recomputeOffset,
								 bool recomputeLineToVisibleIndex ) {
	auto foldRegion = mDoc->getFoldRangeService().find( foldDocIdx );
	if ( !foldRegion )
		return;
	Int64 toDocIdx = foldRegion->end().line();
	removeFoldedRegion( *foldRegion );
	changeVisibility( foldDocIdx + 1, toDocIdx, true, recomputeOffset,
					  recomputeLineToVisibleIndex );
	if ( verifyConsistency )
		verifyStructuralConsistency();
	if ( isOneToOne() )
		clearCache();
}

void DocumentView::ensureCursorVisibility() {
	if ( mFoldedRegions.empty() )
		return;
	const auto& selections = mDoc->getSelections();
	std::vector<TextRange> ranges;
	for ( const auto& selection : selections ) {
		auto res = isInFoldedRange( selection, true );
		if ( res && std::find( ranges.begin(), ranges.end(), *res ) == ranges.end() ) {
			auto sel( selection.normalized() );
			sel.start().setColumn( 0 );
			if ( !sel.normalized().contains( *res ) )
				ranges.emplace_back( std::move( *res ) );
		}
	}
	for ( const auto& range : ranges )
		unfoldRegion( range.start().line() );
}

void DocumentView::onFoldRegionsUpdated() {
	if ( mUpdatingFoldRegions )
		return;
	BoolScopedOp op( mUpdatingFoldRegions, true );
	std::vector<TextRange> add;
	for ( const auto& region : mFoldedRegions ) {
		if ( !mDoc->getFoldRangeService().isFoldingRegionInLine( region.start().line() ) )
			add.push_back( region );
	}
	if ( !add.empty() )
		mDoc->getFoldRangeService().addFoldRegions( add );
}

void DocumentView::moveCursorToVisibleArea() {
	if ( mFoldedRegions.empty() )
		return;
	const auto& selections = mDoc->getSelections();
	TextRanges newSelections;
	for ( const auto& selection : selections ) {
		auto res = isInFoldedRange( selection, true );
		if ( res ) {
			newSelections.emplace_back( TextRange{ res->start(), res->start() } );
		} else {
			newSelections.emplace_back( selection );
		}
	}
	newSelections.merge();
	if ( selections != newSelections )
		mDoc->resetSelection( newSelections );
}

bool DocumentView::isOneToOne() const {
	return mConfig.mode == LineWrapMode::NoWrap && mFoldedRegions.empty();
}

void DocumentView::changeVisibility( Int64 fromDocIdx, Int64 toDocIdx, bool visible,
									 bool recomputeOffset, bool recomputeLineToVisibleIndex ) {
	if ( visible ) {
		auto it = std::lower_bound( mVisibleLines.begin(), mVisibleLines.end(),
									TextPosition{ fromDocIdx, 0 } );
		Int64 oldIdxFrom = std::distance( mVisibleLines.begin(), it );
		auto idxOffset = oldIdxFrom;
		for ( auto i = fromDocIdx; i <= toDocIdx; i++ ) {
			if ( isFolded( i, true ) ) {
				if ( recomputeOffset ) {
					mVisibleLinesOffset[i] = computeOffsets(
						mDoc->line( i ).getText().view(), mFontStyle, mConfig.tabWidth,
						eemax( mMaxWidth - mWhiteSpaceWidth, mWhiteSpaceWidth ) );
				}
				continue;
			}
			auto lb = isWrapEnabled() ? computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth,
														   mConfig.mode, mConfig.keepIndentation,
														   mConfig.tabWidth, mWhiteSpaceWidth )
									  : LineWrapInfo{ { 0 }, 0 };
			if ( recomputeOffset )
				mVisibleLinesOffset[i] = lb.paddingStart;
			for ( const auto& col : lb.wraps ) {
				mVisibleLines.insert( mVisibleLines.begin() + idxOffset, { i, col } );
				idxOffset++;
			}
		}

		recomputeDocLineToVisibleIndex( oldIdxFrom, recomputeLineToVisibleIndex );
	} else {
		Int64 oldIdxFrom = static_cast<Int64>( toVisibleIndex( fromDocIdx ) );
		Int64 oldIdxTo = static_cast<Int64>( toVisibleIndex( toDocIdx, true ) );
		mVisibleLines.erase( mVisibleLines.begin() + oldIdxFrom,
							 mVisibleLines.begin() + oldIdxTo + 1 );
		for ( Int64 idx = fromDocIdx; idx <= toDocIdx; idx++ ) {
			mDocLineToVisibleIndex[idx] = static_cast<Int64>( VisibleIndex::invalid );
		}
		Int64 linesCount = mDoc->linesCount();
		Int64 idxOffset = oldIdxTo - oldIdxFrom + 1;
		for ( Int64 idx = toDocIdx + 1; idx < linesCount; idx++ ) {
			if ( mDocLineToVisibleIndex[idx] != static_cast<Int64>( VisibleIndex::invalid ) )
				mDocLineToVisibleIndex[idx] -= idxOffset;
		}
	}

	if ( recomputeLineToVisibleIndex )
		eeASSERT( mDocLineToVisibleIndex.size() == mDoc->linesCount() );
}

bool DocumentView::isFolded( Int64 docIdx, bool andNotFirstLine ) const {
	return std::any_of( mFoldedRegions.begin(), mFoldedRegions.end(),
						[&]( const TextRange& region ) {
							return region.containsLine( docIdx ) &&
								   ( andNotFirstLine ? region.start().line() != docIdx : true );
						} );
}

std::optional<TextRange> DocumentView::isInFoldedRange( TextRange range,
														bool andNotFirstLine ) const {
	range.normalize();
	for ( const auto& region : mFoldedRegions ) {
		if ( andNotFirstLine && !region.inSameLine() ) {
			auto sregion( region );
			sregion.start().setLine(
				eemin<Int64>( sregion.start().line() + 1, mDoc->linesCount() - 1 ) );
			sregion.start().setColumn( 0 );
			sregion.normalize();
			if ( sregion.intersectsLineRange( range ) )
				return region;
		} else if ( region.intersectsLineRange( range ) )
			return region;
	}
	return {};
}

void DocumentView::removeFoldedRegion( const TextRange& region ) {
	auto found = std::find( mFoldedRegions.begin(), mFoldedRegions.end(), region );
	if ( found != mFoldedRegions.end() )
		mFoldedRegions.erase( found );
}

void DocumentView::shiftFoldingRegions( Int64 fromLine, Int64 numLines ) {
	mDoc->getFoldRangeService().shiftFoldingRegions( fromLine, numLines );

	for ( auto& region : mFoldedRegions ) {
		if ( region.start().line() >= fromLine ) {
			region.start().setLine( region.start().line() + numLines );
			region.end().setLine( region.end().line() + numLines );
		}
	}
}

void DocumentView::verifyStructuralConsistency() {
#ifdef EE_VERIFY_STRUCTURAL_CONSISTENCY
	if ( isOneToOne() )
		return;

	auto visibleLines = mVisibleLines;
	auto docLineToVisibleIndex = mDocLineToVisibleIndex;
	auto visibleLinesOffset = mVisibleLinesOffset;

	invalidateCache();

	auto visibleLinesConsistency = visibleLines == mVisibleLines;
	eeASSERT( visibleLinesConsistency );

	if ( !visibleLinesConsistency && mVisibleLines.size() == visibleLines.size() ) {
		for ( size_t i = 0; i < mVisibleLines.size(); i++ ) {
			if ( mVisibleLines[i] != visibleLines[i] ) {
				eeASSERT( mVisibleLines[i] == visibleLines[i] );
				break;
			}
		}
	}

	bool docConsistency = docLineToVisibleIndex == mDocLineToVisibleIndex;
	eeASSERT( docConsistency );

	if ( !docConsistency && docLineToVisibleIndex.size() == mDocLineToVisibleIndex.size() ) {
		for ( size_t i = 0; i < mDocLineToVisibleIndex.size(); i++ ) {
			if ( mDocLineToVisibleIndex[i] != docLineToVisibleIndex[i] ) {
				eeASSERT( mDocLineToVisibleIndex[i] == docLineToVisibleIndex[i] );
				break;
			}
		}
	}

	bool offsetConsistency = visibleLinesOffset == mVisibleLinesOffset;
	eeASSERT( offsetConsistency );

	if ( !offsetConsistency && visibleLinesOffset.size() == mVisibleLinesOffset.size() ) {
		for ( size_t i = 0; i < mVisibleLinesOffset.size(); i++ ) {
			if ( mVisibleLinesOffset[i] != visibleLinesOffset[i] ) {
				eeASSERT( mVisibleLinesOffset[i] == visibleLinesOffset[i] );
				break;
			}
		}
	}

	eeASSERT( mVisibleLinesOffset.size() == mDoc->linesCount() );
#endif
}

}}} // namespace EE::UI::Doc
