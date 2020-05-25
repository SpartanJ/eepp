#ifndef EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
#define EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP

#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <map>

namespace EE { namespace UI { namespace Doc {

struct TokenizedLine {
	int initState;
	String text;
	std::vector<SyntaxToken> tokens;
	int state;
};

class EE_API SyntaxHighlighter {
  public:
	SyntaxHighlighter( TextDocument* doc );

	void reset();

	const std::vector<SyntaxToken>& getLine( const size_t& index );

  protected:
	TextDocument* mDoc;
	std::map<size_t, TokenizedLine> mLines;
	TokenizedLine tokenizeLine( const size_t& line, const int& state );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXHIGHLIGHTER_HPP
