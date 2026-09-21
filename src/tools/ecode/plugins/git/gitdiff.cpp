#include "gitdiff.hpp"
#include <algorithm>
#include <utility>

#include <dtl/dtl.hpp>

namespace ecode {

namespace GitDiffDetail {

class LineSequence {
  public:
	using value_type = std::string_view;
	using iterator = const value_type*;
	using const_iterator = const value_type*;

	LineSequence() = default;

	LineSequence( const value_type* data, std::size_t size ) : mData( data ), mSize( size ) {}

	LineSequence( const_iterator first, const_iterator last ) :
		mData( first ), mSize( first == last ? 0 : static_cast<std::size_t>( last - first ) ) {}

	const_iterator begin() const { return mData; }

	const_iterator end() const { return mSize == 0 ? mData : mData + mSize; }

	const value_type& operator[]( std::size_t index ) const { return mData[index]; }

  private:
	const value_type* mData{ nullptr };
	std::size_t mSize{ 0 };
};

class LineDiffBuilder {
  public:
	explicit LineDiffBuilder( std::size_t currentLineCount ) {
		mResult.lines.reserve( currentLineCount );
	}

	void add( int type ) {
		switch ( type ) {
			case dtl::SES_COMMON:
				finishEditBlock();
				mResult.lines.emplace_back(
					GitLineDecoration{ GitLineChange::None, mDeletedBeforeNextLine } );
				mDeletedBeforeNextLine = false;
				++mCurrentIndex;
				break;
			case dtl::SES_ADD:
				startEditBlock();
				mResult.lines.emplace_back();
				++mCurrentIndex;
				++mAddedCount;
				break;
			case dtl::SES_DELETE:
				startEditBlock();
				++mDeletedCount;
				break;
		}
	}

	ComputedGitLineDiff finish() {
		finishEditBlock();
		mResult.deletedAtEOF = mDeletedBeforeNextLine;

		// TextDocument exposes one logical line for an empty buffer. Keep one decoration slot so an
		// entire-file deletion can still be rendered at that boundary.
		if ( mResult.deletedAtEOF && mResult.lines.empty() )
			mResult.lines.emplace_back();
		return std::move( mResult );
	}

  private:
	void startEditBlock() {
		if ( !mInEditBlock ) {
			mInEditBlock = true;
			mBlockStart = mCurrentIndex;
		}
	}

	void finishEditBlock() {
		if ( !mInEditBlock )
			return;

		const std::size_t modifiedCount = std::min( mDeletedCount, mAddedCount );
		for ( std::size_t i = 0; i < mAddedCount; ++i ) {
			mResult.lines[mBlockStart + i].change =
				i < modifiedCount ? GitLineChange::Modified : GitLineChange::Added;
		}

		if ( mDeletedCount > modifiedCount ) {
			if ( mAddedCount != 0 )
				mResult.lines[mBlockStart].deletedBefore = true;
			else
				mDeletedBeforeNextLine = true;
		}

		mInEditBlock = false;
		mDeletedCount = 0;
		mAddedCount = 0;
	}

	ComputedGitLineDiff mResult;
	std::size_t mCurrentIndex{ 0 };
	std::size_t mBlockStart{ 0 };
	std::size_t mDeletedCount{ 0 };
	std::size_t mAddedCount{ 0 };
	bool mInEditBlock{ false };
	bool mDeletedBeforeNextLine{ false };
};

template <typename SesElement, typename Builder> class StoreLineDiff {
  public:
	explicit StoreLineDiff( Builder& builder ) : mBuilder( builder ) {}

	void operator()( const SesElement& entry ) { mBuilder.add( entry.second.type ); }

  private:
	Builder& mBuilder;
};

std::size_t lineCount( std::string_view text ) {
	if ( text.empty() )
		return 0;
	return 1 + static_cast<std::size_t>( std::count( text.begin(), text.end(), '\n' ) );
}

void appendLines( std::string_view text, std::vector<std::string_view>& lines ) {
	if ( text.empty() )
		return;
	std::size_t start = 0;
	while ( start <= text.size() ) {
		const std::size_t end = text.find( '\n', start );
		lines.emplace_back( text.substr( start, end == std::string_view::npos ? text.size() - start
																			  : end - start ) );
		if ( end == std::string_view::npos )
			break;
		start = end + 1;
	}
}

} // namespace GitDiffDetail

void normalizeGitDiffText( std::string& text ) {
	const std::size_t firstCarriageReturn = text.find( '\r' );
	if ( firstCarriageReturn == std::string::npos )
		return;

	std::size_t write = firstCarriageReturn;
	for ( std::size_t read = firstCarriageReturn; read < text.size(); ++read ) {
		if ( text[read] != '\r' ) {
			text[write++] = text[read];
			continue;
		}

		text[write++] = '\n';
		if ( read + 1 < text.size() && text[read + 1] == '\n' )
			++read;
	}
	text.resize( write );
}

ComputedGitLineDiff computeGitLineDiff( std::string_view baseline, std::string_view current ) {
	const std::size_t baselineLineCount = GitDiffDetail::lineCount( baseline );
	const std::size_t currentLineCount = GitDiffDetail::lineCount( current );
	std::vector<std::string_view> lineStorage;
	lineStorage.reserve( baselineLineCount + currentLineCount );
	GitDiffDetail::appendLines( baseline, lineStorage );
	GitDiffDetail::appendLines( current, lineStorage );

	static constexpr std::string_view emptyLine;
	const std::string_view* lineData = lineStorage.empty() ? &emptyLine : lineStorage.data();
	const GitDiffDetail::LineSequence baselineLines( lineData, baselineLineCount );
	const GitDiffDetail::LineSequence currentLines(
		baselineLineCount != 0 ? lineData + baselineLineCount : lineData, currentLineCount );

	dtl::Diff<std::string_view, GitDiffDetail::LineSequence> diff( baselineLines, currentLines );
	diff.compose();
	GitDiffDetail::LineDiffBuilder builder( currentLineCount );
	diff.storeSES<GitDiffDetail::LineDiffBuilder, GitDiffDetail::StoreLineDiff>( builder );
	return builder.finish();
}

} // namespace ecode
