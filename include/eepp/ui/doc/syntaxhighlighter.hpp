#ifndef EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
#define EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP

#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <map>

namespace EE { namespace UI { namespace Doc {

struct TokenizedLine {
	Uint64 initState;
	String::HashType hash;
	std::vector<SyntaxToken> tokens;
	Uint64 state;
};

class EE_API SyntaxHighlighter {
  public:
	SyntaxHighlighter( TextDocument* doc );

	void changeDoc( TextDocument* doc );

	void reset();

	void invalidate( Int64 lineIndex );

	const std::vector<SyntaxToken>& getLine( const size_t& index );

	Int64 getFirstInvalidLine() const;

	Int64 getMaxWantedLine() const;

	bool updateDirty( int visibleLinesCount = 40 );

  protected:
	TextDocument* mDoc;
	std::map<size_t, TokenizedLine> mLines;
	Int64 mFirstInvalidLine;
	Int64 mMaxWantedLine;
	TokenizedLine tokenizeLine( const size_t& line, const Uint64& state );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
