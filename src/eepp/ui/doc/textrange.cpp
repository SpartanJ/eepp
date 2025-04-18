#include <eepp/core/debug.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <limits>

namespace EE { namespace UI { namespace Doc {

TextRange::TextRange() {}

TextRange::TextRange( const TextPosition& start, const TextPosition& end ) :
	mStart( start ), mEnd( end ) {}

bool TextRange::isValid() const {
	return mStart.isValid() && mEnd.isValid();
}

void TextRange::clear() {
	mStart = {};
	mEnd = {};
}

TextRange TextRange::normalized() const {
	return TextRange( normalizedStart(), normalizedEnd() );
}

TextRange& TextRange::normalize() {
	auto normalize( normalized() );
	mStart = normalize.start();
	mEnd = normalize.end();
	return *this;
}

void TextRange::set( const TextPosition& start, const TextPosition& end ) {
	mStart = start;
	mEnd = end;
}

bool TextRange::contains( const TextPosition& position ) const {
	if ( !( position.line() > mStart.line() ||
			( position.line() == mStart.line() && position.column() >= mStart.column() ) ) )
		return false;
	if ( !( position.line() < mEnd.line() ||
			( position.line() == mEnd.line() && position.column() <= mEnd.column() ) ) )
		return false;
	return true;
}

bool TextRange::intersectsLineRange( const TextRange& range ) const {
	eeASSERT( range.isNormalized() );
	return mStart.line() <= static_cast<Int64>( range.end().line() ) &&
		   static_cast<Int64>( range.start().line() ) <= mEnd.line();
}

bool TextRange::containsLine( const Int64& line ) const {
	return line >= mStart.line() && line <= mEnd.line();
}

bool TextRange::contains( const TextRange& range ) const {
	return range.start() >= start() && range.end() <= end();
}

bool TextRange::intersects( const TextRange& range ) const {
	return range.start() <= end() && range.end() >= start();
}

TextRange TextRange::merge( const TextRange& range ) const {
	bool wasNormalized = mStart <= mEnd;
	TextRange normalizedThis = this->normalized();
	TextRange normalizedRange = range.normalized();
	TextPosition mergedStart = normalizedThis.start() < normalizedRange.start()
								   ? normalizedThis.start()
								   : normalizedRange.start();
	TextPosition mergedEnd =
		normalizedThis.end() > normalizedRange.end() ? normalizedThis.end() : normalizedRange.end();
	return TextRange( wasNormalized ? mergedStart : mergedEnd,
					  wasNormalized ? mergedEnd : mergedStart );
}

Int64 TextRange::height() const {
	if ( mEnd.line() > mStart.line() )
		return mEnd.line() - mStart.line() + 1;
	return mStart.line() - mStart.line() + 1;
}

Int64 TextRange::length() const {
	if ( !inSameLine() )
		return 0;
	if ( mEnd.column() > mStart.column() )
		return mEnd.column() - mStart.column();
	return mStart.column() - mEnd.column();
}

std::string TextRange::toString() const {
	return String::format( "%s - %s", mStart.toString().c_str(), mEnd.toString().c_str() );
}

TextRange TextRange::fromString( const std::string& range ) {
	auto split = String::split( range, "-" );
	if ( split.size() == 2 ) {
		return { TextPosition::fromString( String::trim( split[0] ) ),
				 TextPosition::fromString( String::trim( split[1] ) ) };
	}
	return {};
}

template <typename StringType>
TextRange TextRange::convertToLineColumn( const StringType& text, Int64 startOffset,
										  Int64 endOffset ) {
	if ( startOffset < 0 || endOffset < 0 || startOffset > static_cast<Int64>( text.length() ) ||
		 endOffset > static_cast<Int64>( text.length() ) || startOffset > endOffset ) {
		return {};
	}

	TextRange result;
	Int64 currentLine = 0;
	Int64 currentCol = 0;
	Int64 currentPos = 0;
	bool foundStart = false;
	bool foundEnd = false;
	size_t len = text.length();

	for ( size_t i = 0; i < len; i++ ) {
		if ( currentPos == startOffset ) {
			result.setStart( { currentLine, currentCol } );
			foundStart = true;
		}

		if ( currentPos == endOffset ) {
			result.setEnd( { currentLine, currentCol } );
			foundEnd = true;
		}

		if ( foundStart && foundEnd )
			break;

		if ( i == text.length() )
			break;

		if ( text[i] == '\n' ) {
			currentLine++;
			currentCol = 0;
		} else {
			currentCol++;
		}

		currentPos++;
	}

	if ( currentPos == startOffset )
		result.setStart( { currentLine, currentCol } );

	if ( currentPos == endOffset )
		result.setEnd( { currentLine, currentCol } );

	return result;
}

TextRange TextRange::convertToLineColumn( const String::View& text, Int64 startOffset,
										  Int64 endOffset ) {
	return convertToLineColumn<String>( text, startOffset, endOffset );
}

TextRange TextRange::convertToLineColumn( const std::string_view& text, Int64 startOffset,
										  Int64 endOffset ) {
	return convertToLineColumn<std::string_view>( text, startOffset, endOffset );
}

Int64 TextRange::minimumDistance( const TextRange& other ) const {
	if ( intersects( other ) )
		return 0;

	auto minDist = std::numeric_limits<Int64>::max();

	const TextPosition corners1[] = { mStart, mEnd };
	const TextPosition corners2[] = { other.start(), other.end() };

	for ( const auto& c1 : corners1 ) {
		for ( const auto& c2 : corners2 ) {
			auto dist = c1.distance( c2 );
			minDist = std::min( minDist, dist );
		}
	}

	return minDist;
}

TextRanges::TextRanges() {}

TextRanges::TextRanges( const std::vector<TextRange>& ranges ) : std::vector<TextRange>( ranges ) {}

TextRanges::TextRanges( const TextRange& ranges ) : std::vector<TextRange>( { ranges } ) {}

bool TextRanges::isSorted() const {
	return mIsSorted;
}

bool TextRanges::isValid() const {
	for ( const auto& selection : *this ) {
		if ( !selection.isValid() )
			return false;
	}
	return true;
}

bool TextRanges::exists( const TextRange& range ) const {
	if ( !mIsSorted )
		return std::find( begin(), end(), range ) != end();
	return std::binary_search( begin(), end(), range );
}

size_t TextRanges::findIndex( const TextRange& range ) const {
	if ( !mIsSorted ) {
		auto it = std::find( begin(), end(), range );
		return it != end() ? std::distance( begin(), it ) : static_cast<size_t>( -1 );
	} else {
		auto it = std::lower_bound( begin(), end(), range );
		return ( it != end() && *it == range ) ? std::distance( begin(), it )
											   : static_cast<size_t>( -1 );
	}
}

bool TextRanges::hasSelection() const {
	for ( const auto& r : *this )
		if ( r.hasSelection() )
			return true;
	return false;
}

void TextRanges::sort() {
	std::sort( begin(), end() );
	setSorted();
}

void TextRanges::setSorted() {
	mIsSorted = true;
}

bool TextRanges::merge() {
	if ( size() <= 1 )
		return false;

	if ( !mIsSorted )
		sort();

	auto itUnique = std::unique( begin(), end(),
								 []( const TextRange& a, const TextRange& b ) { return a == b; } );

	bool merged = itUnique != end();
	erase( itUnique, end() );

	for ( auto it = begin(); it != end(); ++it ) {
		auto next = std::next( it );
		while ( next != end() &&
				( it->intersects( *next ) || it->contains( *next ) || next->contains( *it ) ) ) {
			*it = it->merge( *next );
			next = erase( next );
			merged = true;
		}
	}

	return merged;
}

std::string TextRanges::toString() const {
	std::string str;
	for ( size_t i = 0; i < size(); ++i ) {
		str += ( *this )[i].toString();
		if ( i != size() - 1 )
			str += ";";
	}
	return str;
}

TextRanges TextRanges::fromString( const std::string& str ) {
	auto rangesStr = String::split( str, ';' );
	TextRanges ranges;
	for ( const auto& rangeStr : rangesStr )
		ranges.emplace_back( TextRange::fromString( rangeStr ) );
	return ranges;
}

}}} // namespace EE::UI::Doc
