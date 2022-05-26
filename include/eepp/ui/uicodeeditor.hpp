#ifndef EE_UI_UICODEEDIT_HPP
#define EE_UI_UICODEEDIT_HPP

#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Graphics;
using namespace EE::UI::Doc;

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UICodeEditor;
class UIWindow;
class UIScrollBar;
class UILoader;
class UIPopUpMenu;
class UIMenuItem;

class UICodeEditorModule {
  public:
	virtual std::string getTitle() = 0;
	virtual std::string getDescription() = 0;
	virtual bool hasGUIConfig() { return false; }
	virtual bool hasFileConfig() { return false; }
	virtual UIWindow* getGUIConfig() { return nullptr; }
	virtual std::string getFileConfigPath() { return ""; }

	virtual void onRegister( UICodeEditor* ) = 0;
	virtual void onUnregister( UICodeEditor* ) = 0;
	virtual bool onKeyDown( UICodeEditor*, const KeyEvent& ) { return false; }
	virtual bool onKeyUp( UICodeEditor*, const KeyEvent& ) { return false; }
	virtual bool onTextInput( UICodeEditor*, const TextInputEvent& ) { return false; }
	virtual void update( UICodeEditor* ) {}
	virtual void preDraw( UICodeEditor*, const Vector2f& /*startScroll*/,
						  const Float& /*lineHeight*/, const TextPosition& /*cursor*/ ) {}
	virtual void postDraw( UICodeEditor*, const Vector2f& /*startScroll*/,
						   const Float& /*lineHeight*/, const TextPosition& /*cursor*/ ) {}
	virtual void onFocus( UICodeEditor* ) {}
	virtual void onFocusLoss( UICodeEditor* ) {}
	virtual bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseUp( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseClick( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseDoubleClick( UICodeEditor*, const Vector2i&, const Uint32& ) {
		return false;
	}
	virtual bool onMouseOver( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseLeave( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onCreateContextMenu( UICodeEditor*, const Vector2i&, const Uint32& ) {
		return false;
	}
	virtual void drawBeforeLineText( UICodeEditor*, const Int64&, Vector2f, const Float&,
									 const Float& ){};
	virtual void drawAfterLineText( UICodeEditor*, const Int64&, Vector2f, const Float&,
									const Float& ){};
};

class EE_API DocEvent : public Event {
  public:
	DocEvent( Node* node, TextDocument* doc, const Uint32& eventType ) :
		Event( node, eventType ), doc( doc ) {}
	TextDocument* getDoc() const { return doc; }

  protected:
	TextDocument* doc;
};

class EE_API DocSyntaxDefEvent : public DocEvent {
  public:
	DocSyntaxDefEvent( Node* node, TextDocument* doc, const Uint32& eventType,
					   const std::string& oldLang, const std::string& newLang ) :
		DocEvent( node, doc, eventType ), oldLang( oldLang ), newLang( newLang ) {}
	const std::string& getOldLang() const { return oldLang; }
	const std::string& getNewLang() const { return newLang; }

  protected:
	TextDocument* doc;
	std::string oldLang;
	std::string newLang;
};

class EE_API UICodeEditor : public UIWidget, public TextDocument::Client {
  public:
	struct MinimapConfig {
		Float width{ 100 }; // dp width
		Float maxPercentWidth{
			0.1f }; // 0..1 max width that a minimap can ocupy on the editor view.
		bool syntaxHighlight{ true };
		Float scale{ 1 };
		int tabWidth{ 4 };
		bool drawBackground{ true };
		Float gutterWidth{ 5 }; // dp width
	};

	static UICodeEditor* New();

	static UICodeEditor* NewOpt( const bool& autoRegisterBaseCommands,
								 const bool& autoRegisterBaseKeybindings );

	static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	UICodeEditor( const bool& autoRegisterBaseCommands = true,
				  const bool& autoRegisterBaseKeybindings = true );

	virtual ~UICodeEditor();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void draw();

	virtual void scheduledUpdate( const Time& time );

	void reset();

	TextDocument::LoadStatus loadFromFile( const std::string& path );

	bool loadAsyncFromFile( const std::string& path, std::shared_ptr<ThreadPool> pool,
							std::function<void( std::shared_ptr<TextDocument>, bool )> onLoaded =
								std::function<void( std::shared_ptr<TextDocument>, bool )>() );

	TextDocument::LoadStatus loadFromURL(
		const std::string& url,
		const EE::Network::Http::Request::FieldTable& headers = Http::Request::FieldTable() );

	bool loadAsyncFromURL( const std::string& url,
						   const Http::Request::FieldTable& headers = Http::Request::FieldTable(),
						   std::function<void( std::shared_ptr<TextDocument>, bool )> onLoaded =
							   std::function<void( std::shared_ptr<TextDocument>, bool )>() );

	bool save();

	bool save( const std::string& path );

	bool save( IOStreamFile& stream );

	Font* getFont() const;

	const UIFontStyleConfig& getFontStyleConfig() const;

	UICodeEditor* setFont( Font* font );

	UICodeEditor* setFontSize( const Float& dpSize );

	const Float& getFontSize() const;

	UICodeEditor* setFontColor( const Color& color );

	const Color& getFontColor() const;

	UICodeEditor* setFontSelectedColor( const Color& color );

	const Color& getFontSelectedColor();

	UICodeEditor* setFontSelectionBackColor( const Color& color );

	const Color& getFontSelectionBackColor() const;

	UICodeEditor* setFontShadowColor( const Color& color );

	const Color& getFontShadowColor() const;

	UICodeEditor* setFontStyle( const Uint32& fontStyle );

	const Uint32& getTabWidth() const;

	UICodeEditor* setTabWidth( const Uint32& tabWidth );

	const Uint32& getFontStyle() const;

	const Float& getOutlineThickness() const;

	UICodeEditor* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UICodeEditor* setOutlineColor( const Color& outlineColor );

	const Float& getMouseWheelScroll() const;

	void setMouseWheelScroll( const Float& mouseWheelScroll );

	void setLineNumberPaddingLeft( const Float& dpLeft );

	void setLineNumberPaddingRight( const Float& dpRight );

	void setLineNumberPadding( const Float& dpPaddingLeft, const Float& dpPaddingRight );

	const Float& getLineNumberPaddingLeft() const;

	const Float& getLineNumberPaddingRight() const;

	size_t getLineNumberDigits() const;

	Float getLineNumberWidth() const;

	virtual Float getGutterWidth() const;

	const bool& getShowLineNumber() const;

	void setShowLineNumber( const bool& showLineNumber );

	const Color& getLineNumberBackgroundColor() const;

	void setLineNumberBackgroundColor( const Color& lineNumberBackgroundColor );

	const Color& getCurrentLineBackgroundColor() const;

	void setCurrentLineBackgroundColor( const Color& currentLineBackgroundColor );

	const Color& getCaretColor() const;

	void setCaretColor( const Color& caretColor );

	const Color& getWhitespaceColor() const;

	void setWhitespaceColor( const Color& color );

	const SyntaxColorScheme& getColorScheme() const;

	void setColorScheme( const SyntaxColorScheme& colorScheme );

	/** If the document is managed by more than one client you need to NOT auto register base
	 * commands and implement your own logic for those commands, since are dependant of the client
	 * state.
	 * @see registerCommands */
	std::shared_ptr<Doc::TextDocument> getDocumentRef() const;

	const Doc::TextDocument& getDocument() const;

	Doc::TextDocument& getDocument();

	void setDocument( std::shared_ptr<TextDocument> doc );

	bool isDirty() const;

	const bool& isLocked() const;

	void setLocked( bool locked );

	const Color& getLineNumberFontColor() const;

	void setLineNumberFontColor( const Color& lineNumberFontColor );

	const Color& getLineNumberActiveFontColor() const;

	void setLineNumberActiveFontColor( const Color& lineNumberActiveFontColor );

	bool isTextSelectionEnabled() const;

	void setTextSelection( const bool& active );

	KeyBindings& getKeyBindings();

	void setKeyBindings( const KeyBindings& keyBindings );

	void addKeyBindingString( const std::string& shortcut, const std::string& command,
							  const bool& allowLocked = false );

	void addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command,
						const bool& allowLocked = false );

	void replaceKeyBindingString( const std::string& shortcut, const std::string& command,
								  const bool& allowLocked = false );

	void replaceKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command,
							const bool& allowLocked = false );

	void addKeyBindsString( const std::map<std::string, std::string>& binds,
							const bool& allowLocked = false );

	void addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds,
					  const bool& allowLocked = false );

	const bool& getHighlightCurrentLine() const;

	void setHighlightCurrentLine( const bool& highlightCurrentLine );

	const Uint32& getLineBreakingColumn() const;

	/** Set to 0 to hide. */
	void setLineBreakingColumn( const Uint32& lineBreakingColumn );

	void addUnlockedCommand( const std::string& command );

	void addUnlockedCommands( const std::vector<std::string>& commands );

	bool isUnlockedCommand( const std::string& command );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	const bool& getHighlightMatchingBracket() const;

	void setHighlightMatchingBracket( const bool& highlightMatchingBracket );

	const Color& getMatchingBracketColor() const;

	void setMatchingBracketColor( const Color& matchingBracketColor );

	const bool& getHighlightSelectionMatch() const;

	void setHighlightSelectionMatch( const bool& highlightSelection );

	const Color& getSelectionMatchColor() const;

	void setSelectionMatchColor( const Color& highlightSelectionMatchColor );

	const bool& getEnableColorPickerOnSelection() const;

	void setEnableColorPickerOnSelection( const bool& enableColorPickerOnSelection );

	void setSyntaxDefinition( const SyntaxDefinition& definition );

	const SyntaxDefinition& getSyntaxDefinition() const;

	const bool& getHorizontalScrollBarEnabled() const;

	void setHorizontalScrollBarEnabled( const bool& horizontalScrollBarEnabled );

	bool getVerticalScrollBarEnabled() const;

	void setVerticalScrollBarEnabled( const bool& verticalScrollBarEnabled );

	const Time& getFindLongestLineWidthUpdateFrequency() const;

	void setFindLongestLineWidthUpdateFrequency( const Time& findLongestLineWidthUpdateFrequency );

	/** Doc commands executed in this editor. */
	TextPosition moveToLineOffset( const TextPosition& position, int offset );

	void moveToPreviousLine();

	void moveToNextLine();

	void selectToPreviousLine();

	void selectToNextLine();

	void registerKeybindings();

	void registerCommands();

	void moveScrollUp();

	void moveScrollDown();

	void indent();

	void unindent();

	void copy();

	void cut();

	void paste();

	void fontSizeGrow();

	void fontSizeShrink();

	void fontSizeReset();
	/** Doc commands executed in this editor. */

	const bool& getShowWhitespaces() const;

	void setShowWhitespaces( const bool& showWhitespaces );

	const String& getHighlightWord() const;

	void setHighlightWord( const String& highlightWord );

	const TextRange& getHighlightTextRange() const;

	void setHighlightTextRange( const TextRange& highlightSelection );

	void registerModule( UICodeEditorModule* module );

	void unregisterModule( UICodeEditorModule* module );

	virtual Int64 getColFromXOffset( Int64 line, const Float& x ) const;

	virtual Float getColXOffset( TextPosition position );

	virtual Float getXOffsetCol( const TextPosition& position );

	virtual Float getLineWidth( const Int64& lineIndex );

	size_t characterWidth( const String& str ) const;

	Float getTextWidth( const String& text ) const;

	Float getLineHeight() const;

	Float getCharacterSize() const;

	Float getGlyphWidth() const;

	const bool& getColorPreview() const;

	void setColorPreview( bool colorPreview );

	void goToLine( const TextPosition& position, bool centered = true );

	bool getAutoCloseBrackets() const;

	void setAutoCloseBrackets( bool autoCloseBracket );

	bool getInteractiveLinks() const;

	void setInteractiveLinks( bool newInteractiveLinks );

	UILoader* getLoader();

	bool getDisplayLoaderIfDocumentLoading() const;

	void setDisplayLoaderIfDocumentLoading( bool newDisplayLoaderIfDocumentLoading );

	size_t getMenuIconSize() const;

	void setMenuIconSize( size_t menuIconSize );

	bool getCreateDefaultContextMenuOptions() const;

	void setCreateDefaultContextMenuOptions( bool createDefaultContextMenuOptions );

	void openContainingFolder();

	void copyFilePath();

	void scrollToCursor( bool centered = true );

	void scrollTo( const TextPosition& position, bool centered = false,
				   bool forceExactPosition = false, bool scrollX = true );

	const MinimapConfig& getMinimapConfig() const;

	void setMinimapConfig( const MinimapConfig& newMinimapConfig );

	bool isMinimapShown() const;

	void showMinimap( bool showMinimap );

	bool getAutoCloseXMLTags() const;

	void setAutoCloseXMLTags( bool autoCloseXMLTags );

	const Time& getBlinkTime() const;

	void setBlinkTime( const Time& blinkTime );

	Int64 getCurrentColumnCount() const;

  protected:
	struct LastXOffset {
		TextPosition position;
		Float offset;
	};
	Font* mFont;
	UIFontStyleConfig mFontStyleConfig;
	std::shared_ptr<Doc::TextDocument> mDoc;
	Vector2f mScrollPos;
	Clock mBlinkTimer;
	Time mBlinkTime;
	bool mDirtyEditor{ false };
	bool mCursorVisible{ false };
	bool mMouseDown{ false };
	bool mShowLineNumber{ true };
	bool mShowWhitespaces{ true };
	bool mLocked{ false };
	bool mHighlightCurrentLine{ true };
	bool mHighlightMatchingBracket{ true };
	bool mHighlightSelectionMatch{ true };
	bool mEnableColorPickerOnSelection{ false };
	bool mVerticalScrollBarEnabled{ true };
	bool mHorizontalScrollBarEnabled{ true };
	bool mLongestLineWidthDirty{ true };
	bool mColorPreview{ false };
	bool mInteractiveLinks{ true };
	bool mHandShown{ false };
	bool mDisplayLoaderIfDocumentLoading{ true };
	bool mCreateDefaultContextMenuOptions{ true };
	bool mMinimapEnabled{ false };
	bool mMinimapDragging{ false };
	bool mMinimapHover{ false };
	bool mAutoCloseXMLTags{ false };
	TextRange mLinkPosition;
	String mLink;
	Uint32 mTabWidth;
	Vector2f mScroll;
	Float mMouseWheelScroll;
	Float mFontSize;
	Float mLineNumberPaddingLeft;
	Float mLineNumberPaddingRight;
	Color mLineNumberFontColor;
	Color mLineNumberActiveFontColor;
	Color mLineNumberBackgroundColor;
	Color mCurrentLineBackgroundColor;
	Color mCaretColor;
	Color mWhitespaceColor;
	Color mLineBreakColumnColor;
	Color mMatchingBracketColor;
	Color mSelectionMatchColor;
	Color mErrorColor;
	Color mWarningColor;
	Color mMinimapBackgroundColor;
	Color mMinimapVisibleAreaColor;
	Color mMinimapCurrentLineColor;
	Color mMinimapHoverColor;
	Color mMinimapSelectionColor;
	Color mMinimapHighlightColor;
	SyntaxColorScheme mColorScheme;
	SyntaxHighlighter mHighlighter;
	UIScrollBar* mVScrollBar;
	UIScrollBar* mHScrollBar;
	LastXOffset mLastXOffset{ { 0, 0 }, 0.f };
	KeyBindings mKeyBindings;
	std::unordered_set<std::string> mUnlockedCmd;
	Clock mLastDoubleClick;
	Uint32 mLineBreakingColumn{ 100 };
	TextRange mMatchingBrackets;
	Float mLongestLineWidth{ 0 };
	Time mFindLongestLineWidthUpdateFrequency;
	Clock mLongestLineWidthLastUpdate;
	String mHighlightWord;
	TextRange mHighlightTextRange;
	Color mPreviewColor;
	TextRange mPreviewColorRange;
	std::vector<UICodeEditorModule*> mModules;
	UILoader* mLoader{ nullptr };
	Float mGlyphWidth{ 0 };
	size_t mMenuIconSize{ 16 };
	UIPopUpMenu* mCurrentMenu{ nullptr };
	MinimapConfig mMinimapConfig;
	Int64 mMinimapScrollOffset{ 0 };

	UICodeEditor( const std::string& elementTag, const bool& autoRegisterBaseCommands = true,
				  const bool& autoRegisterBaseKeybindings = true );

	void checkMatchingBrackets();

	void updateColorScheme();

	void updateLongestLineWidth();

	void invalidateEditor();

	void invalidateLongestLineWidth();

	virtual void findLongestLine();

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onKeyUp( const KeyEvent& event );

	virtual bool onCreateContextMenu( const Vector2i& position, const Uint32& flags );

	Int64 calculateMinimapClickedLine( const Vector2i& position );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual void onCursorPosChange();

	void updateEditor();

	virtual void onDocumentTextChanged();

	virtual void onDocumentCursorChange( const TextPosition& );

	virtual void onDocumentSelectionChange( const TextRange& );

	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );

	virtual void onDocumentLineChanged( const Int64& lineIndex );

	virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& );

	virtual void onDocumentSaved( TextDocument* );

	virtual void onDocumentMoved( TextDocument* );

	void onDocumentClosed( TextDocument* doc );

	virtual void onDocumentDirtyOnFileSystem( TextDocument* doc );

	std::pair<Uint64, Uint64> getVisibleLineRange();

	int getVisibleLinesCount();

	void setScrollX( const Float& val, bool emmitEvent = true );

	void setScrollY( const Float& val, bool emmitEvent = true );

	void resetCursor();

	TextPosition resolveScreenPosition( const Vector2f& position, bool clamp = true ) const;

	Vector2f getViewPortLineCount() const;

	Sizef getMaxScroll() const;

	void updateScrollBar();

	void checkColorPickerAction();

	virtual void drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor );

	virtual void drawMatchingBrackets( const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawLineText( const Int64& line, Vector2f position, const Float& fontSize,
							   const Float& lineHeight );

	virtual void drawSelectionMatch( const std::pair<int, int>& lineRange,
									 const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawWordMatch( const String& text, const std::pair<int, int>& lineRange,
								const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawWhitespaces( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								  const Float& lineHeight );

	virtual void drawTextRange( const TextRange& range, const std::pair<int, int>& lineRange,
								const Vector2f& startScroll, const Float& lineHeight,
								const Color& backgroundColor );

	virtual void drawLineNumbers( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								  const Vector2f& screenStart, const Float& lineHeight,
								  const Float& lineNumberWidth, const int& lineNumberDigits,
								  const Float& fontSize );

	virtual void drawColorPreview( const Vector2f& startScroll, const Float& lineHeight );

	virtual void onFontChanged();

	virtual void onFontStyleChanged();

	virtual void onDocumentLoaded();

	virtual void onDocumentChanged();

	virtual Uint32 onMessage( const NodeMessage* msg );

	void checkMouseOverColor( const Vector2i& position );

	String checkMouseOverLink( const Vector2i& position );

	String resetLinkOver();

	void resetPreviewColor();

	void disableEditorFeatures();

	Float getViewportWidth( const bool& forceVScroll = false ) const;

	void udpateGlyphWidth();

	Drawable* findIcon( const std::string& name );

	void createDefaultContextMenuOptions( UIPopUpMenu* menu );

	UIMenuItem* menuAdd( UIPopUpMenu* menu, const std::string& translateKey,
						 const String& translateString, const std::string& icon,
						 const std::string& cmd );

	void drawMinimap( const Vector2f& start, const std::pair<Uint64, Uint64>& lineRange );

	Vector2f getScreenStart() const;

	Float getMinimapLineSpacing() const;

	bool isMinimapFileTooLarge() const;

	Rectf getMinimapRect( const Vector2f& start ) const;

	Float getMinimapWidth() const;

	void updateMipmapHover( const Vector2f& position );

	bool checkAutoCloseXMLTag( const String& text );
};

}} // namespace EE::UI

#endif // EE_UI_UICODEEDIT_HPP
