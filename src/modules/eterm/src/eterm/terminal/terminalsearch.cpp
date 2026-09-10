#include <eterm/terminal/terminalsearch.hpp>

#include <eepp/system/luapattern.hpp>
#include <eepp/system/regex.hpp>

#include <optional>

using namespace EE::System;

namespace eterm { namespace Terminal {

static bool isWordCharacter( Rune rune ) {
	return rune == '_' || String::isAlphaNum( rune );
}

template <typename Positions>
static void searchPattern( PatternMatcher& pattern, const std::string& text,
						   const String& logicalLine, const Positions& positions, bool wholeWord,
						   std::vector<TerminalSearchMatch>& matches ) {
	if ( !pattern.isValid() )
		return;

	PatternMatcher::Range ranges[12];
	int offset = 0;
	size_t byteOffset = 0;
	size_t codepointOffset = 0;
	auto codepointPosition = [&]( size_t targetByteOffset ) {
		while ( byteOffset < targetByteOffset ) {
			if ( ( static_cast<unsigned char>( text[byteOffset] ) & 0xC0 ) != 0x80 )
				++codepointOffset;
			++byteOffset;
		}
		return codepointOffset;
	};
	while ( offset <= static_cast<int>( text.size() ) && pattern.matches( text, ranges, offset ) ) {
		const int start = ranges[0].start;
		const int end = ranges[0].end;
		if ( start < 0 || end < start || end > static_cast<int>( text.size() ) )
			break;

		const size_t codepointStart = codepointPosition( start );
		const size_t codepointEnd = codepointPosition( end );
		if ( codepointStart < codepointEnd && codepointEnd <= positions.size() &&
			 ( !wholeWord ||
			   ( ( codepointStart == 0 || !isWordCharacter( logicalLine[codepointStart - 1] ) ) &&
				 ( codepointEnd == logicalLine.size() ||
				   !isWordCharacter( logicalLine[codepointEnd] ) ) ) ) ) {
			matches.push_back(
				{ positions[codepointStart].start, positions[codepointEnd - 1].end } );
		}

		if ( end > offset ) {
			offset = end;
		} else if ( offset < static_cast<int>( text.size() ) ) {
			do {
				++offset;
			} while ( offset < static_cast<int>( text.size() ) &&
					  ( static_cast<unsigned char>( text[offset] ) & 0xC0 ) == 0x80 );
		} else {
			break;
		}
	}
}

void TerminalSearch::searchLogicalLine( const TerminalSearchQuery& query, PatternMatcher* pattern,
										const String& needle ) {
	if ( mText.empty() )
		return;

	if ( pattern ) {
		mText.toUtf8( mUtf8Text );
		searchPattern( *pattern, mUtf8Text, mText, mPositions, query.wholeWord, mMatches );
		return;
	}

	const String* comparableText = &mText;
	if ( !query.caseSensitive ) {
		mComparableText = mText;
		mComparableText.toLower();
		comparableText = &mComparableText;
	}
	if ( needle.empty() || needle.size() > comparableText->size() )
		return;

	size_t offset = 0;
	while ( ( offset = comparableText->find( needle, offset ) ) != String::InvalidPos ) {
		if ( query.wholeWord && ( ( offset > 0 && isWordCharacter( mText[offset - 1] ) ) ||
								  ( offset + needle.size() < mText.size() &&
									isWordCharacter( mText[offset + needle.size()] ) ) ) ) {
			++offset;
			continue;
		}
		mMatches.push_back(
			{ mPositions[offset].start, mPositions[offset + needle.size() - 1].end } );
		++offset;
	}
}

const std::vector<TerminalSearchMatch>&
TerminalSearch::search( const std::vector<TerminalSearchRowView>& rows,
						const TerminalSearchQuery& query ) {
	mMatches.clear();
	mText.clear();
	mPositions.clear();
	if ( !isQuerySearchable( query ) )
		return mMatches;

	String needle;
	if ( query.type == TerminalSearchType::Normal ) {
		needle = query.text;
		if ( !query.caseSensitive )
			needle.toLower();
	}
	std::string patternText;
	if ( query.type != TerminalSearchType::Normal )
		query.text.toUtf8( patternText );
	std::optional<RegEx> regex;
	std::optional<LuaPatternStorage> luaPattern;
	PatternMatcher* pattern = nullptr;
	if ( query.type == TerminalSearchType::RegEx ) {
		regex.emplace( patternText, static_cast<RegEx::Options>(
										RegEx::Options::Utf | RegEx::Options::AllowFallback |
										( query.caseSensitive ? RegEx::Options::None
															  : RegEx::Options::Caseless ) ) );
		pattern = &*regex;
	} else if ( query.type == TerminalSearchType::LuaPattern ) {
		luaPattern.emplace( std::move( patternText ) );
		pattern = &*luaPattern;
	}

	for ( const auto& row : rows ) {
		if ( !row.cells || row.length < 0 || row.width < row.length )
			continue;
		for ( Int32 column = 0; column < row.length; ++column ) {
			const auto& glyph = row.cells[column];
			if ( glyph.mode & ATTR_WDUMMY )
				continue;
			mText += glyph.u;
			mPositions.push_back(
				{ { row.source, row.row, column },
				  { row.source, row.row, column + ( ( glyph.mode & ATTR_WIDE ) ? 1 : 0 ) } } );
		}
		if ( !row.wrapped ) {
			searchLogicalLine( query, pattern, needle );
			mText.clear();
			mPositions.clear();
		}
	}
	if ( !mText.empty() )
		searchLogicalLine( query, pattern, needle );
	return mMatches;
}

}} // namespace eterm::Terminal
