#ifndef EE_UI_DOC_TEXTDOCUMENT
#define EE_UI_DOC_TEXTDOCUMENT

#include <array>
#include <atomic>
#include <eepp/core/string.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <eepp/ui/doc/textdocumentline.hpp>
#include <eepp/ui/doc/textposition.hpp>
#include <eepp/ui/doc/textrange.hpp>
#include <eepp/ui/doc/undostack.hpp>
#include <functional>
#include <vector>

using namespace EE::System;
using namespace EE::Network;

namespace EE { namespace UI { namespace Doc {

class SyntaxHighlighter;

struct DocumentContentChange {
	TextRange range;
	String text;
};

class EE_API TextDocument {
  public:
	enum class UndoRedo { Undo, Redo };

	enum class IndentType { IndentSpaces, IndentTabs };

	enum class LineEnding { LF, CRLF, CR };

	enum class FindReplaceType { Normal, LuaPattern };

	enum class LoadStatus { Loaded, Interrupted, Failed };

	enum class MatchDirection { Forward, Backward };

	class EE_API Client {
	  public:
		virtual ~Client();
		virtual void onDocumentLoaded( TextDocument* ){};
		virtual void onDocumentTextChanged( const DocumentContentChange& ) = 0;
		virtual void onDocumentUndoRedo( const UndoRedo& eventType ) = 0;
		virtual void onDocumentCursorChange( const TextPosition& ) = 0;
		virtual void onDocumentInterestingCursorChange( const TextPosition& ){};
		virtual void onDocumentSelectionChange( const TextRange& ) = 0;
		virtual void onDocumentLineCountChange( const size_t& lastCount,
												const size_t& newCount ) = 0;
		virtual void onDocumentLineChanged( const Int64& lineIndex ) = 0;
		virtual void onDocumentSaved( TextDocument* ) = 0;
		virtual void onDocumentClosed( TextDocument* ) = 0;
		virtual void onDocumentDirtyOnFileSystem( TextDocument* ) = 0;
		virtual void onDocumentMoved( TextDocument* ) = 0;
		virtual void onDocumentReloaded( TextDocument* doc ) {
			onDocumentClosed( doc );
			onDocumentLoaded( doc );
		}
		virtual void onDocumentSyntaxDefinitionChange( const SyntaxDefinition& ) {}
		virtual void onDocumentLineMove( const Int64& /*fromLine*/, const Int64& /*numLines*/ ){};
		virtual TextRange getVisibleRange() const { return {}; };
	};

	typedef std::function<void()> DocumentCommand;
	typedef std::function<void( Client* )> DocumentRefCommand;

	static std::string lineEndingToString( const LineEnding& le ) {
		switch ( le ) {
			case LineEnding::CRLF:
				return "CRLF";
			case LineEnding::CR:
				return "CR";
			case LineEnding::LF:
			default:
				return "LF";
		}
	}

	static LineEnding stringToLineEnding( const std::string& str ) {
		if ( "CR" == str )
			return LineEnding::CR;
		if ( "CRLF" == str )
			return LineEnding::CRLF;
		return LineEnding::LF;
	}

	TextDocument( bool verbose = true );

	~TextDocument();

	bool isNonWord( String::StringBaseType ch ) const;

	bool hasFilepath() const;

	bool isEmpty() const;

	bool isUntitledEmpty() const;

	void reset();

	void resetCursor();

	LoadStatus loadFromStream( IOStream& path );

	LoadStatus loadFromFile( const std::string& path );

	bool loadAsyncFromFile( const std::string& path, std::shared_ptr<ThreadPool> pool,
							std::function<void( TextDocument*, bool )> onLoaded =
								std::function<void( TextDocument*, bool success )>() );

	LoadStatus loadFromMemory( const Uint8* data, const Uint32& size );

	LoadStatus loadFromPack( Pack* pack, std::string filePackPath );

	/**
	 * @brief loadFromURL
	 * @param url Resources URL.
	 * @param headers Key value map of headers
	 * @return
	 */
	LoadStatus loadFromURL(
		const std::string& url,
		const EE::Network::Http::Request::FieldTable& headers = Http::Request::FieldTable() );

	bool loadAsyncFromURL( const std::string& url,
						   const Http::Request::FieldTable& headers = Http::Request::FieldTable(),
						   std::function<void( TextDocument*, bool )> onLoaded =
							   std::function<void( TextDocument*, bool success )>(),
						   const Http::Request::ProgressCallback& progressCallback = nullptr );

	LoadStatus reload();

	bool save();

	bool save( const std::string& path );

	bool save( IOStream& stream, bool keepUndoRedoStatus = false );

	std::string getFilename() const;

	void setSelection( const TextPosition& position );

	void setSelection( const size_t& cursorIdx, TextPosition start, TextPosition end,
					   bool swap = false );

	void setSelection( TextPosition start, TextPosition end, bool swap = false );

	void setSelection( const TextRange& range );

	TextRange getSelection( bool sort ) const;

	const TextRanges& getSelections() const;

	const TextRange& getSelection() const;

	const TextRange& getSelectionIndex( const size_t& index ) const;

	TextDocumentLine& line( const size_t& index );

	const TextDocumentLine& line( const size_t& index ) const;

	size_t linesCount() const;

	const TextDocumentLine& getCurrentLine() const;

	std::vector<TextDocumentLine>& lines();

	bool hasSelection() const;

	const std::array<Uint8, 16>& getHash() const;

	std::string getHashHexString() const;

	String getText( const TextRange& range ) const;

	String getText() const;

	String getSelectedText() const;

	String::StringBaseType getPrevChar() const;

	String::StringBaseType getCurrentChar() const;

	String::StringBaseType getChar( const TextPosition& position ) const;

	TextPosition insert( const size_t& cursorIdx, const TextPosition& position,
						 const String& text );

	size_t remove( const size_t& cursorIdx, TextRange range );

	TextPosition positionOffset( TextPosition position, int columnOffset ) const;

	TextPosition positionOffset( TextPosition position, TextPosition offset ) const;

	bool replaceLine( const Int64& lineNum, const String& text );

	bool replaceCurrentLine( const String& text );

	void print() const;

	// Translations
	TextPosition nextChar( TextPosition position ) const;

	TextPosition previousChar( TextPosition position ) const;

	TextPosition previousWordBoundary( TextPosition position,
									   bool ignoreFirstNonWord = true ) const;

	TextPosition nextWordBoundary( TextPosition position, bool ignoreFirstNonWord = true ) const;

	TextPosition previousSpaceBoundaryInLine( TextPosition position ) const;

	TextPosition nextSpaceBoundaryInLine( TextPosition position ) const;

	TextPosition startOfWord( TextPosition position ) const;

	TextPosition endOfWord( TextPosition position ) const;

	TextPosition startOfLine( TextPosition position ) const;

	TextPosition endOfLine( TextPosition position ) const;

	TextPosition startOfContent( TextPosition position );

	TextPosition startOfDoc() const;

	TextPosition endOfDoc() const;

	TextRange getDocRange() const;

	void deleteTo( const size_t& cursorIdx, TextPosition position );

	void deleteTo( const size_t& cursorIdx, int offset );

	void deleteSelection();

	void selectTo( TextPosition position );

	void selectTo( int offset );

	void moveTo( TextPosition offset );

	void moveTo( int columnOffset );

	void textInput( const String& text, bool mightBeInteresting = true );

	void imeTextEditing( const String& text );

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

	void deleteCurrentLine();

	void selectToPreviousChar();

	void selectToNextChar();

	void selectToPreviousWord();

	void selectWord( bool withMulticursor = true );

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

	bool hasUndo() const;

	bool hasRedo() const;

	void undo();

	void redo();

	void execute( const std::string& command );

	void execute( const std::string& command, Client* client );

	void setCommands( const UnorderedMap<std::string, DocumentCommand>& cmds );

	void setCommand( const std::string& command, const DocumentCommand& func );

	void setCommand( const std::string& command, const DocumentRefCommand& func );

	bool hasCommand( const std::string& command );

	bool removeCommand( const std::string& command );

	TextRange find( const String& text, TextPosition from = { 0, 0 }, bool caseSensitive = true,
					bool wholeWord = false, FindReplaceType type = FindReplaceType::Normal,
					TextRange restrictRange = TextRange() );

	TextRange findLast( const String& text, TextPosition from = { 0, 0 }, bool caseSensitive = true,
						bool wholeWord = false, FindReplaceType type = FindReplaceType::Normal,
						TextRange restrictRange = TextRange() );

	TextRanges findAll( const String& text, bool caseSensitive = true, bool wholeWord = false,
						FindReplaceType type = FindReplaceType::Normal,
						TextRange restrictRange = TextRange(), size_t maxResults = 0 );

	int replaceAll( const String& text, const String& replace, const bool& caseSensitive = true,
					const bool& wholeWord = false, FindReplaceType type = FindReplaceType::Normal,
					TextRange restrictRange = TextRange() );

	TextPosition replaceSelection( const String& replace );

	TextPosition replace( String search, const String& replace, TextPosition from = { 0, 0 },
						  const bool& caseSensitive = true, const bool& wholeWord = false,
						  FindReplaceType type = FindReplaceType::Normal,
						  TextRange restrictRange = TextRange() );

	String getIndentString();

	const Uint32& getIndentWidth() const;

	void setIndentWidth( const Uint32& tabWidth );

	TextPosition sanitizePosition( const TextPosition& position ) const;

	const IndentType& getIndentType() const;

	void setIndentType( const IndentType& indentType );

	const SyntaxDefinition& getSyntaxDefinition() const;

	void setSyntaxDefinition( const SyntaxDefinition& definition );

	Uint64 getCurrentChangeId() const;

	const std::string& getDefaultFileName() const;

	void setDefaultFileName( const std::string& defaultFileName );

	const std::string& getFilePath() const;

	const URI& getURI() const;

	const FileInfo& getFileInfo() const;

	bool isDirty() const;

	const Uint32& getPageSize() const;

	void setPageSize( const Uint32& pageSize );

	const String& getNonWordChars() const;

	void toggleLineComments();

	void setNonWordChars( const String& nonWordChars );

	void resetSyntax();

	bool getAutoDetectIndentType() const;

	void setAutoDetectIndentType( bool autodetect );

	const LineEnding& getLineEnding() const;

	void setLineEnding( const LineEnding& lineEnding );

	bool getForceNewLineAtEndOfFile() const;

	void setForceNewLineAtEndOfFile( bool forceNewLineAtEndOfFile );

	bool getTrimTrailingWhitespaces() const;

	void setTrimTrailingWhitespaces( bool trimTrailingWhitespaces );

	Client* getActiveClient() const;

	void setActiveClient( Client* activeClient );

	void setBOM( bool active );

	bool getBOM() const;

	TextRange sanitizeRange( const TextRange& range ) const;

	TextRanges sanitizeRange( const TextRanges& ranges ) const;

	bool isValidPosition( const TextPosition& position ) const;

	bool isValidRange( const TextRange& range ) const;

	bool getAutoCloseBrackets() const;

	void setAutoCloseBrackets( bool autoCloseBrackets );

	const std::vector<std::pair<String::StringBaseType, String::StringBaseType>>&
	getAutoCloseBracketsPairs() const;

	void setAutoCloseBracketsPairs(
		const std::vector<std::pair<String::StringBaseType, String::StringBaseType>>&
			autoCloseBracketsPairs );

	bool isDirtyOnFileSystem() const;

	void setDirtyOnFileSystem( bool dirtyOnFileSystem );

	bool isSaving() const;

	void sanitizeCurrentSelection();

	bool isLoading() const;

	bool isDeleteOnClose() const;

	void setDeleteOnClose( bool deleteOnClose );

	bool hasSyntaxDefinition() const;

	void notifyDocumentMoved( const std::string& newPath );

	void toUpperSelection();

	void toLowerSelection();

	const std::string& getLoadingFilePath() const;

	const URI& getLoadingFileURI() const;

	void setSelection( const TextRanges& selection );

	void resetSelection( const TextRanges& selection );

	std::vector<TextRange> getSelectionsSorted() const;

	void addCursorAbove();

	void addCursorBelow();

	TextRange getTopMostCursor();

	TextRange getBottomMostCursor();

	void moveTo( const size_t& cursorIdx, TextPosition offset );

	void moveTo( const size_t& cursorIdx, int columnOffset );

	void setSelection( const size_t& cursorIdx, const TextPosition& position );

	void mergeSelection();

	void selectTo( const size_t& cursorIdx, TextPosition position );

	void selectTo( const size_t& cursorIdx, int offset );

	void setSelection( const size_t& cursorIdx, const TextRange& range );

	TextRange addSelection( TextRange selection );

	TextRange addSelection( const TextPosition& selection );

	void popSelection();

	void deleteSelection( const size_t& cursorIdx );

	String getSelectedText( const size_t& cursorIdx ) const;

	size_t getLastSelection() const;

	bool selectionExists( const TextRange& selection );

	bool selectionExists( const TextPosition& selection );

	TextPosition replaceSelection( const size_t& cursorIdx, const String& replace );

	void cursorUndo();

	void selectAllMatches();

	String getAllSelectedText() const;

	std::vector<std::string> getCommandList() const;

	bool isRunningTransaction() const;

	void setRunningTransaction( const bool runningTransaction );

	TextPosition getMatchingBracket( TextPosition startPosition,
									 const String::StringBaseType& openBracket,
									 const String::StringBaseType& closeBracket, MatchDirection dir,
									 bool allowDepth = true );

	TextRange getMatchingBracket( TextPosition startPosition, const String& openBracket,
								  const String& closeBracket, MatchDirection dir,
								  bool matchingXMLTags = false );

	SyntaxHighlighter* getHighlighter() const;

	TextRange getWordRangeInPosition( const TextPosition& pos, bool basedOnHighlighter = true );

	TextRange getWordRangeInPosition( bool basedOnHighlighter = true );

	String getWordInPosition( const TextPosition& pos, bool basedOnHighlighter = true );

	String getWordInPosition( bool basedOnHighlighter = true );

	bool mightBeBinary() const;

	void setMightBeBinary( bool mightBeBinary );

	TextRange getActiveClientVisibleRange() const;

	bool hAsCpp() const;

	void setHAsCpp( bool hAsCpp );

	const Uint64& getModificationId() const;

	void stopActiveFindAll();

	bool isDoingTextInput() const;

	bool isInsertingText() const;

  protected:
	friend class UndoStack;

	Uint64 mModificationId{ 0 };
	UndoStack mUndoStack;
	std::string mFilePath;
	std::string mLoadingFilePath;
	std::array<Uint8, 16> mHash;
	URI mFileURI;
	URI mLoadingFileURI;
	FileInfo mFileRealPath;
	std::vector<TextDocumentLine> mLines;
	TextRanges mSelection;
	UnorderedSet<Client*> mClients;
	Mutex mClientsMutex;
	LineEnding mLineEnding{ LineEnding::LF };
	std::atomic<bool> mLoading{ false };
	std::atomic<bool> mRunningTransaction{ false };
	std::atomic<bool> mLoadingAsync{ false };
	bool mIsBOM{ false };
	bool mAutoDetectIndentType{ true };
	bool mForceNewLineAtEndOfFile{ false };
	bool mTrimTrailingWhitespaces{ false };
	bool mVerbose{ false };
	bool mAutoCloseBrackets{ false };
	bool mDirtyOnFileSystem{ false };
	bool mSaving{ false };
	bool mDeleteOnClose{ false };
	bool mMightBeBinary{ false };
	bool mHAsCpp{ false };
	bool mLastCursorChangeWasInteresting{ false };
	bool mDoingTextInput{ false };
	bool mInsertingText{ false };
	std::vector<std::pair<String::StringBaseType, String::StringBaseType>> mAutoCloseBracketsPairs;
	Uint32 mIndentWidth{ 4 };
	IndentType mIndentType{ IndentType::IndentTabs };
	Clock mTimer;
	SyntaxDefinition mSyntaxDefinition;
	std::string mDefaultFileName;
	Uint64 mCleanChangeId;
	Uint32 mPageSize{ 10 };
	UnorderedMap<std::string, DocumentCommand> mCommands;
	UnorderedMap<std::string, DocumentRefCommand> mRefCommands;
	String mNonWordChars;
	Client* mActiveClient{ nullptr };
	mutable Mutex mLoadingMutex;
	mutable Mutex mLoadingFilePathMutex;
	size_t mLastSelection{ 0 };
	std::unique_ptr<SyntaxHighlighter> mHighlighter;
	Mutex mStopFlagsMutex;
	UnorderedMap<bool*, std::unique_ptr<bool>> mStopFlags;

	void initializeCommands();

	void cleanChangeId();

	void notifyDocumentLoaded();

	void notifyDocumentReloaded();

	void notifyTextChanged( const DocumentContentChange& );

	void notifyCursorChanged( TextPosition selection = TextPosition() );

	void notifySelectionChanged( TextRange selection = TextRange() );

	void notifyDocumentSaved();

	void notifyDocumentClosed();

	void notifyLineCountChanged( const size_t& lastCount, const size_t& newCount );

	void notifyLineChanged( const Int64& lineIndex );

	void notifyUndoRedo( const UndoRedo& eventType );

	void notifyDirtyOnFileSystem();

	void notifyDocumentMoved();

	void notifySyntaxDefinitionChange();

	void notifiyDocumenLineMove( const Int64& fromLine, const Int64& numLines );

	void notifyInterstingCursorChange( TextPosition selection );

	void insertAtStartOfSelectedLines( const String& text, bool skipEmpty );

	void removeFromStartOfSelectedLines( const String& text, bool skipEmpty,
										 bool removeExtraSpaces = false );

	/** @return The number of lines removed (complete lines, not modified lines) */
	size_t remove( const size_t& cursorIdx, TextRange range, UndoStackContainer& undoStack,
				   const Time& time, bool fromUndoRedo = false );

	TextPosition insert( const size_t& cursorIdx, TextPosition position, const String& text,
						 UndoStackContainer& undoStack, const Time& time,
						 bool fromUndoRedo = false );

	void appendLineIfLastLine( const size_t& cursorIdx, Int64 line );

	void guessIndentType();

	std::vector<bool> autoCloseBrackets( const String& text );

	LoadStatus loadFromStream( IOStream& file, std::string path, bool callReset );

	TextRange findText( String text, TextPosition from = { 0, 0 }, bool caseSensitive = true,
						bool wholeWord = false, FindReplaceType type = FindReplaceType::Normal,
						TextRange restrictRange = TextRange() );

	TextRange findTextLast( String text, TextPosition from = { 0, 0 }, bool caseSensitive = true,
							bool wholeWord = false, FindReplaceType type = FindReplaceType::Normal,
							TextRange restrictRange = TextRange() );
};

struct TextSearchParams {
	String text;
	TextRange range = TextRange();
	bool caseSensitive{ false };
	bool wholeWord{ false };
	bool escapeSequences{ false };
	TextDocument::FindReplaceType type{ TextDocument::FindReplaceType::Normal };

	bool operator==( const TextSearchParams& other ) {
		return text == other.text && range == other.range && caseSensitive == other.caseSensitive &&
			   wholeWord == other.wholeWord && escapeSequences == other.escapeSequences &&
			   type == other.type;
	}

	bool operator!=( const TextSearchParams& other ) { return !( *this == other ); }

	bool isEmpty();

	void reset() {
		range = TextRange();
		text = "";
	}
};

}}} // namespace EE::UI::Doc

#endif
