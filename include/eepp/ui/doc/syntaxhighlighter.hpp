#ifndef EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
#define EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP

#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <unordered_map>

namespace EE { namespace UI { namespace Doc {

struct TokenizedLine {
	Uint64 initState{ SYNTAX_TOKENIZER_STATE_NONE };
	String::HashType hash;
	std::vector<SyntaxTokenPosition> tokens;
	Uint64 state{ SYNTAX_TOKENIZER_STATE_NONE };
};

class EE_API SyntaxHighlighter {
  public:
	explicit SyntaxHighlighter( TextDocument* doc );

	void changeDoc( TextDocument* doc );

	void reset();

	void invalidate( Int64 lineIndex );

	const std::vector<SyntaxTokenPosition>& getLine( const size_t& index );

	Int64 getFirstInvalidLine() const;

	Int64 getMaxWantedLine() const;

	bool updateDirty( int visibleLinesCount = 40 );

	const SyntaxDefinition& getSyntaxDefinitionFromTextPosition( const TextPosition& position );

	std::string getTokenTypeAt( const TextPosition& pos );

	SyntaxTokenPosition getTokenPositionAt( const TextPosition& pos );

	void setLine( const size_t& line, const TokenizedLine& tokenization );

	void mergeLine( const size_t& line, const TokenizedLine& tokenization );

	TokenizedLine tokenizeLine( const size_t& line,
								const Uint64& state = SYNTAX_TOKENIZER_STATE_NONE );

	Mutex& getLinesMutex();

  protected:
	TextDocument* mDoc;
	std::unordered_map<size_t, TokenizedLine> mLines;
	std::unordered_map<size_t, TokenizedLine> mTokenizerLines;
	Mutex mLinesMutex;
	Int64 mFirstInvalidLine;
	Int64 mMaxWantedLine;
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
