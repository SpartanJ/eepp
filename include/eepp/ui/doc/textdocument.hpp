#ifndef EE_UI_DOC_TEXTDOCUMENT
#define EE_UI_DOC_TEXTDOCUMENT

#include <eepp/core/string.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <eepp/ui/doc/undostack.hpp>
#include <unordered_set>
#include <vector>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API TextDocument {
  public:
	class Client {
	  public:
		virtual ~Client();
		virtual void onDocumentTextChanged() = 0;
		virtual void onDocumentCursorChange( const TextPosition& ) = 0;
		virtual void onDocumentSelectionChange( const TextRange& ) = 0;
		virtual void onDocumentLineCountChange( const size_t& lastCount,
												const size_t& newCount ) = 0;
	};

	enum IndentType { IndentSpaces, IndentTabs };

	static bool isNonWord( String::StringBaseType ch );

	TextDocument();

	void reset();

	void loadFromPath( const std::string& path );

	bool save();

	bool save( const std::string& path, const bool& utf8bom = false );

	bool save( IOStreamFile& stream, const bool& utf8bom = false );

	const std::string getFilename() const;

	void setSelection( TextPosition position );

	void setSelection( TextPosition start, TextPosition end, bool swap = false );

	void setSelection( TextRange range );

	TextRange getSelection( bool sort ) const;

	const TextRange& getSelection() const;

	String& line( const size_t& index );

	const String& line( const size_t& index ) const;

	size_t linesCount() const;

	std::vector<String>& lines();

	bool hasSelection() const;

	String getText( const TextRange& range ) const;

	String getSelectedText() const;

	String::StringBaseType getChar( const TextPosition& position ) const;

	TextPosition insert( const TextPosition& position, const String& text );

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

	void deleteTo( TextPosition position );

	void deleteTo( int offset );

	void deleteSelection();

	void selectTo( TextPosition position );

	void selectTo( int offset );

	void moveTo( TextPosition offset );

	void moveTo( int columnOffset );

	void textInput( const String& text );

	void registerClient( Client& client );

	void unregisterClient( Client& client );

	void moveToPreviousChar();

	void moveToNextChar();

	void moveToPreviousWord();

	void moveToNextWord();

	void moveToPreviousLine( Int64 lastColIndex = 0 );

	void moveToNextLine( Int64 lastColIndex = 0 );

	void moveToPreviousPage( Int64 pageSize );

	void moveToNextPage( Int64 pageSize );

	void deleteToPreviousChar();

	void deleteToNextChar();

	void deleteToPreviousWord();

	void deleteToNextWord();

	void selectToPreviousChar();

	void selectToNextChar();

	void selectToPreviousWord();

	void selectWord();

	void selectToNextWord();

	void selectToPreviousLine( Int64 lastColIndex );

	void selectToNextLine( Int64 lastColIndex );

	void selectToStartOfLine();

	void selectToEndOfLine();

	void selectToPreviousPage( Int64 pageSize );

	void selectToNextPage( Int64 pageSize );

	void selectAll();

	void newLine();

	void indent();

	void unindent();

	String getIndentString();

	const Uint32& getTabWidth() const;

	void setTabWidth( const Uint32& tabWidth );

	TextPosition sanitizePosition( const TextPosition& position ) const;

	const IndentType& getIndentType() const;

	void setIndentType( const IndentType& indentType );

	void undo();

	void redo();

	const SyntaxDefinition& getSyntaxDefinition() const;

	Uint64 getCurrentChangeId() const;

	const std::string& getDefaultFileName() const;

	void setDefaultFileName( const std::string& defaultFileName );

	const std::string& getFilePath() const;

	bool isDirty() const;

  protected:
	friend class UndoStack;
	UndoStack mUndoStack;
	std::string mFilePath;
	std::vector<String> mLines;
	TextRange mSelection;
	std::unordered_set<Client*> mClients;
	bool mIsCLRF{false};
	bool mIsBOM{false};
	Uint32 mTabWidth{4};
	IndentType mIndentType{IndentTabs};
	Clock mTimer;
	SyntaxDefinition mSyntaxDefinition;
	std::string mDefaultFileName;
	Uint64 mCleanChangeId;

	void cleanChangeId();

	void notifyTextChanged();

	void notifyCursorChanged();

	void notifySelectionChanged();

	void notifyLineCountChanged( const size_t& lastCount, const size_t& newCount );

	void insertAtStartOfSelectedLines( String text, bool skipEmpty );

	void removeFromStartOfSelectedLines( String text, bool skipEmpty );

	void remove( TextRange range, UndoStackContainer& undoStack, const Time& time );

	TextPosition insert( const TextPosition& position, const String& text,
						 UndoStackContainer& undoStack, const Time& time );

	TextPosition insert( TextPosition position, const String::StringBaseType& text );
};

}}} // namespace EE::UI::Doc

#endif
