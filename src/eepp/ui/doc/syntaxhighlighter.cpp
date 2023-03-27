#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>

namespace EE { namespace UI { namespace Doc {

SyntaxHighlighter::SyntaxHighlighter( TextDocument* doc ) :
	mDoc( doc ), mFirstInvalidLine( 0 ), mMaxWantedLine( 0 ) {
	reset();
}

void SyntaxHighlighter::changeDoc( TextDocument* doc ) {
	mDoc = doc;
	reset();
	mMaxWantedLine = (Int64)mDoc->linesCount() - 1;
}

void SyntaxHighlighter::reset() {
	Lock l( mLinesMutex );
	mLines.clear();
	mFirstInvalidLine = 0;
	mMaxWantedLine = 0;
}

void SyntaxHighlighter::invalidate( Int64 lineIndex ) {
	mFirstInvalidLine = eemin( lineIndex, mFirstInvalidLine );
	mMaxWantedLine = eemin<Int64>( mMaxWantedLine, (Int64)mDoc->linesCount() - 1 );
}

TokenizedLine SyntaxHighlighter::tokenizeLine( const size_t& line, const Uint64& state ) {
	TokenizedLine tokenizedLine;
	tokenizedLine.initState = state;
	tokenizedLine.hash = mDoc->line( line ).getHash();
	auto res = SyntaxTokenizer::tokenizePosition( mDoc->getSyntaxDefinition(),
												  mDoc->line( line ).toUtf8(), state );
	tokenizedLine.tokens = std::move( res.first );
	tokenizedLine.state = std::move( res.second );
	return tokenizedLine;
}

Mutex& SyntaxHighlighter::getLinesMutex() {
	return mLinesMutex;
}

const std::vector<SyntaxTokenPosition>& SyntaxHighlighter::getLine( const size_t& index ) {
	if ( mDoc->getSyntaxDefinition().getPatterns().empty() ) {
		static std::vector<SyntaxTokenPosition> noHighlightVector = { { "normal", 0 } };
		noHighlightVector[0].len = mDoc->line( index ).size();
		return noHighlightVector;
	}
	Lock l( mLinesMutex );
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
		mTokenizerLines[index] = mLines[index];
		mMaxWantedLine = eemax<Int64>( mMaxWantedLine, index );
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
	if ( visibleLinesCount <= 0 )
		return 0;
	if ( mFirstInvalidLine > mMaxWantedLine ) {
		mMaxWantedLine = 0;
	} else {
		bool changed = false;
		Int64 max = eemax( 0LL, eemin( mFirstInvalidLine + visibleLinesCount, mMaxWantedLine ) );

		for ( Int64 index = mFirstInvalidLine; index <= max; index++ ) {
			Uint64 state = SYNTAX_TOKENIZER_STATE_NONE;
			if ( index > 0 ) {
				auto prevIt = mLines.find( index - 1 );
				if ( prevIt != mLines.end() ) {
					state = prevIt->second.state;
				}
			}
			const auto& it = mLines.find( index );
			if ( it == mLines.end() || it->second.initState != state ) {
				mLines[index] = tokenizeLine( index, state );
				mTokenizerLines[index] = mLines[index];
				changed = true;
			}
		}

		mFirstInvalidLine = max + 1;
		return changed;
	}
	return false;
}

const SyntaxDefinition&
SyntaxHighlighter::getSyntaxDefinitionFromTextPosition( const TextPosition& position ) {
	Lock l( mLinesMutex );
	auto found = mLines.find( position.line() );
	if ( found == mLines.end() )
		return SyntaxDefinitionManager::instance()->getPlainStyle();

	TokenizedLine& line = found->second;
	SyntaxState state =
		SyntaxTokenizer::retrieveSyntaxState( mDoc->getSyntaxDefinition(), line.state );

	if ( nullptr == state.currentSyntax )
		return SyntaxDefinitionManager::instance()->getPlainStyle();

	return *state.currentSyntax;
}

std::string SyntaxHighlighter::getTokenTypeAt( const TextPosition& pos ) {
	if ( !pos.isValid() || pos.line() < 0 || pos.line() >= (Int64)mDoc->linesCount() )
		return "normal";
	auto tokens = getLine( pos.line() );
	if ( tokens.empty() )
		return "normal";
	Int64 col = 0;
	for ( const auto& token : tokens ) {
		col += token.len;
		if ( col > pos.column() )
			return token.type;
	}
	return "normal";
}

SyntaxTokenPosition SyntaxHighlighter::getTokenPositionAt( const TextPosition& pos ) {
	if ( !pos.isValid() || pos.line() < 0 || pos.line() >= (Int64)mDoc->linesCount() )
		return {};
	auto tokens = getLine( pos.line() );
	if ( tokens.empty() )
		return {};
	Int64 col = 0;
	for ( const auto& token : tokens ) {
		col += token.len;
		if ( col > pos.column() )
			return { token.type, static_cast<Int64>( col - token.len ), token.len };
	}
	return {};
}

void SyntaxHighlighter::setLine( const size_t& line, const TokenizedLine& tokenization ) {
	Lock l( mLinesMutex );
	mLines[line] = tokenization;
}

void SyntaxHighlighter::mergeLine( const size_t& line, const TokenizedLine& tokenization ) {
	TokenizedLine tline;
	{
		Lock l( mLinesMutex );
		auto found = mTokenizerLines.find( line );
		if ( found != mTokenizerLines.end() &&
			 mDoc->line( line ).getHash() == found->second.hash ) {
			tline = found->second;
		} else {
			tline = tokenizeLine( line );
			mTokenizerLines[line] = tline;
		}
	}

	for ( const auto& token : tokenization.tokens ) {
		for ( size_t i = 0; i < tline.tokens.size(); ++i ) {
			const auto ltoken = tline.tokens[i];
			if ( token.pos >= ltoken.pos && token.pos + token.len <= ltoken.pos + ltoken.len ) {
				tline.tokens.erase( tline.tokens.begin() + i );
				int iDiff = i;

				if ( token.pos > ltoken.pos ) {
					++iDiff;
					tline.tokens.insert( tline.tokens.begin() + i,
										 { ltoken.type, ltoken.pos,
										   static_cast<size_t>( token.pos - ltoken.pos ) } );
				}

				tline.tokens.insert( tline.tokens.begin() + iDiff, token );

				if ( token.pos + token.len < ltoken.pos + ltoken.len ) {
					tline.tokens.insert( tline.tokens.begin() + iDiff + 1,
										 { ltoken.type, static_cast<Int64>( token.pos + token.len ),
										   static_cast<size_t>( ( ltoken.pos + ltoken.len ) -
																( token.pos + token.len ) ) } );
				}

				break;
			}
		}
	}

	Lock l( mLinesMutex );
	mLines[line] = std::move( tline );
}

}}} // namespace EE::UI::Doc
