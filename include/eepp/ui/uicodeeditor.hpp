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

class UIScrollBar;

class EE_API UICodeEditor : public UIWidget, public TextDocument::Client {
  public:
	static UICodeEditor* New();

	UICodeEditor();

	virtual ~UICodeEditor();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void draw();

	virtual void scheduledUpdate( const Time& time );

	void reset();

	void loadFromFile( const std::string& path );

	bool save();

	bool save( const std::string& path, const bool& utf8bom = false );

	bool save( IOStreamFile& stream, const bool& utf8bom = false );

	Font* getFont() const;

	const UIFontStyleConfig& getFontStyleConfig() const;

	UICodeEditor* setFont( Font* font );

	UICodeEditor* setFontSize( Float dpSize );

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

	const bool& getShowLineNumber() const;

	void setShowLineNumber( const bool& showLineNumber );

	const Color& getLineNumberBackgroundColor() const;

	void setLineNumberBackgroundColor( const Color& lineNumberBackgroundColor );

	const Color& getCurrentLineBackgroundColor() const;

	void setCurrentLineBackgroundColor( const Color& currentLineBackgroundColor );

	const Color& getCaretColor() const;

	void setCaretColor( const Color& caretColor );

	const Color& getIndentationGuideColor() const;

	void setIndentationGuideColor( const Color& color );

	const SyntaxColorScheme& getColorScheme() const;

	void setColorScheme( const SyntaxColorScheme& colorScheme );

	const Doc::TextDocument& getDocument() const;

	Doc::TextDocument& getDocument();

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

	void addKeybindsString( const std::map<std::string, std::string>& binds,
							const bool& allowLocked = false );

	void addKeybinds( const std::map<KeyBindings::Shortcut, std::string>& binds,
					  const bool& allowLocked = false );

	const bool& getHighlightCurrentLine() const;

	void setHighlightCurrentLine( const bool& highlightCurrentLine );

	const Uint32& getLineBreakingColumn() const;

	/** Set to 0 to hide. */
	void setLineBreakingColumn( const Uint32& lineBreakingColumn );

	void addUnlockedCommand( const std::string& command );

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

  protected:
	struct LastXOffset {
		TextPosition position;
		Float offset;
	};
	Font* mFont;
	UIFontStyleConfig mFontStyleConfig;
	Doc::TextDocument mDoc;
	Vector2f mScrollPos;
	Clock mBlinkTimer;
	bool mDirtyEditor;
	bool mCursorVisible;
	bool mMouseDown;
	bool mShowLineNumber;
	bool mShowIndentationGuide;
	bool mLocked;
	bool mHighlightCurrentLine;
	bool mHighlightMatchingBracket;
	bool mHighlightSelectionMatch;
	Uint32 mTabWidth;
	Int64 mLastColOffset;
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
	Color mIndentationGuideColor;
	Color mLineBreakColumnColor;
	Color mMatchingBracketColor;
	Color mSelectionMatchColor;
	SyntaxColorScheme mColorScheme;
	SyntaxHighlighter mHighlighter;
	UIScrollBar* mVScrollBar;
	LastXOffset mLastXOffset{{0, 0}, 0.f};
	KeyBindings mKeyBindings;
	std::unordered_set<std::string> mUnlockedCmd;
	Clock mLastDoubleClick;
	Uint32 mLineBreakingColumn{100};
	TextRange mMatchingBrackets;

	void checkMatchingBrackets();

	void updateColorScheme();

	void invalidateEditor();

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	void updateEditor();

	virtual void onDocumentTextChanged();

	virtual void onDocumentCursorChange( const TextPosition& );

	virtual void onDocumentSelectionChange( const TextRange& );

	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );

	virtual void onDocumentLineChanged( const Int64& lineIndex );

	std::pair<int, int> getVisibleLineRange();

	int getVisibleLinesCount();

	void scrollToMakeVisible( const TextPosition& position );

	void setScrollY( const Float& val, bool emmitEvent = true );

	virtual Int64 getColFromXOffset( Int64 line, const Float& x ) const;

	virtual Float getColXOffset( TextPosition position );

	virtual Float getXOffsetCol( const TextPosition& position ) const;

	virtual Float getTextWidth( const String& text ) const;

	Float getLineHeight() const;

	Float getCharacterSize() const;

	Float getGlyphWidth() const;

	void resetCursor();

	TextPosition resolveScreenPosition( const Vector2f& position ) const;

	Vector2f getViewPortLineCount() const;

	Sizef getMaxScroll() const;

	void updateScrollBar();

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

	virtual void drawMatchingBrackets( const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawSelectionMatch( const std::pair<int, int>& lineRange,
									 const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawLineText( const Int64& index, Vector2f position, const Float& fontSize );

	virtual void drawIndentationGuide( const std::pair<int, int>& lineRange,
									   const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawSelection( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								const Float& lineHeight );

	virtual void drawLineNumbers( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								  const Vector2f& screenStart, const Float& lineHeight,
								  const Float& lineNumberWidth, const int& lineNumberDigits,
								  const Float& fontSize );
};

}} // namespace EE::UI

#endif // EE_UI_UICODEEDIT_HPP
