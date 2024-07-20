#ifndef EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
#define EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP

#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <unordered_map>

namespace EE { namespace UI { namespace Doc {

struct EE_API TokenizedLine {
	SyntaxState initState;
	String::HashType hash{ 0 };
	std::vector<SyntaxTokenPosition> tokens;
	SyntaxState state;
	Uint64 signature{ 0 };

	void updateSignature();

	static Uint64 calcSignature( const std::vector<SyntaxTokenPosition>& tokens );
};

class EE_API SyntaxHighlighter {
  public:
	explicit SyntaxHighlighter( TextDocument* doc );

	~SyntaxHighlighter();

	void changeDoc( TextDocument* doc );

	void reset();

	void invalidate( Int64 lineIndex );

	const std::vector<SyntaxTokenPosition>& getLine( const size_t& index,
													 bool mustTokenize = true );

	Int64 getFirstInvalidLine() const;

	Int64 getMaxWantedLine() const;

	bool updateDirty( int visibleLinesCount = 40 );

	const SyntaxDefinition& getSyntaxDefinitionFromTextPosition( const TextPosition& position );

	SyntaxStyleType getTokenTypeAt( const TextPosition& pos );

	SyntaxTokenPosition getTokenPositionAt( const TextPosition& pos );

	void setLine( const size_t& line, const TokenizedLine& tokenization );

	void mergeLine( const size_t& line, const TokenizedLine& tokenization );

	TokenizedLine tokenizeLine( const size_t& line, const SyntaxState& state = SyntaxState{} );

	Mutex& getLinesMutex();

	void moveHighlight( const Int64& fromLine, const Int64& toLine, const Int64& numLines );

	Uint64 getTokenizedLineSignature( const size_t& index );

	const Int64& getMaxTokenizationLength() const;

	void setMaxTokenizationLength( const Int64& maxTokenizationLength );

	void tokenizeAsync( std::shared_ptr<ThreadPool> pool,
						const std::function<void()>& onDone = {} );

	bool isTokenizingAsync() const { return mTokenizeAsync; }

	void setStopTokenizingAsync() { mStopTokenizing = true; }

  protected:
	TextDocument* mDoc;
	std::unordered_map<size_t, TokenizedLine> mLines;
	UnorderedMap<size_t, TokenizedLine> mTokenizerLines;
	Mutex mLinesMutex;
	Int64 mFirstInvalidLine;
	Int64 mMaxWantedLine;
	Int64 mMaxTokenizationLength{ 0 };
	std::mutex mAsyncTokenizeMutex;
	std::condition_variable mAsyncTokenizeConf;
	bool mTokenizeAsync{ false };
	bool mStopTokenizing{ false };
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
