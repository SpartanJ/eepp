#include <algorithm>
#include <cctype>
#include <eepp/core/string.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/gridlayouter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <set>

namespace EE { namespace UI {

// ── Internal helpers ──

static std::vector<std::string> tokenizeTrackList( std::string_view value ) {
	std::vector<std::string> tokens;
	std::string current;
	int parenDepth = 0;
	int bracketDepth = 0;

	for ( size_t i = 0; i < value.size(); ++i ) {
		char c = value[i];
		if ( c == '(' ) {
			++parenDepth;
			current += c;
		} else if ( c == ')' ) {
			--parenDepth;
			current += c;
		} else if ( c == '[' ) {
			++bracketDepth;
			current += c;
		} else if ( c == ']' ) {
			--bracketDepth;
			current += c;
		} else if ( std::isspace( static_cast<unsigned char>( c ) ) && parenDepth == 0 &&
					bracketDepth == 0 ) {
			if ( !current.empty() ) {
				tokens.push_back( current );
				current.clear();
			}
		} else {
			current += c;
		}
	}
	if ( !current.empty() )
		tokens.push_back( current );
	return tokens;
}

static std::string_view peekIdentifier( std::string_view token ) {
	size_t end = 0;
	while ( end < token.size() && ( std::isalnum( static_cast<unsigned char>( token[end] ) ) ||
									token[end] == '-' || token[end] == '_' ) ) {
		++end;
	}
	return token.substr( 0, end );
}

static bool isNumber( std::string_view s ) {
	if ( s.empty() )
		return false;
	size_t start = 0;
	if ( s[0] == '+' || s[0] == '-' )
		++start;
	bool hasDigit = false;
	bool hasDot = false;
	for ( size_t i = start; i < s.size(); ++i ) {
		if ( std::isdigit( static_cast<unsigned char>( s[i] ) ) ) {
			hasDigit = true;
		} else if ( s[i] == '.' ) {
			if ( hasDot )
				return false;
			hasDot = true;
		} else {
			return false;
		}
	}
	return hasDigit;
}

static bool parseInt( std::string_view s, int& out ) {
	std::string str( s );
	return String::fromString( out, str );
}

static GridTrackBreadth parseTrackBreadth( std::string_view token ) {
	GridTrackBreadth breadth;
	breadth.raw = std::string( token );

	if ( token == "auto" ) {
		breadth.type = GridTrackBreadthType::Auto;
		return breadth;
	}
	if ( token == "min-content" ) {
		breadth.type = GridTrackBreadthType::MinContent;
		return breadth;
	}
	if ( token == "max-content" ) {
		breadth.type = GridTrackBreadthType::MaxContent;
		return breadth;
	}

	// Check for fr unit
	if ( token.size() > 2 && token[token.size() - 1] == 'r' && token[token.size() - 2] == 'f' ) {
		std::string_view numPart = token.substr( 0, token.size() - 2 );
		if ( isNumber( numPart ) ) {
			breadth.type = GridTrackBreadthType::Flex;
			Float val = 0.f;
			String::fromString( val, std::string( numPart ) );
			if ( val < 0.f ) {
				breadth.valid = false;
				return breadth;
			}
			breadth.value = val;
			return breadth;
		}
	}

	// Try StyleSheetLength for length / percentage
	CSS::StyleSheetLength len = CSS::StyleSheetLength::fromString( std::string( token ) );
	if ( len.getUnit() == CSS::StyleSheetLength::Percentage ) {
		breadth.type = GridTrackBreadthType::Percentage;
		breadth.value = len.getValue();
		return breadth;
	}
	char first = token.front();
	if ( first != '.' && ( first < '0' || first > '9' ) ) {
		breadth.valid = false;
		return breadth;
	}
	breadth.type = GridTrackBreadthType::Length;
	breadth.value = len.getValue();
	return breadth;
}

static bool parseBreadthList( std::string_view args, GridTrackBreadth& outMin,
							  GridTrackBreadth& outMax ) {
	// Split args on top-level commas
	std::vector<std::string> parts;
	std::string current;
	int parenDepth = 0;
	for ( size_t i = 0; i < args.size(); ++i ) {
		char c = args[i];
		if ( c == '(' )
			++parenDepth;
		else if ( c == ')' )
			--parenDepth;
		else if ( c == ',' && parenDepth == 0 ) {
			parts.push_back( String::trim( current ) );
			current.clear();
			continue;
		}
		current += c;
	}
	parts.push_back( String::trim( current ) );

	if ( parts.size() != 2 )
		return false;

	outMin = parseTrackBreadth( parts[0] );
	outMax = parseTrackBreadth( parts[1] );
	return true;
}

static GridTrackList parseFunctionRepeat( std::string_view args ) {
	GridTrackList result;
	std::string countStr;
	std::string tracksStr;
	int parenDepth = 0;
	bool foundComma = false;
	for ( size_t i = 0; i < args.size(); ++i ) {
		char c = args[i];
		if ( c == '(' )
			++parenDepth;
		else if ( c == ')' )
			--parenDepth;
		else if ( c == ',' && parenDepth == 0 && !foundComma ) {
			countStr = String::trim( countStr );
			tracksStr = String::trim( std::string( args.substr( i + 1 ) ) );
			foundComma = true;
			break;
		}
		if ( !foundComma )
			countStr += c;
	}
	if ( !foundComma ) {
		result.valid = false;
		return result;
	}

	int count = 0;
	bool isAutoRepeat = ( countStr == "auto-fill" || countStr == "auto-fit" );

	if ( isAutoRepeat ) {
		GridTrackList inner = GridTrackParser::parseTrackList( tracksStr );
		if ( !inner.valid ) {
			result.valid = false;
			return result;
		}
		// Nested auto-repeat is invalid per CSS Grid spec
		if ( inner.hasAutoRepeat ) {
			result.valid = false;
			return result;
		}
		result.hasAutoRepeat = true;
		result.autoRepeatIsFit = ( countStr == "auto-fit" );
		result.autoRepeatTemplate = inner.tracks;
		return result;
	}

	if ( !parseInt( countStr, count ) || count <= 0 ) {
		result.valid = false;
		return result;
	}

	GridTrackList inner = GridTrackParser::parseTrackList( tracksStr );
	if ( !inner.valid ) {
		result.valid = false;
		return result;
	}

	for ( int i = 0; i < count; ++i ) {
		for ( const auto& t : inner.tracks ) {
			result.tracks.push_back( t );
		}
	}
	return result;
}

static bool parseLineNames( std::string_view token, SmallVector<std::string, 2>& out ) {
	if ( token.size() < 2 || token.front() != '[' || token.back() != ']' )
		return false;
	std::string inside( token.substr( 1, token.size() - 2 ) );
	String::removeExtraSpaces( inside );
	if ( inside.empty() )
		return true; // empty list is OK
	auto names = String::split( inside, ' ' );
	for ( const auto& n : names ) {
		std::string trimmed = String::trim( n );
		if ( !trimmed.empty() )
			out.push_back( trimmed );
	}
	return true;
}

static bool isValidTrackBreadthType( const GridTrackBreadth& b ) {
	if ( !b.valid )
		return false;
	return b.type == GridTrackBreadthType::Auto || b.type == GridTrackBreadthType::MinContent ||
		   b.type == GridTrackBreadthType::MaxContent || b.type == GridTrackBreadthType::Length ||
		   b.type == GridTrackBreadthType::Percentage || b.type == GridTrackBreadthType::Flex;
}

static GridTrackSize parseAutoTrackSize( const std::string& value ) {
	GridTrackSize size;
	std::string trimmed = String::trim( value );
	String::toLowerInPlace( trimmed );
	if ( trimmed.empty() || trimmed == "auto" )
		return size;

	GridTrackList list = GridTrackParser::parseTrackList( trimmed );
	if ( list.valid && !list.none && list.tracks.size() == 1 )
		return list.tracks[0].size;

	GridTrackBreadth b = parseTrackBreadth( trimmed );
	if ( b.valid ) {
		size.min = b;
		size.max = b;
	}
	return size;
}

static bool isFlexibleTrack( const GridTrack& track ) {
	return track.definition.min.type == GridTrackBreadthType::Flex ||
		   track.definition.max.type == GridTrackBreadthType::Flex;
}

// ── Public API ──

GridTrackList GridTrackParser::parseTrackList( const std::string& value ) {
	GridTrackList result;
	std::string trimmed = String::trim( value );
	String::toLowerInPlace( trimmed );

	if ( trimmed == "none" ) {
		result.none = true;
		return result;
	}
	if ( trimmed.empty() ) {
		result.none = true;
		return result;
	}

	std::vector<std::string> tokens = tokenizeTrackList( trimmed );
	if ( tokens.empty() ) {
		result.none = true;
		return result;
	}

	SmallVector<std::string, 2> pendingBeforeNames;

	for ( const std::string& rawToken : tokens ) {
		std::string_view token = rawToken;

		if ( token.front() == '[' && token.back() == ']' ) {
			parseLineNames( token, pendingBeforeNames );
			continue;
		}

		if ( token.front() == '[' || token.back() == ']' ) {
			result.valid = false;
			return result;
		}

		// Function tokens: ident(...)
		if ( token.back() == ')' ) {
			std::string_view ident = peekIdentifier( token );
			size_t openParen = token.find( '(' );
			if ( openParen == std::string_view::npos ) {
				result.valid = false;
				return result;
			}
			std::string_view args = token.substr( openParen + 1, token.size() - openParen - 2 );

			if ( ident == "minmax" ) {
				GridExplicitTrack track;
				track.beforeLineNames = pendingBeforeNames;
				pendingBeforeNames.clear();
				if ( !parseBreadthList( args, track.size.min, track.size.max ) ) {
					result.valid = false;
					return result;
				}
				result.tracks.push_back( std::move( track ) );
				continue;
			}

			if ( ident == "fit-content" ) {
				GridExplicitTrack track;
				track.beforeLineNames = pendingBeforeNames;
				pendingBeforeNames.clear();
				std::string arg = String::trim( std::string( args ) );
				CSS::StyleSheetLength len = CSS::StyleSheetLength::fromString( arg );
				track.size.min.type = GridTrackBreadthType::Auto;
				track.size.min.raw = "auto";
				track.size.max.type = GridTrackBreadthType::FitContent;
				track.size.max.value = len.getValue();
				track.size.max.raw = arg;
				result.tracks.push_back( std::move( track ) );
				continue;
			}

			if ( ident == "repeat" ) {
				GridTrackList repeated = parseFunctionRepeat( args );
				if ( !repeated.valid ) {
					result.valid = false;
					return result;
				}
				if ( !repeated.tracks.empty() && !pendingBeforeNames.empty() ) {
					for ( auto& name : pendingBeforeNames )
						repeated.tracks[0].beforeLineNames.insert(
							repeated.tracks[0].beforeLineNames.begin(), std::move( name ) );
					pendingBeforeNames.clear();
				}
				for ( auto& t : repeated.tracks )
					result.tracks.push_back( std::move( t ) );
				if ( repeated.hasAutoRepeat ) {
					result.hasAutoRepeat = true;
					result.autoRepeatPosition = result.tracks.size();
					result.autoRepeatIsFit = repeated.autoRepeatIsFit;
					result.autoRepeatTemplate = std::move( repeated.autoRepeatTemplate );
				}
				continue;
			}

			result.valid = false;
			return result;
		}

		// Plain breadth token
		GridTrackBreadth b = parseTrackBreadth( token );
		if ( !isValidTrackBreadthType( b ) ) {
			result.valid = false;
			return result;
		}
		GridExplicitTrack track;
		track.beforeLineNames = pendingBeforeNames;
		pendingBeforeNames.clear();
		track.size.min = b;
		track.size.max = b;
		track.size.max.raw = b.raw;
		result.tracks.push_back( std::move( track ) );
	}

	return result;
}

bool GridTrackParser::isValidTrackList( const std::string& value ) {
	GridTrackList list = parseTrackList( value );
	return list.valid;
}

void GridTrackParser::expandAutoRepeat( GridTrackList& list, Float availableSize, Float gap ) {
	if ( !list.hasAutoRepeat || list.autoRepeatTemplate.empty() )
		return;

	// Compute minimum size for one repetition using the min side of each track
	Float oneRepMinSize = 0.f;
	bool hasZeroLengthMin = false;
	for ( const auto& t : list.autoRepeatTemplate ) {
		switch ( t.size.min.type ) {
			case GridTrackBreadthType::Length:
				oneRepMinSize += t.size.min.value;
				hasZeroLengthMin = true;
				break;
			case GridTrackBreadthType::Percentage:
				if ( availableSize > 0.f ) {
					oneRepMinSize += t.size.min.value * availableSize / 100.f;
					hasZeroLengthMin = true;
				}
				break;
			default:
				break;
		}
	}
	if ( list.autoRepeatTemplate.size() > 1 )
		oneRepMinSize += static_cast<Float>( list.autoRepeatTemplate.size() - 1 ) * gap;

	int count = 1;
	if ( availableSize > 0.f && oneRepMinSize > 0.f ) {
		count = static_cast<int>( ( availableSize + gap ) / ( oneRepMinSize + gap ) );
		if ( count < 1 )
			count = 1;
	} else if ( availableSize > 0.f && hasZeroLengthMin && gap > 0.f ) {
		// All tracks have zero-length minima (e.g. minmax(0, 1fr)) — use gap as effective min
		count = static_cast<int>( ( availableSize + gap ) / gap );
		if ( count < 1 )
			count = 1;
	}

	// Expand auto-repeat tracks and insert at the recorded position
	std::vector<GridExplicitTrack> expanded;
	expanded.reserve( count * list.autoRepeatTemplate.size() );
	for ( int i = 0; i < count; ++i ) {
		for ( const auto& t : list.autoRepeatTemplate ) {
			expanded.push_back( t );
		}
	}

	if ( list.autoRepeatPosition <= list.tracks.size() )
		list.tracks.insert( list.tracks.begin() + list.autoRepeatPosition,
							std::make_move_iterator( expanded.begin() ),
							std::make_move_iterator( expanded.end() ) );
	else
		list.tracks.insert( list.tracks.end(), std::make_move_iterator( expanded.begin() ),
							std::make_move_iterator( expanded.end() ) );

	list.hasAutoRepeat = false;
	list.autoRepeatIsFit = false;
	list.autoRepeatPosition = 0;
	list.autoRepeatTemplate.clear();
}

// ── GridLayouter ──

static void resolveAutoRepeatLengths( GridTrackList& list, UIWidget* container,
									  CSS::PropertyRelativeTarget relativeTarget ) {
	for ( auto& track : list.autoRepeatTemplate ) {
		if ( track.size.min.type == GridTrackBreadthType::Length )
			track.size.min.value = container->lengthFromValue( track.size.min.raw, relativeTarget,
															   track.size.min.value );
		if ( track.size.max.type == GridTrackBreadthType::Length ||
			 track.size.max.type == GridTrackBreadthType::FitContent )
			track.size.max.value = container->lengthFromValue( track.size.max.raw, relativeTarget,
															   track.size.max.value );
	}
}

void GridLayouter::readContainerStyle() {
	UIHTMLWidget* grid =
		mContainer->isType( UI_TYPE_HTML_WIDGET ) ? mContainer->asType<UIHTMLWidget>() : nullptr;
	if ( !grid )
		return;

	mColumnGap = mContainer->lengthFromValue(
		grid->getColumnGap(), CSS::PropertyRelativeTarget::ContainingBlockWidth, 0.f );
	mRowGap = mContainer->lengthFromValue(
		grid->getRowGap(), CSS::PropertyRelativeTarget::ContainingBlockHeight, 0.f );
	mTemplateAreas = GridAreasParser::parseAreas( grid->getGridTemplateAreas() );
	mAutoFlow = grid->getGridAutoFlow();
	mAutoFlowDense = grid->getGridAutoFlowDense();

	mAutoColumnSize = parseAutoTrackSize( grid->getGridAutoColumns() );
	mAutoRowSize = parseAutoTrackSize( grid->getGridAutoRows() );

	// Parse and expand column tracks (including auto-repeat)
	GridTrackList colList = GridTrackParser::parseTrackList( grid->getGridTemplateColumns() );
	mColAutoRepeatIsFit = colList.autoRepeatIsFit;
	// Record explicit count before auto-repeat expansion
	mExplicitColCount = colList.tracks.size();
	{
		Float contentW = mContainer->getPixelsSize().getWidth() -
						 mContainer->getPixelsContentOffset().Left -
						 mContainer->getPixelsContentOffset().Right;
		if ( contentW > 0.f ) {
			resolveAutoRepeatLengths( colList, mContainer,
									  CSS::PropertyRelativeTarget::ContainingBlockWidth );
			GridTrackParser::expandAutoRepeat( colList, contentW, mColumnGap );
		}
	}
	mColumns.clear();
	mColLineNames.clear();
	if ( colList.valid && !colList.none ) {
		for ( const auto& t : colList.tracks ) {
			GridTrack track;
			track.definition = t.size;
			if ( !t.beforeLineNames.empty() ) {
				int lineIdx = static_cast<int>( mColumns.size() ) + 1;
				for ( const auto& name : t.beforeLineNames )
					mColLineNames[name].push_back( lineIdx );
			}
			mColumns.push_back( track );
		}
	}

	// Parse and expand row tracks
	GridTrackList rowList = GridTrackParser::parseTrackList( grid->getGridTemplateRows() );
	mRowAutoRepeatIsFit = rowList.autoRepeatIsFit;
	mExplicitRowCount = rowList.tracks.size();
	{
		Float contentH = mContainer->getPixelsSize().getHeight() -
						 mContainer->getPixelsContentOffset().Top -
						 mContainer->getPixelsContentOffset().Bottom;
		if ( contentH > 0.f ) {
			resolveAutoRepeatLengths( rowList, mContainer,
									  CSS::PropertyRelativeTarget::ContainingBlockHeight );
			GridTrackParser::expandAutoRepeat( rowList, contentH, mRowGap );
		}
	}
	mRows.clear();
	mRowLineNames.clear();
	if ( rowList.valid && !rowList.none ) {
		for ( const auto& t : rowList.tracks ) {
			GridTrack track;
			track.definition = t.size;
			if ( !t.beforeLineNames.empty() ) {
				int lineIdx = static_cast<int>( mRows.size() ) + 1;
				for ( const auto& name : t.beforeLineNames )
					mRowLineNames[name].push_back( lineIdx );
			}
			mRows.push_back( track );
		}
	}
	mExplicitRowCount = mRows.size();
}

void GridLayouter::collapseEmptyAutoFitTracks() {
	int minCols = static_cast<int>( mExplicitColCount );
	int minRows = static_cast<int>( mExplicitRowCount );

	if ( mColAutoRepeatIsFit && !mItems.empty() ) {
		int maxUsedCol = 0;
		for ( const auto& item : mItems ) {
			int end = item.resolvedColumnEnd;
			if ( end > maxUsedCol )
				maxUsedCol = end;
		}
		int target = std::max( maxUsedCol - 1, minCols );
		while ( static_cast<int>( mColumns.size() ) > target && (int)mColumns.size() > 0 )
			mColumns.pop_back();
	}

	if ( mRowAutoRepeatIsFit && !mItems.empty() ) {
		int maxUsedRow = 0;
		for ( const auto& item : mItems ) {
			int end = item.resolvedRowEnd;
			if ( end > maxUsedRow )
				maxUsedRow = end;
		}
		int target = std::max( maxUsedRow - 1, minRows );
		while ( static_cast<int>( mRows.size() ) > target && (int)mRows.size() > 0 )
			mRows.pop_back();
	}
}

void GridLayouter::buildImplicitTracks() {
	while ( static_cast<int>( mColumns.size() ) < mMaxColumn - 1 ) {
		GridTrack track;
		track.definition = mAutoColumnSize;
		mColumns.push_back( track );
	}
	while ( static_cast<int>( mRows.size() ) < mMaxRow - 1 ) {
		GridTrack track;
		track.definition = mAutoRowSize;
		mRows.push_back( track );
	}
}

void GridLayouter::sizeTracksForAxis( bool isColumns ) {
	auto& tracks = isColumns ? mColumns : mRows;
	Float containerSize = isColumns ? mContainer->getPixelsSize().getWidth()
									: mContainer->getPixelsSize().getHeight();
	Float contentOffset = isColumns ? mContainer->getPixelsContentOffset().Left
									: mContainer->getPixelsContentOffset().Top;
	Float contentEnd = isColumns ? mContainer->getPixelsContentOffset().Right
								 : mContainer->getPixelsContentOffset().Bottom;
	Float contentBoxSize = containerSize - contentOffset - contentEnd;
	if ( contentBoxSize < 0.f )
		contentBoxSize = 0.f;
	bool hasDefiniteSize = mContainer->getLayoutWidthPolicy() != SizePolicy::WrapContent;
	if ( !isColumns )
		hasDefiniteSize = mContainer->getLayoutHeightPolicy() != SizePolicy::WrapContent;
	CSS::PropertyRelativeTarget relativeTarget =
		isColumns ? CSS::PropertyRelativeTarget::ContainingBlockWidth
				  : CSS::PropertyRelativeTarget::ContainingBlockHeight;
	auto resolveLength = [&]( const GridTrackBreadth& breadth ) {
		// GridTrackParser keeps the raw CSS token so sizing can resolve units with the container
		// context. Do not use breadth.value directly for Length tracks: tokens such as "6.5rem"
		// parse to numeric 6.5 but must become CSS pixels before track sizing and item wrapping.
		return mContainer->lengthFromValue( breadth.raw, relativeTarget, breadth.value );
	};

	// Initialize base sizes
	for ( auto& track : tracks ) {
		switch ( track.definition.min.type ) {
			case GridTrackBreadthType::Length:
				track.baseSize = resolveLength( track.definition.min );
				break;
			case GridTrackBreadthType::Percentage:
				if ( hasDefiniteSize )
					track.baseSize = track.definition.min.value * contentBoxSize / 100.f;
				else
					track.baseSize = 0.f;
				break;
			default:
				track.baseSize = 0.f;
				break;
		}
		if ( track.definition.max.type == GridTrackBreadthType::Length )
			track.growthLimit = resolveLength( track.definition.max );
		else if ( track.definition.max.type == GridTrackBreadthType::Percentage && hasDefiniteSize )
			track.growthLimit = track.definition.max.value * contentBoxSize / 100.f;
		else if ( track.definition.max.type == GridTrackBreadthType::FitContent )
			track.growthLimit = resolveLength( track.definition.max );
		else
			track.growthLimit = std::numeric_limits<Float>::infinity();
	}

	// Non-spanning item contributions
	for ( const auto& item : mItems ) {
		if ( item.isTextNode )
			continue;
		int start = isColumns ? item.resolvedColumnStart : item.resolvedRowStart;
		int end = isColumns ? item.resolvedColumnEnd : item.resolvedRowEnd;
		int span = end - start;

		if ( start <= 0 || start - 1 >= static_cast<int>( tracks.size() ) )
			continue;

		auto& track = tracks[start - 1];

		// Only update auto/min-content/max-content tracks (non-spanning)
		if ( span == 1 && ( track.definition.min.type == GridTrackBreadthType::Auto ||
							track.definition.min.type == GridTrackBreadthType::MinContent ||
							track.definition.min.type == GridTrackBreadthType::MaxContent ) ) {
			Float itemSize = isColumns ? item.widget->getPixelsSize().getWidth()
									   : item.widget->getPixelsSize().getHeight();
			if ( itemSize > track.baseSize )
				track.baseSize = itemSize;
		}
	}

	// Phase 9: spanning items — distribute extra item size across spanned tracks
	for ( const auto& item : mItems ) {
		if ( item.isTextNode )
			continue;
		int start = isColumns ? item.resolvedColumnStart : item.resolvedRowStart;
		int end = isColumns ? item.resolvedColumnEnd : item.resolvedRowEnd;
		int span = end - start;
		if ( span <= 1 )
			continue;
		if ( start <= 0 || start - 1 >= static_cast<int>( tracks.size() ) )
			continue;

		Float itemSize = isColumns ? item.widget->getPixelsSize().getWidth()
								   : item.widget->getPixelsSize().getHeight();
		Float spannedSize = 0.f;
		int spanStart = start - 1;
		int spanEnd = std::min( end - 1, static_cast<int>( tracks.size() ) );
		bool allSpannedTracksAreFlexible = true;
		for ( int i = spanStart; i < spanEnd; ++i )
			allSpannedTracksAreFlexible =
				allSpannedTracksAreFlexible && isFlexibleTrack( tracks[i] );
		if ( allSpannedTracksAreFlexible )
			continue;
		for ( int i = spanStart; i < spanEnd; ++i )
			spannedSize += tracks[i].baseSize;
		spannedSize += static_cast<Float>( span - 1 ) * ( isColumns ? mColumnGap : mRowGap );

		if ( itemSize > spannedSize ) {
			Float extra = itemSize - spannedSize;
			bool anyGrew = true;
			while ( extra > 0.001f && anyGrew ) {
				anyGrew = false;
				Float perTrack = extra / static_cast<Float>( spanEnd - spanStart );
				for ( int i = spanStart; i < spanEnd && extra > 0.001f; ++i ) {
					if ( tracks[i].baseSize >= tracks[i].growthLimit )
						continue;
					Float add = std::min( perTrack, tracks[i].growthLimit - tracks[i].baseSize );
					tracks[i].baseSize += add;
					extra -= add;
					anyGrew = true;
				}
			}
			if ( extra > 0.f ) {
				for ( int i = spanStart; i < spanEnd; ++i ) {
					if ( tracks[i].baseSize < tracks[i].growthLimit ) {
						tracks[i].baseSize += extra;
						break;
					}
				}
			}
		}
	}

	// Phase 8: flexible (fr) tracks
	Float remainingSpace = contentBoxSize;
	Float totalFlex = 0.f;
	for ( auto& track : tracks )
		remainingSpace -= track.baseSize;

	if ( tracks.size() > 1 ) {
		Float axisGap = isColumns ? mColumnGap : mRowGap;
		remainingSpace -= static_cast<Float>( tracks.size() - 1 ) * axisGap;
	}
	if ( remainingSpace < 0 )
		remainingSpace = 0;

	for ( auto& track : tracks ) {
		if ( track.definition.min.type == GridTrackBreadthType::Flex ) {
			totalFlex += track.definition.min.value;
		} else if ( track.definition.max.type == GridTrackBreadthType::Flex ) {
			totalFlex += track.definition.max.value;
		}
	}

	if ( totalFlex > 0.f && remainingSpace > 0.f ) {
		for ( auto& track : tracks ) {
			Float flex = 0.f;
			if ( track.definition.min.type == GridTrackBreadthType::Flex )
				flex = track.definition.min.value;
			else if ( track.definition.max.type == GridTrackBreadthType::Flex )
				flex = track.definition.max.value;

			if ( flex > 0.f ) {
				Float extra = remainingSpace * ( flex / totalFlex );
				Float wanted = track.baseSize + extra;
				if ( wanted > track.growthLimit )
					wanted = track.growthLimit;
				Float actualExtra = wanted - track.baseSize;
				track.baseSize = wanted;
				remainingSpace -= actualExtra;
				totalFlex -= flex;
			}
		}
	}
}

void GridLayouter::preSizeItemsForRowSizing() {
	// Compute column line positions WITHOUT gaps (gaps added only as offset)
	std::vector<Float> colLines;
	colLines.push_back( 0.f );
	for ( const auto& col : mColumns )
		colLines.push_back( colLines.back() + col.baseSize );

	for ( auto& item : mItems ) {
		if ( item.isTextNode )
			continue;

		int cs = item.resolvedColumnStart - 1;
		int ce = item.resolvedColumnEnd - 1;
		if ( cs < 0 )
			cs = 0;
		if ( ce >= static_cast<int>( colLines.size() ) )
			ce = static_cast<int>( colLines.size() ) - 1;

		Float cellW = colLines[ce] - colLines[cs];
		if ( ce > cs + 1 )
			cellW += static_cast<Float>( ce - cs - 1 ) * mColumnGap;

		auto oldWidthPolicy = item.widget->getLayoutWidthPolicy();
		item.widget->setLayoutWidthPolicy( SizePolicy::Fixed );
		item.widget->setPixelsSize( cellW, item.widget->getPixelsSize().getHeight() );

		if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) ) {
			auto* childHtml = item.widget->asType<UIHTMLWidget>();
			auto* layouter = childHtml->getLayouter();
			if ( layouter && !layouter->isPacking() ) {
				auto oldHP = childHtml->getLayoutHeightPolicy();
				childHtml->setLayoutHeightPolicy( SizePolicy::WrapContent );
				childHtml->updateLayout();
				childHtml->setLayoutHeightPolicy( oldHP );
			}
		}
		item.widget->setLayoutWidthPolicy( oldWidthPolicy );
	}
}

void GridLayouter::applyLayout() {
	// Compute line positions WITHOUT gaps (gaps are added as per-cell offset)
	std::vector<Float> colLines;
	colLines.push_back( 0.f );
	for ( const auto& col : mColumns )
		colLines.push_back( colLines.back() + col.baseSize );
	Float totalWidth = colLines.back() +
					   static_cast<Float>( std::max( 0, (int)mColumns.size() - 1 ) ) * mColumnGap;

	std::vector<Float> rowLines;
	rowLines.push_back( 0.f );
	for ( const auto& row : mRows )
		rowLines.push_back( rowLines.back() + row.baseSize );
	Float totalHeight =
		rowLines.back() + static_cast<Float>( std::max( 0, (int)mRows.size() - 1 ) ) * mRowGap;

	Float padLeft = mContainer->getPixelsContentOffset().Left;
	Float padTop = mContainer->getPixelsContentOffset().Top;
	Float padRight = mContainer->getPixelsContentOffset().Right;
	Float padBottom = mContainer->getPixelsContentOffset().Bottom;

	// Phase 10: grid alignment (justify-content, align-content)
	UIHTMLWidget* gridWidget =
		mContainer->isType( UI_TYPE_HTML_WIDGET ) ? mContainer->asType<UIHTMLWidget>() : nullptr;
	Float colGapX = mColumnGap;
	Float rowGapY = mRowGap;
	Float colStartX = padLeft;
	Float rowStartY = padTop;
	bool wrapW = mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent;
	bool wrapH = mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent;

	if ( gridWidget ) {
		Float containerW = mContainer->getPixelsSize().getWidth() - padLeft - padRight;
		Float containerH = mContainer->getPixelsSize().getHeight() - padTop - padBottom;

		if ( containerW > totalWidth && !wrapW ) {
			Float extra = containerW - totalWidth;
			CSSJustifyContent jc = gridWidget->getJustifyContent();
			if ( jc == CSSJustifyContent::Center )
				colStartX += extra * 0.5f;
			else if ( jc == CSSJustifyContent::FlexEnd )
				colStartX += extra;
			else if ( jc == CSSJustifyContent::SpaceBetween && mColumns.size() > 1 ) {
				Float gapExtra = extra / static_cast<Float>( mColumns.size() - 1 );
				colGapX += gapExtra;
			} else if ( jc == CSSJustifyContent::SpaceAround && mColumns.size() > 0 ) {
				Float gapExtra = extra / static_cast<Float>( mColumns.size() );
				colStartX += gapExtra * 0.5f;
				colGapX += gapExtra;
			} else if ( jc == CSSJustifyContent::SpaceEvenly && mColumns.size() > 0 ) {
				Float gapExtra = extra / static_cast<Float>( mColumns.size() + 1 );
				colStartX += gapExtra;
				colGapX += gapExtra;
			}
		}

		if ( containerH > totalHeight && !wrapH ) {
			Float extra = containerH - totalHeight;
			CSSAlignContent ac = gridWidget->getAlignContent();
			if ( ac == CSSAlignContent::Center )
				rowStartY += extra * 0.5f;
			else if ( ac == CSSAlignContent::FlexEnd )
				rowStartY += extra;
			else if ( ac == CSSAlignContent::SpaceBetween && mRows.size() > 1 ) {
				Float gapExtra = extra / static_cast<Float>( mRows.size() - 1 );
				rowGapY += gapExtra;
			} else if ( ac == CSSAlignContent::SpaceAround && mRows.size() > 0 ) {
				Float gapExtra = extra / static_cast<Float>( mRows.size() );
				rowStartY += gapExtra * 0.5f;
				rowGapY += gapExtra;
			} else if ( ac == CSSAlignContent::SpaceEvenly && mRows.size() > 0 ) {
				Float gapExtra = extra / static_cast<Float>( mRows.size() + 1 );
				rowStartY += gapExtra;
				rowGapY += gapExtra;
			}
		}
	}

	// Set container size if wrapping (use internal setters to avoid re-layout cycle)
	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent )
		mContainer->setInternalPixelsWidth( totalWidth + padLeft + padRight );
	if ( mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent )
		mContainer->setInternalPixelsHeight( totalHeight + padTop + padBottom );

	// Phase 10: Read justify-items from container for resolving auto
	CSSJustifyItems containerJItems = CSSJustifyItems::Normal;
	if ( gridWidget )
		containerJItems = gridWidget->getJustifyItems();

	// Position items with alignment
	for ( auto& item : mItems ) {
		if ( item.isTextNode )
			continue;

		int cs = item.resolvedColumnStart - 1;
		int ce = item.resolvedColumnEnd - 1;
		int rs = item.resolvedRowStart - 1;
		int re = item.resolvedRowEnd - 1;

		if ( cs < 0 )
			cs = 0;
		if ( ce >= static_cast<int>( colLines.size() ) )
			ce = static_cast<int>( colLines.size() ) - 1;
		if ( rs < 0 )
			rs = 0;
		if ( re >= static_cast<int>( rowLines.size() ) )
			re = static_cast<int>( rowLines.size() ) - 1;

		Float cellX = colLines[cs] + colStartX + cs * colGapX;
		Float cellY = rowLines[rs] + rowStartY + rs * rowGapY;
		Float cellW = colLines[ce] - colLines[cs];
		Float cellH = rowLines[re] - rowLines[rs];
		// Add internal gaps for multi-span items
		if ( ce > cs + 1 )
			cellW += static_cast<Float>( ce - cs - 1 ) * colGapX;
		if ( re > rs + 1 )
			cellH += static_cast<Float>( re - rs - 1 ) * rowGapY;

		// Determine effective alignment
		CSSJustifySelf js = item.justifySelf;
		if ( js == CSSJustifySelf::Auto ) {
			js = ( containerJItems == CSSJustifyItems::Normal ||
				   containerJItems == CSSJustifyItems::Stretch )
					 ? CSSJustifySelf::Stretch
					 : static_cast<CSSJustifySelf>( containerJItems );
		}
		CSSAlignSelf as = item.alignSelf;
		if ( as == CSSAlignSelf::Auto )
			as = CSSAlignSelf::Stretch;

		// Apply alignment
		Float finalX = cellX, finalY = cellY, finalW = cellW, finalH = cellH;
		if ( js == CSSJustifySelf::Stretch ) {
			finalW = cellW;
			finalX = cellX;
		} else if ( js == CSSJustifySelf::Center ) {
			Float iw = item.widget->getPixelsSize().getWidth();
			finalW = iw;
			finalX = cellX + ( cellW - iw ) * 0.5f;
		} else if ( js == CSSJustifySelf::End || js == CSSJustifySelf::FlexEnd ) {
			Float iw = item.widget->getPixelsSize().getWidth();
			finalW = iw;
			finalX = cellX + cellW - iw;
		} else {
			finalW = item.widget->getPixelsSize().getWidth();
		}

		if ( as == CSSAlignSelf::Stretch ) {
			finalH = cellH;
			finalY = cellY;
		} else if ( as == CSSAlignSelf::Center ) {
			Float ih = item.widget->getPixelsSize().getHeight();
			finalH = ih;
			finalY = cellY + ( cellH - ih ) * 0.5f;
		} else if ( as == CSSAlignSelf::FlexEnd ) {
			Float ih = item.widget->getPixelsSize().getHeight();
			finalH = ih;
			finalY = cellY + cellH - ih;
		} else {
			finalH = item.widget->getPixelsSize().getHeight();
		}

		item.widget->setPixelsPosition( finalX, finalY );
		// Set both width and height to Fixed so the card's own BlockLayouter
		// won't override the grid-placed cell dimensions via
		// setMatchParentIfNeededVerticalGrowth / WrapContent height computation.
		item.widget->setLayoutWidthPolicy( SizePolicy::Fixed );
		item.widget->setLayoutHeightPolicy( SizePolicy::Fixed );
		item.widget->setPixelsSize( finalW, finalH );
	}
}

void GridLayouter::updateLayout() {
	if ( mPacking )
		return;

	mPacking = true;
	struct PackingGuard {
		bool& flag;
		~PackingGuard() { flag = false; }
	} guard{ mPacking };
	mIntrinsicWidthsDirty = false;

	UIHTMLWidget* grid =
		mContainer->isType( UI_TYPE_HTML_WIDGET ) ? mContainer->asType<UIHTMLWidget>() : nullptr;
	if ( grid && grid->getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
		Float matchWidth = grid->getMatchParentWidth();
		if ( matchWidth > 0.f && eeabs( matchWidth - grid->getPixelsSize().getWidth() ) > 0.01f )
			grid->setInternalPixelsWidth( matchWidth );
	}
	readContainerStyle();
	collectGridItems();
	resolveDefinitePlacements();
	autoPlaceItems();
	buildImplicitTracks();
	collapseEmptyAutoFitTracks();

	int needResize = true;
	bool hPercentUnresolved = false;
	bool wPercentUnresolved = false;
	if ( grid && grid->getUIStyle() ) {
		const StyleSheetProperty* hprop = grid->getUIStyle()->getProperty( PropertyId::Height );
		hPercentUnresolved = hprop && StyleSheetLength::isPercentage( hprop->value() ) &&
							 grid->getPixelsSize().getHeight() <= 0.f;
		const StyleSheetProperty* wprop = grid->getUIStyle()->getProperty( PropertyId::Width );
		wPercentUnresolved = wprop && StyleSheetLength::isPercentage( wprop->value() ) &&
							 grid->getPixelsSize().getWidth() <= 0.f;
	}
	for ( int sizingPass = 0; sizingPass < 2 && needResize; ++sizingPass ) {
		needResize = false;
		sizeTracksForAxis( true );
		preSizeItemsForRowSizing();
		sizeTracksForAxis( false );
		applyLayout();

		if ( hPercentUnresolved ) {
			Float contentH = 0.f;
			for ( const auto& row : mRows )
				contentH += row.baseSize;
			if ( mRows.size() > 1 )
				contentH += static_cast<Float>( mRows.size() - 1 ) * mRowGap;
			contentH += mContainer->getPixelsContentOffset().Top +
						mContainer->getPixelsContentOffset().Bottom;
			if ( contentH > 0.f ) {
				mContainer->setInternalPixelsHeight( contentH );
				needResize = true;
			}
		}

		if ( wPercentUnresolved ) {
			const StyleSheetProperty* wprop = grid->getUIStyle()->getProperty( PropertyId::Width );
			std::string v = wprop->value();
			v.pop_back();
			Float pct = 0.f;
			String::fromString( pct, v );
			Float best = 0.f;
			for ( Node* anc = grid->getParent(); anc; anc = anc->getParent() ) {
				if ( anc->isWidget() ) {
					Float sz = anc->asType<UIWidget>()->getPixelsSize().getWidth();
					if ( sz > best )
						best = sz;
				}
			}
			UISceneNode* root = grid->getUISceneNode();
			if ( root ) {
				Float sz = root->getPixelsSize().getWidth();
				if ( sz > best )
					best = sz;
			}
			Float resolved = best * pct / 100.f;
			if ( resolved > 0.f ) {
				mContainer->setInternalPixelsWidth(
					grid->cssResolvedLengthToBorderBoxWidth( resolved ) );
				needResize = true;
			}
		}
	}

	// Phase 15: set paint-order sort flag
	if ( grid ) {
		bool needsSort = false;
		for ( size_t i = 1; i < mItems.size(); ++i ) {
			if ( mItems[i].order != mItems[0].order ) {
				needsSort = true;
				break;
			}
		}
		grid->setNeedsOrderSort( needsSort );
	}

	// Phase 11: compute container baseline from first in-flow item
	mContainerBaseline = 0.f;
	for ( const auto& item : mItems ) {
		if ( !item.isTextNode && !item.isOutOfFlow ) {
			Float b = item.widget->getPixelsPosition().y + item.widget->getPixelsSize().getHeight();
			if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) ) {
				b = item.widget->getPixelsPosition().y +
					item.widget->asType<UIHTMLWidget>()->getBaseline();
			}
			mContainerBaseline = b;
			break;
		}
	}
}

void GridLayouter::collectGridItems() {
	mItems.clear();

	for ( Node* child = mContainer->getFirstChild(); child; child = child->getNextNode() ) {
		// Skip non-widget non-text nodes
		if ( !child->isWidget() && !child->isTextNode() )
			continue;

		// Text node handling
		if ( child->isTextNode() ) {
			UITextNode* textNode = child->asType<UITextNode>();
			if ( textNode->isWhitespaceOnly() )
				continue;
			GridItem item;
			item.widget = textNode;
			item.isTextNode = true;
			mItems.push_back( item );
			continue;
		}

		// Widget handling
		UIWidget* widget = child->asType<UIWidget>();

		// Skip display: none
		if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
			UIHTMLWidget* htmlChild = widget->asType<UIHTMLWidget>();
			if ( htmlChild->getDisplay() == CSSDisplay::None )
				continue;
			if ( htmlChild->isOutOfFlow() )
				continue;
		} else {
			// Regular widget: skip if not visible
			if ( !widget->isVisible() )
				continue;
		}

		GridItem item;
		item.widget = widget;

		if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
			UIHTMLWidget* htmlChild = widget->asType<UIHTMLWidget>();
			item.order = htmlChild->getOrder();
			item.gridRowStart = htmlChild->getGridRowStart();
			item.gridRowEnd = htmlChild->getGridRowEnd();
			item.gridColumnStart = htmlChild->getGridColumnStart();
			item.gridColumnEnd = htmlChild->getGridColumnEnd();
			item.justifySelf = htmlChild->getJustifySelf();
			item.alignSelf = htmlChild->getAlignSelf();
		}

		mItems.push_back( item );
	}

	// Sort by order (stable sort preserves source order for equal orders)
	if ( mItems.size() > 1 ) {
		std::stable_sort( mItems.begin(), mItems.end(), []( const GridItem& a, const GridItem& b ) {
			return a.order < b.order;
		} );
	}
}

// ── Grid line placement helpers ──

static GridLineResult parseGridLine( const std::string& raw ) {
	GridLineResult result;
	std::string trimmed = String::trim( raw );
	String::toLowerInPlace( trimmed );

	if ( trimmed.empty() || trimmed == "auto" )
		return result;

	int intVal = 0;
	if ( String::fromString( intVal, trimmed ) ) {
		result.isAuto = false;
		result.lineNumber = intVal;
		return result;
	}

	if ( trimmed == "span" ) {
		result.isAuto = false;
		result.isSpan = true;
		return result;
	}

	if ( String::startsWith( trimmed, "span " ) ) {
		result.isAuto = false;
		result.isSpan = true;
		std::string rest = String::trim( trimmed.substr( 5 ) );
		if ( !rest.empty() && String::fromString( intVal, rest ) ) {
			result.spanCount = intVal;
		}
		return result;
	}

	// <integer> <custom-ident>
	size_t sp = trimmed.find( ' ' );
	if ( sp != std::string::npos ) {
		std::string first = trimmed.substr( 0, sp );
		std::string second = String::trim( trimmed.substr( sp + 1 ) );
		if ( String::fromString( intVal, first ) ) {
			result.isAuto = false;
			result.lineNumber = intVal;
			if ( !second.empty() )
				result.nameId = second;
			return result;
		}
	}

	// <custom-ident> alone — store as named line reference
	result.isAuto = false;
	result.nameId = trimmed;
	return result;
}

GridLineResult GridLineParser::parse( const std::string& value ) {
	return parseGridLine( value );
}

static void resolveAxisPlacement( const GridLineResult& start, const GridLineResult& end,
								  int explicitCount, int& outStart, int& outEnd ) {
	if ( start.isAuto && end.isAuto )
		return;

	auto normalize = [explicitCount]( int val ) -> int {
		if ( val < 0 ) {
			int res = explicitCount + 2 + val;
			return res < 1 ? 1 : res;
		}
		return val;
	};

	if ( start.isSpan && end.isSpan ) {
		// Both are span — only the end span is honored (start treated as auto)
		outStart = 0;
		outEnd = end.spanCount;
		return;
	}

	// start auto, end span N: keep start as auto, store span count as end
	if ( start.isAuto && end.isSpan ) {
		outStart = 0;
		outEnd = end.spanCount;
		return;
	}

	// start span N, end auto: keep end as auto, store span count as start
	if ( start.isSpan && end.isAuto ) {
		outStart = start.spanCount;
		outEnd = 0;
		return;
	}

	// Both definite (not auto, not span)
	if ( !start.isAuto && !end.isAuto && !start.isSpan && !end.isSpan ) {
		int s = normalize( start.lineNumber );
		int e = normalize( end.lineNumber );
		if ( s == e )
			++e;
		if ( s > e )
			std::swap( s, e );
		outStart = s;
		outEnd = e;
		return;
	}

	// Start definite, end auto → span 1 track
	if ( !start.isAuto && !start.isSpan && end.isAuto ) {
		outStart = normalize( start.lineNumber );
		outEnd = outStart + 1;
		return;
	}

	// Start auto, end definite → span backwards
	if ( start.isAuto && !end.isAuto && !end.isSpan ) {
		outEnd = normalize( end.lineNumber );
		outStart = outEnd - 1;
		if ( outStart < 1 )
			outStart = 1;
		return;
	}

	// Start definite, end span N
	if ( !start.isAuto && !start.isSpan && end.isSpan ) {
		outStart = normalize( start.lineNumber );
		outEnd = outStart + end.spanCount;
		return;
	}

	// Start span N, end definite
	if ( start.isSpan && !end.isAuto && !end.isSpan ) {
		outEnd = normalize( end.lineNumber );
		outStart = outEnd - start.spanCount;
		return;
	}
}

void GridLayouter::resolveDefinitePlacements() {
	int explicitCols =
		std::max( static_cast<int>( mExplicitColCount ), static_cast<int>( mColumns.size() ) );
	int explicitRows =
		std::max( static_cast<int>( mExplicitRowCount ), static_cast<int>( mRows.size() ) );

	mMaxColumn = explicitCols;
	mMaxRow = explicitRows;

	auto resolveNamedLine = []( const UnorderedMap<std::string, std::vector<int>>& lineNames,
								const std::string& nameId, int lineNumber,
								int explicitCount ) -> int {
		auto it = lineNames.find( nameId );
		if ( it == lineNames.end() || it->second.empty() )
			return 0; // unresolved

		const auto& indices = it->second;
		if ( lineNumber == 0 ) {
			// Just the name — return first matching line
			return indices[0];
		}

		// <integer> <custom-ident> — find the Nth line of this name
		if ( lineNumber > 0 ) {
			if ( lineNumber <= static_cast<int>( indices.size() ) )
				return indices[lineNumber - 1];
		} else {
			// Negative: count from end
			int idx = explicitCount + 2 + lineNumber; // line 0 = explicitCount+2
			// Clamp to valid range
			if ( idx < 1 )
				idx = 1;
			return idx;
		}
		return 0;
	};

	for ( auto& item : mItems ) {
		GridLineResult startCol = parseGridLine( item.gridColumnStart );
		GridLineResult endCol = parseGridLine( item.gridColumnEnd );
		GridLineResult startRow = parseGridLine( item.gridRowStart );
		GridLineResult endRow = parseGridLine( item.gridRowEnd );

		// Resolve named lines to line numbers
		if ( !startCol.nameId.empty() )
			startCol.lineNumber = resolveNamedLine( mColLineNames, startCol.nameId,
													startCol.lineNumber, explicitCols );
		if ( !endCol.nameId.empty() )
			endCol.lineNumber =
				resolveNamedLine( mColLineNames, endCol.nameId, endCol.lineNumber, explicitCols );
		if ( !startRow.nameId.empty() )
			startRow.lineNumber = resolveNamedLine( mRowLineNames, startRow.nameId,
													startRow.lineNumber, explicitRows );
		if ( !endRow.nameId.empty() )
			endRow.lineNumber =
				resolveNamedLine( mRowLineNames, endRow.nameId, endRow.lineNumber, explicitRows );

		resolveAxisPlacement( startCol, endCol, explicitCols, item.resolvedColumnStart,
							  item.resolvedColumnEnd );
		resolveAxisPlacement( startRow, endRow, explicitRows, item.resolvedRowStart,
							  item.resolvedRowEnd );

		// Normalize span fields: resolvedEnd can be overloaded as a span count
		// when the start is auto. Extract into explicit rowSpan/columnSpan.
		auto normalizeSpan = []( int& resolvedStart, int& resolvedEnd, int& outSpan ) {
			if ( resolvedStart > 0 && resolvedEnd > 0 ) {
				outSpan = resolvedEnd - resolvedStart;
			} else if ( resolvedStart > 0 && resolvedEnd == 0 ) {
				outSpan = resolvedStart;
				resolvedStart = 0;
			} else if ( resolvedStart == 0 && resolvedEnd > 0 ) {
				outSpan = resolvedEnd;
				resolvedEnd = 0;
			}
			// else both 0: outSpan stays default 1
		};
		normalizeSpan( item.resolvedColumnStart, item.resolvedColumnEnd, item.columnSpan );
		normalizeSpan( item.resolvedRowStart, item.resolvedRowEnd, item.rowSpan );

		if ( item.resolvedColumnEnd > mMaxColumn )
			mMaxColumn = item.resolvedColumnEnd;
		if ( item.resolvedRowEnd > mMaxRow )
			mMaxRow = item.resolvedRowEnd;
		// Auto axes with span: need at least span columns/rows for implicit tracks
		if ( item.resolvedColumnStart == 0 && item.columnSpan > mMaxColumn )
			mMaxColumn = item.columnSpan;
		if ( item.resolvedRowStart == 0 && item.rowSpan > mMaxRow )
			mMaxRow = item.rowSpan;
	}
}

void GridLayouter::autoPlaceItems() {
	int maxColumns = mMaxColumn;
	int maxRows = mMaxRow;

	std::set<std::pair<int, int>> occupied;
	for ( const auto& item : mItems ) {
		if ( item.resolvedColumnStart > 0 && item.resolvedRowStart > 0 ) {
			for ( int r = item.resolvedRowStart; r < item.resolvedRowEnd; ++r )
				for ( int c = item.resolvedColumnStart; c < item.resolvedColumnEnd; ++c )
					occupied.insert( { r, c } );
		}
	}

	int cursorCol = 1;
	int cursorRow = 1;
	bool isRowFlow = mAutoFlow == CSSGridAutoFlow::Row;

	auto occupyRange = [&]( int r, int c, int w, int h ) {
		for ( int dr = 0; dr < h; ++dr )
			for ( int dc = 0; dc < w; ++dc )
				occupied.insert( { r + dr, c + dc } );
	};

	auto fits = [&]( int r, int c, int w, int h ) {
		for ( int dr = 0; dr < h; ++dr )
			for ( int dc = 0; dc < w; ++dc )
				if ( occupied.find( { r + dr, c + dc } ) != occupied.end() )
					return false;
		return true;
	};

	for ( auto& item : mItems ) {
		bool colAuto = item.resolvedColumnStart == 0;
		bool rowAuto = item.resolvedRowStart == 0;

		if ( !colAuto && !rowAuto )
			continue;

		int sw = item.columnSpan;
		int sh = item.rowSpan;

		if ( !colAuto && rowAuto ) {
			int c = item.resolvedColumnStart;
			for ( int r = mAutoFlowDense ? 1 : cursorRow;; ++r ) {
				if ( fits( r, c, sw, sh ) ) {
					item.resolvedRowStart = r;
					item.resolvedRowEnd = r + sh;
					occupyRange( r, c, sw, sh );
					cursorRow = r;
					break;
				}
			}
		} else if ( !rowAuto && colAuto ) {
			int r = item.resolvedRowStart;
			for ( int c = mAutoFlowDense ? 1 : cursorCol;; ++c ) {
				if ( fits( r, c, sw, sh ) ) {
					item.resolvedColumnStart = c;
					item.resolvedColumnEnd = c + sw;
					occupyRange( r, c, sw, sh );
					cursorCol = c;
					break;
				}
			}
		} else {
			// Both auto — row-flow default
			if ( isRowFlow ) {
				bool placed = false;
				if ( mAutoFlowDense ) {
					// Dense: search from the beginning for first empty cell
					for ( int r = 1;; ++r ) {
						for ( int c = 1; c <= std::max( maxColumns, 1 ); ++c ) {
							if ( fits( r, c, sw, sh ) ) {
								item.resolvedRowStart = r;
								item.resolvedRowEnd = r + sh;
								item.resolvedColumnStart = c;
								item.resolvedColumnEnd = c + sw;
								occupyRange( r, c, sw, sh );
								cursorRow = r;
								cursorCol = c;
								placed = true;
								break;
							}
						}
						if ( placed )
							break;
					}
				} else {
					for ( int r = cursorRow;; ++r ) {
						int startC = ( r == cursorRow ) ? cursorCol : 1;
						for ( int c = startC; c <= std::max( maxColumns, 1 ); ++c ) {
							if ( fits( r, c, sw, sh ) ) {
								item.resolvedRowStart = r;
								item.resolvedRowEnd = r + sh;
								item.resolvedColumnStart = c;
								item.resolvedColumnEnd = c + sw;
								occupyRange( r, c, sw, sh );
								cursorRow = r;
								cursorCol = c;
								placed = true;
								break;
							}
						}
						if ( placed )
							break;
					}
				}
			} else {
				// Column flow
				bool placed = false;
				if ( mAutoFlowDense ) {
					for ( int c = 1;; ++c ) {
						for ( int r = 1; r <= std::max( maxRows, 1 ); ++r ) {
							if ( fits( r, c, sw, sh ) ) {
								item.resolvedRowStart = r;
								item.resolvedRowEnd = r + sh;
								item.resolvedColumnStart = c;
								item.resolvedColumnEnd = c + sw;
								occupyRange( r, c, sw, sh );
								cursorRow = r;
								cursorCol = c;
								placed = true;
								break;
							}
						}
						if ( placed )
							break;
					}
				} else {
					for ( int c = cursorCol;; ++c ) {
						int startR = ( c == cursorCol ) ? cursorRow : 1;
						int rowLimit = std::max( maxRows, 1 );
						for ( int r = startR; r <= rowLimit; ++r ) {
							if ( fits( r, c, sw, sh ) ) {
								item.resolvedRowStart = r;
								item.resolvedRowEnd = r + sh;
								item.resolvedColumnStart = c;
								item.resolvedColumnEnd = c + sw;
								occupyRange( r, c, sw, sh );
								cursorRow = r;
								cursorCol = c;
								placed = true;
								break;
							}
						}
						if ( placed )
							break;
					}
				}
			}
		}

		// Only update member fields — the local snapshot must NOT grow
		// during auto-placement, otherwise later items overflow into
		// implicit columns instead of wrapping to the next row.
		if ( item.resolvedColumnEnd > mMaxColumn )
			mMaxColumn = item.resolvedColumnEnd;
		if ( item.resolvedRowEnd > mMaxRow )
			mMaxRow = item.resolvedRowEnd;
	}
}

void GridLayouter::computeIntrinsicWidths() {
	mIntrinsicWidthsDirty = false;
	mMinIntrinsicWidth = 0.f;
	mMaxIntrinsicWidth = 0.f;

	UIHTMLWidget* grid =
		mContainer->isType( UI_TYPE_HTML_WIDGET ) ? mContainer->asType<UIHTMLWidget>() : nullptr;
	if ( !grid )
		return;

	Float gap = mContainer->lengthFromValue(
		grid->getColumnGap(), CSS::PropertyRelativeTarget::ContainingBlockWidth, 0.f );

	GridTrackList colList = GridTrackParser::parseTrackList( grid->getGridTemplateColumns() );
	if ( !colList.valid || colList.none )
		return;

	// Expand auto-repeat tracks (use a large available size as heuristic)
	if ( colList.hasAutoRepeat && !colList.autoRepeatTemplate.empty() ) {
		Float intrinsicExpandSize = 10000.f; // large enough to get a meaningful count
		resolveAutoRepeatLengths( colList, mContainer,
								  CSS::PropertyRelativeTarget::ContainingBlockWidth );
		GridTrackParser::expandAutoRepeat( colList, intrinsicExpandSize, gap );
	}

	Float minW = 0.f;
	Float maxW = 0.f;
	for ( size_t i = 0; i < colList.tracks.size(); ++i ) {
		const auto& size = colList.tracks[i].size;
		Float trackMin = 0.f;
		Float trackMax = 0.f;

		switch ( size.min.type ) {
			case GridTrackBreadthType::Length:
				trackMin = size.min.value;
				break;
			case GridTrackBreadthType::Percentage:
				trackMin = 0.f;
				break;
			case GridTrackBreadthType::Flex:
				trackMin = 0.f;
				break;
			default:
				trackMin = 0.f;
				break;
		}

		switch ( size.max.type ) {
			case GridTrackBreadthType::Length:
				trackMax = size.max.value;
				break;
			case GridTrackBreadthType::Flex:
				trackMax = 0.f;
				break;
			default:
				trackMax = trackMin;
				break;
		}

		if ( trackMax < trackMin )
			trackMax = trackMin;

		minW += trackMin;
		maxW += trackMax;

		if ( i + 1 < colList.tracks.size() ) {
			minW += gap;
			maxW += gap;
		}
	}

	mMinIntrinsicWidth = minW;
	mMaxIntrinsicWidth = maxW;
}

Float GridLayouter::getMinIntrinsicWidth() {
	if ( mIntrinsicWidthsDirty )
		computeIntrinsicWidths();
	return mMinIntrinsicWidth;
}

Float GridLayouter::getMaxIntrinsicWidth() {
	if ( mIntrinsicWidthsDirty )
		computeIntrinsicWidths();
	return mMaxIntrinsicWidth;
}

// ── GridAreasParser implementation ──

GridAreaTemplate GridAreasParser::parseAreas( const std::string& value ) {
	GridAreaTemplate result;
	std::string trimmed = String::trim( value );
	String::toLowerInPlace( trimmed );

	if ( trimmed == "none" ) {
		result.none = true;
		return result;
	}

	// Extract quoted strings as rows; fallback to newline splitting.
	std::vector<std::string> rows;
	if ( trimmed.find( '"' ) != std::string::npos ) {
		size_t pos = 0;
		while ( pos < trimmed.size() ) {
			size_t open = trimmed.find( '"', pos );
			if ( open == std::string::npos )
				break;
			size_t close = trimmed.find( '"', open + 1 );
			if ( close == std::string::npos ) {
				result.valid = false;
				return result;
			}
			rows.push_back( trimmed.substr( open + 1, close - open - 1 ) );
			pos = close + 1;
		}
	} else {
		rows = String::split( trimmed, '\n' );
	}

	if ( rows.empty() ) {
		result.none = true;
		return result;
	}

	// Tokenize each row and validate equal widths.
	std::vector<std::vector<std::string>> grid;
	size_t expectedCols = 0;
	for ( const auto& rowStr : rows ) {
		std::string clean = String::trim( rowStr );
		if ( clean.empty() )
			continue;
		std::vector<std::string> tokens = String::split( clean, ' ' );
		for ( auto& t : tokens )
			t = String::trim( t );
		tokens.erase( std::remove_if( tokens.begin(), tokens.end(),
									  []( const std::string& s ) { return s.empty(); } ),
					  tokens.end() );
		if ( expectedCols == 0 )
			expectedCols = tokens.size();
		if ( tokens.size() != expectedCols ) {
			result.valid = false;
			return result;
		}
		grid.push_back( std::move( tokens ) );
	}

	if ( grid.empty() ) {
		result.none = true;
		return result;
	}

	// Collect cells per area.
	UnorderedMap<std::string, std::vector<std::pair<int, int>>> cells; // row, col (0-based)
	for ( size_t r = 0; r < grid.size(); ++r ) {
		for ( size_t c = 0; c < grid[r].size(); ++c ) {
			const std::string& tok = grid[r][c];
			if ( tok == "." )
				continue;
			cells[tok].push_back( { static_cast<int>( r ), static_cast<int>( c ) } );
		}
	}

	// Validate each area is a filled rectangle.
	for ( const auto& kv : cells ) {
		const auto& coords = kv.second;
		int minR = coords[0].first, maxR = coords[0].first;
		int minC = coords[0].second, maxC = coords[0].second;
		for ( const auto& p : coords ) {
			minR = std::min( minR, p.first );
			maxR = std::max( maxR, p.first );
			minC = std::min( minC, p.second );
			maxC = std::max( maxC, p.second );
		}

		int expectedCount = ( maxR - minR + 1 ) * ( maxC - minC + 1 );
		if ( static_cast<int>( coords.size() ) != expectedCount ) {
			result.valid = false;
			return result;
		}

		GridAreaTemplate::Area area;
		area.startRow = minR + 1; // convert to 1-based lines
		area.endRow = maxR + 2;	  // exclusive
		area.startColumn = minC + 1;
		area.endColumn = maxC + 2;
		result.areas[kv.first] = area;
	}

	return result;
}

bool GridAreasParser::isValidAreas( const std::string& value ) {
	GridAreaTemplate tmpl = parseAreas( value );
	return tmpl.valid;
}

std::vector<std::string> GridAreasParser::getLineNamesForRowLine( const GridAreaTemplate& tmpl,
																  int line ) {
	std::vector<std::string> names;
	for ( const auto& kv : tmpl.areas ) {
		if ( kv.second.startRow == line )
			names.push_back( kv.first + "-start" );
		if ( kv.second.endRow == line )
			names.push_back( kv.first + "-end" );
	}
	return names;
}

std::vector<std::string> GridAreasParser::getLineNamesForColumnLine( const GridAreaTemplate& tmpl,
																	 int line ) {
	std::vector<std::string> names;
	for ( const auto& kv : tmpl.areas ) {
		if ( kv.second.startColumn == line )
			names.push_back( kv.first + "-start" );
		if ( kv.second.endColumn == line )
			names.push_back( kv.first + "-end" );
	}
	return names;
}

}} // namespace EE::UI
