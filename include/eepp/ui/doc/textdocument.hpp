#ifndef EE_UI_DOC_TEXTDOCUMENT
#define EE_UI_DOC_TEXTDOCUMENT

#include <eepp/core/string.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <eepp/ui/doc/textdocumentline.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <eepp/ui/doc/undostack.hpp>
#include <functional>
#include <map>
#include <unordered_set>
#include <vector>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

class EE_API TextDocument {
  public:
	typedef std::function<void()> DocumentCommand;

	class EE_API Client {
	  public:
		virtual ~Client();
		virtual void onDocumentTextChanged() = 0;
		virtual void onDocumentCursorChange( const TextPosition& ) = 0;
		virtual void onDocumentSelectionChange( const TextRange& ) = 0;
		virtual void onDocumentLineCountChange( const size_t& lastCount,
												const size_t& newCount ) = 0;
		virtual void onDocumentLineChanged( const Int64& lineIndex ) = 0;
	};

	enum IndentType { IndentSpaces, IndentTabs };

	static bool isNonWord( String::StringBaseType ch );

	TextDocument();

	bool isEmpty();

	void reset();

	bool loadFromStream( IOStream& path );

	bool loadFromFile( const std::string& path );

	bool loadFromMemory( const Uint8* data, const Uint32& size );

	bool loadFromPack( Pack* pack, std::string filePackPath );

	bool save();

	bool save( const std::string& path, const bool& utf8bom = false );

	bool save( IOStream& stream, const bool& utf8bom = false );

	std::string getFilename() const;

	void setSelection( TextPosition position );

	void setSelection( TextPosition start, TextPosition end, bool swap = false );

	void setSelection( TextRange range );

	TextRange getSelection( bool sort ) const;

	const TextRange& getSelection() const;

	TextDocumentLine& line( const size_t& index );

	const TextDocumentLine& line( const size_t& index ) const;

	size_t linesCount() const;

	std::vector<TextDocumentLine>& lines();

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

	TextPosition startOfContent( TextPosition position );

	TextPosition startOfDoc() const;

	TextPosition endOfDoc() const;

	void deleteTo( TextPosition position );

	void deleteTo( int offset );

	void deleteSelection();

	void selectTo( TextPosition position );

	void selectTo( int offset );

	void moveTo( TextPosition offset );

	void moveTo( int columnOffset );

	void textInput( const String& text );

	void registerClient( Client* client );

	void unregisterClient( Client* client );

	void moveToPreviousChar();

	void moveToNextChar();

	void moveToPreviousWord();

	void moveToNextWord();

	void moveToPreviousLine();

	void moveToNextLine();

	void moveToPreviousPage( Int64 pageSize );

	void moveToNextPage( Int64 pageSize );

	void moveToStartOfDoc();

	void moveToEndOfDoc();

	void moveToStartOfLine();

	void moveToEndOfLine();

	void moveToStartOfContent();

	void deleteToPreviousChar();

	void deleteToNextChar();

	void deleteToPreviousWord();

	void deleteToNextWord();

	void selectToPreviousChar();

	void selectToNextChar();

	void selectToPreviousWord();

	void selectWord();

	void selectLine();

	void selectToNextWord();

	void selectToPreviousLine();

	void selectToNextLine();

	void selectToStartOfLine();

	void selectToEndOfLine();

	void selectToStartOfDoc();

	void selectToEndOfDoc();

	void selectToPreviousPage( Int64 pageSize );

	void selectToNextPage( Int64 pageSize );

	void selectToStartOfContent();

	void selectAll();

	void newLine();

	void newLineAbove();

	void indent();

	void unindent();

	void moveLinesUp();

	void moveLinesDown();

	void undo();

	void redo();

	void execute( const std::string& command );

	void setCommand( const std::string& command, DocumentCommand func );

	TextPosition find( String text, TextPosition from = {0, 0}, const bool& caseSensitive = true );

	TextPosition findLast( String text, TextPosition from = {0, 0},
						   const bool& caseSensitive = true );

	TextPosition replaceSelection( const String& replace );

	TextPosition replace( String search, const String& replace, TextPosition from = {0, 0},
						  const bool& caseSensitive = true );

	String getIndentString();

	const Uint32& getTabWidth() const;

	void setTabWidth( const Uint32& tabWidth );

	TextPosition sanitizePosition( const TextPosition& position ) const;

	const IndentType& getIndentType() const;

	void setIndentType( const IndentType& indentType );

	const SyntaxDefinition& getSyntaxDefinition() const;

	Uint64 getCurrentChangeId() const;

	const std::string& getDefaultFileName() const;

	void setDefaultFileName( const std::string& defaultFileName );

	const std::string& getFilePath() const;

	bool isDirty() const;

	const Uint32& getPageSize() const;

	void setPageSize( const Uint32& pageSize );

	TextPosition findOpenBracket( TextPosition startPosition,
								  const String::StringBaseType& openBracket,
								  const String::StringBaseType& closeBracket ) const;

	TextPosition findCloseBracket( TextPosition startPosition,
								   const String::StringBaseType& openBracket,
								   const String::StringBaseType& closeBracket ) const;

  protected:
	friend class UndoStack;
	UndoStack mUndoStack;
	std::string mFilePath;
	std::vector<TextDocumentLine> mLines;
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
	Uint32 mPageSize{10};
	std::map<std::string, DocumentCommand> mCommands;

	void initializeCommands();

	void cleanChangeId();

	void notifyTextChanged();

	void notifyCursorChanged();

	void notifySelectionChanged();

	void notifyLineCountChanged( const size_t& lastCount, const size_t& newCount );

	void notifyLineChanged( const Int64& lineIndex );

	void insertAtStartOfSelectedLines( const String& text, bool skipEmpty );

	void removeFromStartOfSelectedLines( const String& text, bool skipEmpty );

	void remove( TextRange range, UndoStackContainer& undoStack, const Time& time );

	TextPosition insert( const TextPosition& position, const String& text,
						 UndoStackContainer& undoStack, const Time& time );

	TextPosition insert( TextPosition position, const String::StringBaseType& text );

	void appendLineIfLastLine( Int64 line );
};

}}} // namespace EE::UI::Doc

#endif
