#ifndef EE_UI_DOC_TEXTDOCUMENT
#define EE_UI_DOC_TEXTDOCUMENT

#include <eepp/core/string.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <unordered_set>
#include <vector>

namespace EE { namespace UI { namespace Doc {

class EE_API TextDocument {
  public:
	class Client {
	  public:
		virtual ~Client();
		virtual void onDocumentTextChanged() = 0;
		virtual void onDocumentCursorChange( const TextPosition& ) = 0;
		virtual void onDocumentSelectionChange( const TextRange& ) = 0;
	};

	static bool isNonWord( String::StringBaseType ch );

	TextDocument();

	void reset();

	void loadFromPath( const std::string& path );

	void save( const std::string& path );

	const std::string getFilename() const;

	void setSelection( TextPosition position );

	void setSelection( TextPosition start, TextPosition end, bool swap = false );

	void setSelection( TextRange range );

	TextRange getSelection( bool sort ) const;

	const TextRange& getSelection() const;

	String& line( const size_t& index );

	const String& line( const size_t& index ) const;

	size_t lineCount() const;

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

	TextPosition getAbsolutePosition( TextPosition position ) const;

	Int64 getRelativeColumnOffset( TextPosition position ) const;

	void deleteTo( TextPosition offset );

	void deleteTo( int offset );

	void selectTo( TextPosition offset );

	void moveTo( TextPosition offset );

	void textInput( const String& text );

	void registerClient( Client& client );

	void unregisterClient( Client& client );

	void moveToPreviousChar();

	void moveToNextChar();

	void moveToPreviousLine( Int64 lastColIndex = 0 );

	void moveToNextLine( Int64 lastColIndex = 0 );

	void moveToPreviousPage( Int64 pageSize );

	void moveToNextPage( Int64 pageSize );

	const Uint32& getTabWidth() const;

	void setTabWidth( const Uint32& tabWidth );

	TextPosition sanitizePosition( const TextPosition& position ) const;

  protected:
	std::string mFilename;
	std::vector<String> mLines;
	TextRange mSelection;
	std::unordered_set<Client*> mClients;
	bool mIsCLRF;
	Uint32 mTabWidth{4};

	void notifyTextChanged();

	void notifyCursorChanged();

	void notifySelectionChanged();
};

}}} // namespace EE::UI::Doc

#endif
