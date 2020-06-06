#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>

namespace EE { namespace UI { namespace Doc {

SyntaxHighlighter::SyntaxHighlighter( TextDocument* doc ) :
	mDoc( doc ), mFirstInvalidLine( 0 ), mMaxWantedLine( 0 ) {
	reset();
}

void SyntaxHighlighter::reset() {
	mLines.clear();
	mFirstInvalidLine = 0;
	mMaxWantedLine = 0;
}

void SyntaxHighlighter::invalidate( Int64 lineIndex ) {
	mFirstInvalidLine = eemin( lineIndex, mFirstInvalidLine );
	mMaxWantedLine = eemin<Int64>( mMaxWantedLine, (Int64)mDoc->linesCount() - 1 );
}

TokenizedLine SyntaxHighlighter::tokenizeLine( const size_t& line, const int& state ) {
	TokenizedLine tokenizedLine;
	tokenizedLine.initState = state;
	tokenizedLine.hash = mDoc->line( line ).getHash();
	std::pair<std::vector<SyntaxToken>, int> res = SyntaxTokenizer::tokenize(
		mDoc->getSyntaxDefinition(), mDoc->line( line ).toUtf8(), state );
	tokenizedLine.tokens = std::move( res.first );
	tokenizedLine.state = std::move( res.second );
	return tokenizedLine;
}

const std::vector<SyntaxToken>& SyntaxHighlighter::getLine( const size_t& index ) {
	const auto& it = mLines.find( index );
	if ( it == mLines.end() ||
		 ( index < mDoc->linesCount() && mDoc->line( index ).getHash() != it->second.hash ) ) {
		int prevState = SYNTAX_TOKENIZER_STATE_NONE;
		if ( index > 0 ) {
			auto prevIt = mLines.find( index - 1 );
			if ( prevIt != mLines.end() ) {
				prevState = prevIt->second.state;
			}
		}
		mLines[index] = tokenizeLine( index, prevState );
		return mLines[index].tokens;
	}
	mMaxWantedLine = eemax<Int64>( mMaxWantedLine, index );
	return it->second.tokens;
}

Int64 SyntaxHighlighter::getFirstInvalidLine() const {
	return mFirstInvalidLine;
}

Int64 SyntaxHighlighter::getMaxWantedLine() const {
	return mMaxWantedLine;
}

bool SyntaxHighlighter::updateDirty( int visibleLinesCount ) {
	if ( mFirstInvalidLine > mMaxWantedLine ) {
		mMaxWantedLine = 0;
	} else {
		bool changed = false;
		Int64 max = eemin( mFirstInvalidLine + visibleLinesCount, mMaxWantedLine );

		for ( Int64 index = mFirstInvalidLine; index <= max; index++ ) {
			int state = SYNTAX_TOKENIZER_STATE_NONE;
			if ( index > 0 ) {
				auto prevIt = mLines.find( index - 1 );
				if ( prevIt != mLines.end() ) {
					state = prevIt->second.state;
				}
			}
			const auto& it = mLines.find( index );
			if ( it != mLines.end() && it->second.initState != state ) {
				mLines[index] = tokenizeLine( index, state );
				changed = true;
			}
		}

		mFirstInvalidLine = max + 1;
		return changed;
	}
	return false;
}

}}} // namespace EE::UI::Doc
