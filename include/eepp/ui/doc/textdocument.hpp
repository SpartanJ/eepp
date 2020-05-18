#ifndef EE_UI_DOC_TEXTDOCUMENT
#define EE_UI_DOC_TEXTDOCUMENT

#include <eepp/core/string.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <vector>

namespace EE { namespace UI { namespace Doc {

class EE_API TextDocument {
  public:
	static bool isNonWord( String::StringBaseType ch );

	TextDocument();

	void reset();

	void load( const std::string& path );

	void save( const std::string& path );

	const std::string getFilename() const;

	void setSelection( TextPosition position );

	void setSelection( TextPosition pos1, TextPosition pos2, bool swap = false );

	void setSelection( TextRange range );

	TextRange getSelection( bool sort ) const;

	const TextRange& getSelection() const;

	String& line( const size_t& index );

	std::vector<String>& lines();

	bool hasSelection() const;

	String getText( const TextRange& range ) const;

	String::StringBaseType getChar( const TextPosition& position ) const;

	TextPosition insert( const TextPosition& position, const String& text );

	TextPosition insert( TextPosition position, const String::StringBaseType& text );

	void remove( TextPosition position );

	void remove( TextRange range );

	TextPosition positionOffset( TextPosition position, int columnOffset ) const;

	TextPosition positionOffset( TextPosition position, TextPosition offset ) const;

	void print() const;

	// Translations
	TextPosition nextChar( TextPosition position ) const;

	TextPosition previousChar( TextPosition position ) const;

	TextPosition previousWordBoundary( TextPosition position ) const;

	TextPosition nextWordBoundary( TextPosition position ) const;

	TextPosition startOfWord( TextPosition position ) const;

	TextPosition endOfWord( TextPosition position ) const;

	TextPosition startOfLine( TextPosition position ) const;

	TextPosition endOfLine( TextPosition position ) const;

	TextPosition startOfDoc() const;

	TextPosition endOfDoc() const;

	void deleteTo( TextPosition offset );

	void deleteTo( int offset );

	void selectTo( TextPosition offset );

	void moveTo( TextPosition offset );

	void textInput( const String& text );

  protected:
	std::string mFilename;
	std::vector<String> mLines;
	TextRange mSelection;
	bool mIsCLRF;

	TextPosition sanitizePosition( const TextPosition& position ) const;
};

}}} // namespace EE::UI::Doc

#endif
