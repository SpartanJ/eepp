#include <algorithm>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

UICodeEditor* UICodeEditor::New() {
	return eeNew( UICodeEditor, () );
}

UICodeEditor::UICodeEditor() :
	UIWidget( "codeeditor" ),
	mFont( FontManager::instance()->getByName( "monospace" ) ),
	mDirtyEditor( false ),
	mCursorVisible( false ),
	mMouseDown( false ),
	mShowLineNumber( true ),
	mShowIndentationGuide( true ),
	mLocked( false ),
	mHighlightCurrentLine( true ),
	mHighlightMatchingBracket( true ),
	mHighlightSelectionMatch( true ),
	mTabWidth( 4 ),
	mLastColOffset( 0 ),
	mMouseWheelScroll( 50 ),
	mFontSize( mFontStyleConfig.getFontCharacterSize() ),
	mLineNumberPaddingLeft( 8 ),
	mLineNumberPaddingRight( 8 ),
	mHighlighter( &mDoc ),
	mKeyBindings( getUISceneNode()->getWindow()->getInput() ) {
	mFlags |= UI_TAB_STOP;
	setTextSelection( true );
	setColorScheme( SyntaxColorScheme::getDefault() );
	mVScrollBar = UIScrollBar::NewVertical();
	mVScrollBar->setParent( this );
	mVScrollBar->addEventListener( Event::OnSizeChange,
								   [&]( const Event* ) { updateScrollBar(); } );
	mVScrollBar->addEventListener( Event::OnValueChange, [&]( const Event* ) {
		setScrollY( mVScrollBar->getValue() * getMaxScroll().y, false );
	} );

	if ( NULL == mFont )
		eePRINTL( "A monospace font must be loaded to be able to use the code editor.\nTry loading "
				  "a font with the name \"monospace\"" );

	setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );

	clipEnable();
	mDoc.registerClient( *this );
	subscribeScheduledUpdate();

	registerCommands();
	registerKeybindings();
}

UICodeEditor::~UICodeEditor() {
	mDoc.unregisterClient( *this );
}

Uint32 UICodeEditor::getType() const {
	return UI_TYPE_CODEEDITOR;
}

bool UICodeEditor::isType( const Uint32& type ) const {
	return type == getType() || UIWidget::isType( type );
}

void UICodeEditor::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "codeeditor" );
}

void UICodeEditor::draw() {
	UIWidget::draw();

	if ( mFont == NULL )
		return;

	if ( mDirtyEditor ) {
		updateEditor();
	}

	Color col;
	std::pair<int, int> lineRange = getVisibleLineRange();
	Float charSize = PixelDensity::pxToDp( getCharacterSize() );
	Float lineHeight = getLineHeight();
	int lineNumberDigits = getLineNumberDigits();
	Float lineNumberWidth = mShowLineNumber ? getLineNumberWidth() : 0.f;
	Vector2f screenStart( eefloor( mScreenPos.x + mRealPadding.Left ),
						  eefloor( mScreenPos.y + mRealPadding.Top ) );
	Vector2f start( screenStart.x + lineNumberWidth, screenStart.y );
	Vector2f startScroll( start - mScroll );
	Primitives primitives;
	TextPosition cursor( mDoc.getSelection().start() );

	if ( !mLocked && mHighlightCurrentLine ) {
		primitives.setColor( Color( mCurrentLineBackgroundColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle( Rectf(
			Vector2f( startScroll.x + mScroll.x, startScroll.y + cursor.line() * lineHeight ),
			Sizef( mSize.getWidth(), lineHeight ) ) );
	}

	if ( mLineBreakingColumn ) {
		Float lineBreakingOffset = start.x + getGlyphWidth() * mLineBreakingColumn;
		primitives.setColor( Color( mLineBreakColumnColor ).blendAlpha( mAlpha ) );
		primitives.drawLine(
			{{lineBreakingOffset, start.y}, {lineBreakingOffset, start.y + mSize.getHeight()}} );
	}

	if ( mHighlightMatchingBracket ) {
		drawMatchingBrackets( startScroll, lineHeight );
	}

	if ( mHighlightSelectionMatch && mDoc.hasSelection() &&
		 mDoc.getSelection().start().line() == mDoc.getSelection().end().line() ) {
		drawSelectionMatch( lineRange, startScroll, lineHeight );
	}

	if ( mDoc.hasSelection() ) {
		drawSelection( lineRange, startScroll, lineHeight );
	}

	// Draw tab marker
	if ( mShowIndentationGuide ) {
		drawIndentationGuide( lineRange, startScroll, lineHeight );
	}

	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		drawLineText( i, {startScroll.x, startScroll.y + lineHeight * i}, charSize );
	}

	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight );

		primitives.setColor( Color( mCaretColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), lineHeight ) ) );
	}

	if ( mShowLineNumber ) {
		drawLineNumbers( lineRange, startScroll, screenStart, lineHeight, lineNumberWidth,
						 lineNumberDigits, charSize );
	}
}

void UICodeEditor::scheduledUpdate( const Time& ) {
	if ( hasFocus() && getUISceneNode()->getWindow()->isActive() ) {
		if ( mBlinkTimer.getElapsedTime().asSeconds() > 0.5f ) {
			mCursorVisible = !mCursorVisible;
			mBlinkTimer.restart();
			invalidateDraw();
		}
	}

	if ( mMouseDown &&
		 !( getUISceneNode()->getWindow()->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
		mMouseDown = false;
		getUISceneNode()->getWindow()->getInput()->captureMouse( false );
	}

	if ( mHighlighter.updateDirty( getVisibleLinesCount() ) ) {
		invalidateDraw();
	}
}

void UICodeEditor::reset() {
	mDoc.reset();
	invalidateDraw();
}

void UICodeEditor::loadFromFile( const std::string& path ) {
	mDoc.loadFromFile( path );
	invalidateEditor();
}

bool UICodeEditor::save() {
	return mDoc.save();
}

bool UICodeEditor::save( const std::string& path, const bool& utf8bom ) {
	return mDoc.save( path, utf8bom );
}

bool UICodeEditor::save( IOStreamFile& stream, const bool& utf8bom ) {
	return mDoc.save( stream, utf8bom );
}

Font* UICodeEditor::getFont() const {
	return mFont;
}

const UIFontStyleConfig& UICodeEditor::getFontStyleConfig() const {
	return mFontStyleConfig;
}

UICodeEditor* UICodeEditor::setFont( Font* font ) {
	if ( mFont != font ) {
		mFont = font;
		invalidateDraw();
		invalidateEditor();
	}
	return this;
}

UICodeEditor* UICodeEditor::setFontSize( Float dpSize ) {
	if ( mFontStyleConfig.CharacterSize != dpSize ) {
		mFontStyleConfig.CharacterSize = dpSize;
		mFontSize = dpSize;
		invalidateDraw();
	}
	return this;
}

const Float& UICodeEditor::getFontSize() const {
	return mFontStyleConfig.getFontCharacterSize();
}

UICodeEditor* UICodeEditor::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.getFontColor() != color ) {
		mFontStyleConfig.FontColor = color;
		invalidateDraw();
	}
	return this;
}

const Color& UICodeEditor::getFontColor() const {
	return mFontStyleConfig.getFontColor();
}

const Color& UICodeEditor::getFontSelectedColor() {
	return mFontStyleConfig.getFontSelectedColor();
}

UICodeEditor* UICodeEditor::setFontSelectedColor( const Color& color ) {
	if ( mFontStyleConfig.getFontSelectedColor() != color ) {
		mFontStyleConfig.FontSelectedColor = color;
		invalidateDraw();
	}
	return this;
}

UICodeEditor* UICodeEditor::setFontSelectionBackColor( const Color& color ) {
	if ( mFontStyleConfig.getFontSelectionBackColor() != color ) {
		mFontStyleConfig.FontSelectionBackColor = color;
		invalidateDraw();
	}
	return this;
}

const Color& UICodeEditor::getFontSelectionBackColor() const {
	return mFontStyleConfig.getFontSelectionBackColor();
}

const Uint32& UICodeEditor::getTabWidth() const {
	return mTabWidth;
}

UICodeEditor* UICodeEditor::setTabWidth( const Uint32& tabWidth ) {
	if ( mTabWidth != tabWidth ) {
		mTabWidth = tabWidth;
		mDoc.setTabWidth( mTabWidth );
	}
	return this;
}

const Float& UICodeEditor::getMouseWheelScroll() const {
	return mMouseWheelScroll;
}

void UICodeEditor::setMouseWheelScroll( const Float& mouseWheelScroll ) {
	mMouseWheelScroll = mouseWheelScroll;
}

void UICodeEditor::setLineNumberPaddingLeft( const Float& dpLeft ) {
	if ( dpLeft != mLineNumberPaddingLeft ) {
		mLineNumberPaddingLeft = dpLeft;
		invalidateDraw();
	}
}

void UICodeEditor::setLineNumberPaddingRight( const Float& dpRight ) {
	if ( dpRight != mLineNumberPaddingRight ) {
		mLineNumberPaddingRight = dpRight;
		invalidateDraw();
	}
}

void UICodeEditor::setLineNumberPadding( const Float& dpPaddingLeft, const Float& dpPaddingRight ) {
	setLineNumberPaddingLeft( dpPaddingLeft );
	setLineNumberPaddingRight( dpPaddingRight );
}

const Float& UICodeEditor::getLineNumberPaddingLeft() const {
	return mLineNumberPaddingLeft;
}

const Float& UICodeEditor::getLineNumberPaddingRight() const {
	return mLineNumberPaddingRight;
}

size_t UICodeEditor::getLineNumberDigits() const {
	return eemax<size_t>( 2UL, Math::countDigits( mDoc.linesCount() ) );
}

Float UICodeEditor::getLineNumberWidth() const {
	return eeceil( getLineNumberDigits() * getGlyphWidth() + getLineNumberPaddingLeft() +
				   getLineNumberPaddingRight() );
}

const bool& UICodeEditor::getShowLineNumber() const {
	return mShowLineNumber;
}

void UICodeEditor::setShowLineNumber( const bool& showLineNumber ) {
	if ( mShowLineNumber != showLineNumber ) {
		mShowLineNumber = showLineNumber;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getLineNumberBackgroundColor() const {
	return mLineNumberBackgroundColor;
}

void UICodeEditor::setLineNumberBackgroundColor( const Color& lineNumberBackgroundColor ) {
	if ( mLineNumberBackgroundColor != lineNumberBackgroundColor ) {
		mLineNumberBackgroundColor = lineNumberBackgroundColor;
		invalidateDraw();
	};
}

const Color& UICodeEditor::getCurrentLineBackgroundColor() const {
	return mCurrentLineBackgroundColor;
}

void UICodeEditor::setCurrentLineBackgroundColor( const Color& currentLineBackgroundColor ) {
	mCurrentLineBackgroundColor = currentLineBackgroundColor;
}

const Color& UICodeEditor::getCaretColor() const {
	return mCaretColor;
}

void UICodeEditor::setCaretColor( const Color& caretColor ) {
	if ( mCaretColor != caretColor ) {
		mCaretColor = caretColor;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getIndentationGuideColor() const {
	return mIndentationGuideColor;
}

void UICodeEditor::setIndentationGuideColor( const Color& indentationGuide ) {
	if ( mIndentationGuideColor != indentationGuide ) {
		mIndentationGuideColor = indentationGuide;
		invalidateDraw();
	}
}

const SyntaxColorScheme& UICodeEditor::getColorScheme() const {
	return mColorScheme;
}

void UICodeEditor::updateColorScheme() {
	setBackgroundColor( mColorScheme.getEditorColor( "background" ) );
	setFontColor( mColorScheme.getEditorColor( "text" ) );
	mFontStyleConfig.setFontSelectionBackColor( mColorScheme.getEditorColor( "selection" ) );
	mLineNumberFontColor = mColorScheme.getEditorColor( "line_number" );
	mLineNumberActiveFontColor = mColorScheme.getEditorColor( "line_number2" );
	mLineNumberBackgroundColor = mColorScheme.getEditorColor( "line_number_background" );
	mCurrentLineBackgroundColor = mColorScheme.getEditorColor( "line_highlight" );
	mCaretColor = mColorScheme.getEditorColor( "caret" );
	mIndentationGuideColor = mColorScheme.getEditorColor( "guide" );
	mLineBreakColumnColor = mColorScheme.getEditorColor( "line_break_column" );
	mMatchingBracketColor = mColorScheme.getEditorColor( "matching_bracket" );
	mSelectionMatchColor = mColorScheme.getEditorColor( "matching_selection" );
}

void UICodeEditor::setColorScheme( const SyntaxColorScheme& colorScheme ) {
	mColorScheme = colorScheme;
	updateColorScheme();
	invalidateDraw();
}

const TextDocument& UICodeEditor::getDocument() const {
	return mDoc;
}

TextDocument& UICodeEditor::getDocument() {
	return mDoc;
}

bool UICodeEditor::isDirty() const {
	return mDoc.isDirty();
}

void UICodeEditor::invalidateEditor() {
	mDirtyEditor = true;
}

Uint32 UICodeEditor::onFocus() {
	if ( !mLocked ) {
		getUISceneNode()->getWindow()->startTextInput();
		resetCursor();
	}
	return UIWidget::onFocus();
}

Uint32 UICodeEditor::onFocusLoss() {
	mMouseDown = false;
	mCursorVisible = false;
	getSceneNode()->getWindow()->stopTextInput();
	return UIWidget::onFocusLoss();
}

Uint32 UICodeEditor::onTextInput( const TextInputEvent& event ) {
	if ( mLocked || NULL == mFont )
		return 1;

	if ( !getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
		if ( getUISceneNode()->getWindow()->getInput()->isAltPressed() &&
			 !event.getText().empty() && event.getText()[0] == '\t' )
			return 1;

		mDoc.textInput( event.getText() );
	}
	return 1;
}

Uint32 UICodeEditor::onKeyDown( const KeyEvent& event ) {
	if ( NULL == mFont )
		return 1;

	std::string cmd = mKeyBindings.getCommandFromKeyBind( {event.getKeyCode(), event.getMod()} );

	if ( !cmd.empty() ) {
		// Allow copy selection on locked mode
		if ( !mLocked || mUnlockedCmd.find( cmd ) != mUnlockedCmd.end() )
			mDoc.execute( cmd );
	}

	return 0;
}

TextPosition UICodeEditor::resolveScreenPosition( const Vector2f& position ) const {
	Vector2f localPos( position );
	worldToNode( localPos );
	localPos = PixelDensity::dpToPx( localPos );
	localPos += mScroll;
	localPos.x -= mRealPadding.Left + ( mShowLineNumber ? getLineNumberWidth() : 0.f );
	localPos.y -= mRealPadding.Top;
	Int64 line = eeclamp<Int64>( (Int64)eefloor( localPos.y / getLineHeight() ), 0,
								 ( Int64 )( mDoc.linesCount() - 1 ) );
	return TextPosition( line, getColFromXOffset( line, localPos.x ) );
}

Vector2f UICodeEditor::getViewPortLineCount() const {
	return Vector2f( eefloor( ( mSize.getWidth() - mRealPadding.Left ) / getGlyphWidth() ),
					 eefloor( ( mSize.getHeight() - mRealPadding.Top ) / getLineHeight() ) );
}

Sizef UICodeEditor::getMaxScroll() const {
	Vector2f vplc( getViewPortLineCount() );
	return Sizef(
		eefloor( ( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right ) / getGlyphWidth() ),
		vplc.y > mDoc.linesCount() - 1
			? 0.f
			: eefloor( mDoc.linesCount() - getViewPortLineCount().y ) * getLineHeight() );
}

Uint32 UICodeEditor::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( isTextSelectionEnabled() && !getUISceneNode()->getEventDispatcher()->isNodeDragging() &&
		 NULL != mFont && !mMouseDown && ( flags & EE_BUTTON_LMASK ) ) {
		mMouseDown = true;
		Input* input = getUISceneNode()->getWindow()->getInput();
		input->captureMouse( true );
		if ( input->isShiftPressed() ) {
			mDoc.selectTo( resolveScreenPosition( position.asFloat() ) );
		} else {
			mDoc.setSelection( resolveScreenPosition( position.asFloat() ) );
		}
	}
	return UIWidget::onMouseDown( position, flags );
}

Uint32 UICodeEditor::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	if ( isTextSelectionEnabled() && !getUISceneNode()->getEventDispatcher()->isNodeDragging() &&
		 NULL != mFont && mMouseDown && ( flags & EE_BUTTON_LMASK ) ) {
		TextRange selection = mDoc.getSelection();
		selection.setStart( resolveScreenPosition( position.asFloat() ) );
		mDoc.setSelection( selection );
	}
	return UIWidget::onMouseMove( position, flags );
}

Uint32 UICodeEditor::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( NULL == mFont )
		return UIWidget::onMouseUp( position, flags );

	if ( flags & EE_BUTTON_LMASK ) {
		if ( mMouseDown ) {
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		}
	} else if ( flags & EE_BUTTON_WDMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
			mFontStyleConfig.CharacterSize = eemax<Float>( 4, mFontStyleConfig.CharacterSize - 1 );
		} else {
			setScrollY( mScroll.y + PixelDensity::dpToPx( mMouseWheelScroll ) );
		}
		invalidateDraw();
	} else if ( flags & EE_BUTTON_WUMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
			mFontStyleConfig.CharacterSize = eemin<Float>( 96, mFontStyleConfig.CharacterSize + 1 );
		} else {
			setScrollY( mScroll.y - PixelDensity::dpToPx( mMouseWheelScroll ) );
		}
		invalidateDraw();
	}
	return UIWidget::onMouseUp( position, flags );
}

Uint32 UICodeEditor::onMouseClick( const Vector2i&, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) &&
		 mLastDoubleClick.getElapsedTime() < Milliseconds( 300.f ) ) {
		mDoc.selectLine();
	}
	return 1;
}

Uint32 UICodeEditor::onMouseDoubleClick( const Vector2i&, const Uint32& flags ) {
	if ( mLocked || NULL == mFont )
		return 1;

	if ( flags & EE_BUTTON_LMASK ) {
		mDoc.selectWord();
		mLastDoubleClick.restart();
	}
	return 1;
}

Uint32 UICodeEditor::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	getUISceneNode()->setCursor( !mLocked ? Cursor::IBeam : Cursor::Arrow );
	return UIWidget::onMouseOver( position, flags );
}

Uint32 UICodeEditor::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	getUISceneNode()->setCursor( Cursor::Arrow );
	return UIWidget::onMouseLeave( position, flags );
}

void UICodeEditor::onSizeChange() {
	UIWidget::onSizeChange();
	invalidateEditor();
}

void UICodeEditor::onPaddingChange() {
	UIWidget::onPaddingChange();
	invalidateEditor();
}

void UICodeEditor::updateScrollBar() {
	mVScrollBar->setPixelsSize( mVScrollBar->getPixelsSize().getWidth(), mSize.getHeight() );
	mVScrollBar->setPixelsPosition( mSize.getWidth() - mVScrollBar->getPixelsSize().getWidth(), 0 );
	int notVisibleLineCount = (int)mDoc.linesCount() - (int)getViewPortLineCount().y;
	mVScrollBar->setPageStep( getViewPortLineCount().y / (float)mDoc.linesCount() );
	mVScrollBar->setClickStep( 0.2f );
	mVScrollBar->setEnabled( notVisibleLineCount > 0 );
	mVScrollBar->setVisible( notVisibleLineCount > 0 );
	setScrollY( mScroll.y );
}

void UICodeEditor::updateEditor() {
	mDoc.setPageSize( getVisibleLinesCount() );
	scrollToMakeVisible( mDoc.getSelection().start() );
	updateScrollBar();
	mDirtyEditor = false;
}

void UICodeEditor::onDocumentTextChanged() {
	invalidateDraw();
	sendCommonEvent( Event::OnTextChanged );
}

void UICodeEditor::onDocumentCursorChange( const Doc::TextPosition& ) {
	resetCursor();
	checkMatchingBrackets();
	invalidateEditor();
	invalidateDraw();
}

void UICodeEditor::onDocumentSelectionChange( const Doc::TextRange& ) {
	resetCursor();
	invalidateDraw();
	sendCommonEvent( Event::OnSelectionChanged );
}

void UICodeEditor::onDocumentLineCountChange( const size_t&, const size_t& ) {
	updateScrollBar();
}

void UICodeEditor::onDocumentLineChanged( const Int64& lineIndex ) {
	mHighlighter.invalidate( lineIndex );
}

std::pair<int, int> UICodeEditor::getVisibleLineRange() {
	Float lineHeight = getLineHeight();
	Float minLine = eemax( 0.f, eefloor( mScroll.y / lineHeight ) );
	Float maxLine = eemin( mDoc.linesCount() - 1.f,
						   eefloor( ( mSize.getHeight() + mScroll.y ) / lineHeight ) + 1 );
	return std::make_pair<int, int>( (int)minLine, (int)maxLine );
}

int UICodeEditor::getVisibleLinesCount() {
	auto lines = getVisibleLineRange();
	return lines.second - lines.first;
}

void UICodeEditor::scrollToMakeVisible( const TextPosition& position ) {
	auto lineRange = getVisibleLineRange();

	if ( position.line() <= lineRange.first || position.line() >= lineRange.second - 2 ) {
		// Vertical Scroll
		Float lineHeight = getLineHeight();
		Float min = eefloor( lineHeight * ( eemax<Float>( 0, position.line() - 1 ) ) );
		Float max = eefloor( lineHeight * ( position.line() + 2 ) - mSize.getHeight() );
		if ( min < mScroll.y )
			setScrollY( min );
		else if ( max > mScroll.y )
			setScrollY( max );
	}

	// Horizontal Scroll
	Float offsetX = getXOffsetCol( position );
	Float glyphSize = getGlyphWidth();
	Float minVisibility = glyphSize * 10;
	Float viewPortWidth = eefloor( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right );
	if ( offsetX + minVisibility > viewPortWidth - mScroll.x ) {
		mScroll.x = eefloor( eemax( 0.f, offsetX + minVisibility - viewPortWidth ) );
	} else if ( offsetX < mScroll.x ) {
		mScroll.x = eefloor( eemax( 0.f, offsetX - minVisibility ) );
	}
	invalidateDraw();
}

void UICodeEditor::setScrollY( const Float& val, bool emmitEvent ) {
	Float oldVal = mScroll.y;
	mScroll.y = eefloor( eeclamp<Float>( val, 0, getMaxScroll().y ) );
	if ( oldVal != mScroll.y ) {
		invalidateDraw();
		if ( emmitEvent )
			mVScrollBar->setValue( mScroll.y / getMaxScroll().y, false );
	}
}

Float UICodeEditor::getXOffsetCol( const TextPosition& position ) const {
	const String& line = mDoc.line( position.line() ).getText();
	Float glyphWidth = getGlyphWidth();
	Float x = 0;
	for ( auto i = 0; i < position.column(); i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * mTabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
	}
	return x;
}

Float UICodeEditor::getTextWidth( const String& line ) const {
	Float glyphWidth = getGlyphWidth();
	size_t len = line.length();
	Float x = 0;
	for ( size_t i = 0; i < len; i++ )
		x += ( line[i] == '\t' ) ? glyphWidth * mTabWidth : glyphWidth;
	return x;
}

Float UICodeEditor::getColXOffset( TextPosition position ) {
	position.setLine( eeclamp<Int64>( position.line(), 0L, mDoc.linesCount() - 1 ) );
	// This is different from sanitizePosition, sinze allows the last character.
	position.setColumn( eeclamp<Int64>( position.column(), 0L,
										eemax<Int64>( 0, mDoc.line( position.line() ).size() ) ) );
	return getTextWidth( mDoc.line( position.line() ).substr( 0, position.column() ) );
}

const bool& UICodeEditor::isLocked() const {
	return mLocked;
}

void UICodeEditor::setLocked( bool locked ) {
	if ( mLocked != locked ) {
		mLocked = locked;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getLineNumberFontColor() const {
	return mLineNumberFontColor;
}

void UICodeEditor::setLineNumberFontColor( const Color& lineNumberFontColor ) {
	if ( lineNumberFontColor != mLineNumberFontColor ) {
		mLineNumberFontColor = lineNumberFontColor;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getLineNumberActiveFontColor() const {
	return mLineNumberActiveFontColor;
}

void UICodeEditor::setLineNumberActiveFontColor( const Color& lineNumberActiveFontColor ) {
	if ( mLineNumberActiveFontColor != lineNumberActiveFontColor ) {
		mLineNumberActiveFontColor = lineNumberActiveFontColor;
		invalidateDraw();
	}
}

KeyBindings& UICodeEditor::getKeyBindings() {
	return mKeyBindings;
}

void UICodeEditor::setKeyBindings( const KeyBindings& keyBindings ) {
	mKeyBindings = keyBindings;
}

void UICodeEditor::addKeyBindingString( const std::string& shortcut, const std::string& command,
										const bool& allowLocked ) {
	mKeyBindings.addKeybindString( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command,
								  const bool& allowLocked ) {
	mKeyBindings.addKeybind( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::replaceKeyBindingString( const std::string& shortcut, const std::string& command,
											const bool& allowLocked ) {
	mKeyBindings.replaceKeybindString( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::replaceKeyBinding( const KeyBindings::Shortcut& shortcut,
									  const std::string& command, const bool& allowLocked ) {
	mKeyBindings.replaceKeybind( shortcut, command );
	if ( allowLocked )
		mUnlockedCmd.insert( command );
}

void UICodeEditor::addKeybindsString( const std::map<std::string, std::string>& binds,
									  const bool& allowLocked ) {
	mKeyBindings.addKeybindsString( binds );
	for ( auto bind : binds ) {
		if ( allowLocked ) {
			mUnlockedCmd.insert( bind.second );
		}
	}
}

void UICodeEditor::addKeybinds( const std::map<KeyBindings::Shortcut, std::string>& binds,
								const bool& allowLocked ) {
	mKeyBindings.addKeybinds( binds );
	for ( auto bind : binds ) {
		if ( allowLocked ) {
			mUnlockedCmd.insert( bind.second );
		}
	}
}

const bool& UICodeEditor::getHighlightCurrentLine() const {
	return mHighlightCurrentLine;
}

void UICodeEditor::setHighlightCurrentLine( const bool& highlightCurrentLine ) {
	if ( mHighlightCurrentLine != highlightCurrentLine ) {
		mHighlightCurrentLine = highlightCurrentLine;
		invalidateDraw();
	}
}

const Uint32& UICodeEditor::getLineBreakingColumn() const {
	return mLineBreakingColumn;
}

void UICodeEditor::setLineBreakingColumn( const Uint32& lineBreakingColumn ) {
	if ( lineBreakingColumn != mLineBreakingColumn ) {
		mLineBreakingColumn = lineBreakingColumn;
		invalidateDraw();
	}
}

void UICodeEditor::addUnlockedCommand( const std::string& command ) {
	mUnlockedCmd.insert( command );
}

UICodeEditor* UICodeEditor::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		invalidateDraw();
	}

	return this;
}

UICodeEditor* UICodeEditor::setFontStyle( const Uint32& fontStyle ) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mFontStyleConfig.Style = fontStyle;
		invalidateDraw();
	}

	return this;
}

const Color& UICodeEditor::getFontShadowColor() const {
	return mFontStyleConfig.getFontShadowColor();
}

const Uint32& UICodeEditor::getFontStyle() const {
	return mFontStyleConfig.getFontStyle();
}

const Float& UICodeEditor::getOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

UICodeEditor* UICodeEditor::setOutlineThickness( const Float& outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mFontStyleConfig.OutlineThickness = outlineThickness;
		invalidateDraw();
	}

	return this;
}

const Color& UICodeEditor::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

UICodeEditor* UICodeEditor::setOutlineColor( const Color& outlineColor ) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mFontStyleConfig.OutlineColor = outlineColor;
		invalidateDraw();
	}

	return this;
}

bool UICodeEditor::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

void UICodeEditor::setTextSelection( const bool& active ) {
	if ( active ) {
		mFlags |= UI_TEXT_SELECTION_ENABLED;
	} else {
		mFlags &= ~UI_TEXT_SELECTION_ENABLED;
	}
}

bool UICodeEditor::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Locked:
			setLocked( attribute.asBool() );
			break;
		case PropertyId::Color:
			setFontColor( attribute.asColor() );
			break;
		case PropertyId::ShadowColor:
			setFontShadowColor( attribute.asColor() );
			break;
		case PropertyId::SelectionColor:
			setFontSelectedColor( attribute.asColor() );
			break;
		case PropertyId::SelectionBackColor:
			setFontSelectionBackColor( attribute.asColor() );
			break;
		case PropertyId::FontFamily: {
			Font* font = FontManager::instance()->getByName( attribute.asString() );
			if ( NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			setFontSize( lengthFromValueAsDp( attribute ) );
			break;
		case PropertyId::FontStyle: {
			setFontStyle( attribute.asFontStyle() );
			break;
		}
		case PropertyId::TextStrokeWidth:
			setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextSelection:
			setTextSelection( attribute.asBool() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::string UICodeEditor::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Locked:
			return isLocked() ? "true" : "false";
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::ShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::SelectionColor:
			return getFontSelectedColor().toHexString();
		case PropertyId::SelectionBackColor:
			return getFontSelectionBackColor().toHexString();
		case PropertyId::FontFamily:
			return NULL != getFont() ? getFont()->getName() : "";
		case PropertyId::FontSize:
			return String::format( "%.2fdp", getFontSize() );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::toStr( PixelDensity::dpToPx( getOutlineThickness() ) );
		case PropertyId::TextStrokeColor:
			return getOutlineColor().toHexString();
		case PropertyId::TextSelection:
			return isTextSelectionEnabled() ? "true" : "false";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

const bool& UICodeEditor::getHighlightMatchingBracket() const {
	return mHighlightMatchingBracket;
}

void UICodeEditor::setHighlightMatchingBracket( const bool& highlightMatchingBracket ) {
	if ( highlightMatchingBracket != mHighlightMatchingBracket ) {
		mHighlightMatchingBracket = highlightMatchingBracket;
		checkMatchingBrackets();
		invalidateDraw();
	}
}

const Color& UICodeEditor::getMatchingBracketColor() const {
	return mMatchingBracketColor;
}

void UICodeEditor::setMatchingBracketColor( const Color& matchingBracketColor ) {
	if ( matchingBracketColor != mMatchingBracketColor ) {
		mMatchingBracketColor = matchingBracketColor;
		invalidateDraw();
	}
}

const bool& UICodeEditor::getHighlightSelectionMatch() const {
	return mHighlightSelectionMatch;
}

void UICodeEditor::setHighlightSelectionMatch( const bool& highlightSelection ) {
	if ( highlightSelection != mHighlightSelectionMatch ) {
		mHighlightSelectionMatch = highlightSelection;
		invalidateDraw();
	}
}

const Color& UICodeEditor::getSelectionMatchColor() const {
	return mSelectionMatchColor;
}

void UICodeEditor::setSelectionMatchColor( const Color& highlightSelectionMatchColor ) {
	if ( highlightSelectionMatchColor != mSelectionMatchColor ) {
		mSelectionMatchColor = highlightSelectionMatchColor;
		invalidateDraw();
	}
}

void UICodeEditor::checkMatchingBrackets() {
	if ( mHighlightMatchingBracket ) {
		mMatchingBrackets = TextRange();
		std::vector<String::StringBaseType> open{'{', '(', '['};
		std::vector<String::StringBaseType> close{'}', ')', ']'};
		TextPosition pos = mDoc.getSelection().start();
		TextDocumentLine& line = mDoc.line( pos.line() );
		auto isOpenIt = std::find( open.begin(), open.end(), line[pos.column()] );
		auto isCloseIt = std::find( close.begin(), close.end(), line[pos.column()] );
		if ( ( isOpenIt == open.end() && isCloseIt == close.end() ) && pos.column() > 0 ) {
			isOpenIt = std::find( open.begin(), open.end(), line[pos.column() - 1] );
			isCloseIt = std::find( close.begin(), close.end(), line[pos.column() - 1] );
			if ( isOpenIt != open.end() ) {
				pos.setColumn( pos.column() - 1 );
			} else if ( isCloseIt != close.end() ) {
				pos.setColumn( pos.column() - 1 );
			}
		}
		if ( isOpenIt != open.end() ) {
			size_t index = std::distance( open.begin(), isOpenIt );
			String::StringBaseType openBracket = open[index];
			String::StringBaseType closeBracket = close[index];
			TextPosition closePosition = mDoc.findCloseBracket( pos, openBracket, closeBracket );
			mMatchingBrackets = {pos, closePosition};
		} else if ( isCloseIt != close.end() ) {
			size_t index = std::distance( close.begin(), isCloseIt );
			String::StringBaseType openBracket = open[index];
			String::StringBaseType closeBracket = close[index];
			TextPosition closePosition = mDoc.findOpenBracket( pos, openBracket, closeBracket );
			mMatchingBrackets = {pos, closePosition};
		}
	}
}

Int64 UICodeEditor::getColFromXOffset( Int64 lineNumber, const Float& x ) const {
	if ( x <= 0 )
		return 0;
	TextPosition pos = mDoc.sanitizePosition( TextPosition( lineNumber, 0 ) );
	const String& line = mDoc.line( pos.line() ).getText();
	Int64 len = line.length();
	Float glyphWidth = getGlyphWidth();
	Float xOffset = 0;
	Float tabWidth = glyphWidth * mTabWidth;
	Float hTab = tabWidth * 0.5f;
	bool isTab;
	for ( int i = 0; i < len; i++ ) {
		isTab = ( line[i] == '\t' );
		if ( xOffset >= x ) {
			return xOffset - x > ( isTab ? hTab : glyphWidth * 0.5f ) ? eemax<Int64>( 0, i - 1 )
																	  : i;
		} else if ( isTab && ( xOffset + tabWidth > x ) ) {
			return x - xOffset > hTab ? eemin<Int64>( i + 1, line.size() - 1 ) : i;
		}
		xOffset += isTab ? tabWidth : glyphWidth;
	}
	return static_cast<Int64>( line.size() ) - 1;
}

Float UICodeEditor::getLineHeight() const {
	return mFont->getLineSpacing( getCharacterSize() );
}

Float UICodeEditor::getCharacterSize() const {
	return PixelDensity::dpToPx( mFontStyleConfig.getFontCharacterSize() );
}

Float UICodeEditor::getGlyphWidth() const {
	return mFont->getGlyph( ' ', getCharacterSize(), false ).advance;
}

void UICodeEditor::resetCursor() {
	mCursorVisible = true;
	mBlinkTimer.restart();
}

TextPosition UICodeEditor::moveToLineOffset( const TextPosition& position, int offset ) {
	auto& xo = mLastXOffset;
	if ( xo.position != position ) {
		xo.offset = getColXOffset( position );
	}
	xo.position.setLine( position.line() + offset );
	xo.position.setColumn( getColFromXOffset( position.line() + offset, xo.offset ) );
	return xo.position;
}

void UICodeEditor::moveToPreviousLine() {
	TextPosition position = mDoc.getSelection().start();
	if ( position.line() == 0 )
		return mDoc.moveToStartOfDoc();
	mDoc.moveTo( moveToLineOffset( position, -1 ) );
}

void UICodeEditor::moveToNextLine() {
	TextPosition position = mDoc.getSelection().start();
	if ( position.line() == (Int64)mDoc.linesCount() - 1 )
		return mDoc.moveToEndOfDoc();
	mDoc.moveTo( moveToLineOffset( position, 1 ) );
}

void UICodeEditor::selectToPreviousLine() {
	TextPosition position = mDoc.getSelection().start();
	if ( position.line() == 0 )
		return mDoc.moveToStartOfDoc();
	mDoc.selectTo( moveToLineOffset( position, -1 ) );
}

void UICodeEditor::selectToNextLine() {
	TextPosition position = mDoc.getSelection().start();
	if ( position.line() == (Int64)mDoc.linesCount() - 1 )
		return mDoc.moveToEndOfDoc();
	mDoc.selectTo( moveToLineOffset( position, 1 ) );
}

void UICodeEditor::moveScrollUp() {
	setScrollY( mScroll.y - getLineHeight() );
}

void UICodeEditor::moveScrollDown() {
	setScrollY( mScroll.y + getLineHeight() );
}

void UICodeEditor::indent() {
	UIEventDispatcher* eventDispatcher =
		static_cast<UIEventDispatcher*>( getUISceneNode()->getEventDispatcher() );
	if ( !eventDispatcher->justGainedFocus() ) {
		mDoc.indent();
	}
}

void UICodeEditor::unindent() {
	UIEventDispatcher* eventDispatcher =
		static_cast<UIEventDispatcher*>( getUISceneNode()->getEventDispatcher() );
	if ( !eventDispatcher->justGainedFocus() ) {
		mDoc.unindent();
	}
}

void UICodeEditor::copy() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
}

void UICodeEditor::cut() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
	mDoc.deleteSelection();
}

void UICodeEditor::paste() {
	mDoc.textInput( getUISceneNode()->getWindow()->getClipboard()->getText() );
}

void UICodeEditor::fontSizeGrow() {
	mFontStyleConfig.CharacterSize = eemin<Float>( 96, mFontStyleConfig.CharacterSize + 1 );
	invalidateDraw();
}

void UICodeEditor::fontSizeShrink() {
	mFontStyleConfig.CharacterSize = eemax<Float>( 4, mFontStyleConfig.CharacterSize - 1 );
	invalidateDraw();
}

void UICodeEditor::fontSizeReset() {
	setFontSize( mFontSize );
}

void UICodeEditor::drawMatchingBrackets( const Vector2f& startScroll, const Float& lineHeight ) {
	if ( mMatchingBrackets.isValid() ) {
		Primitives primitive;
		primitive.setForceDraw( false );
		primitive.setColor( Color( mMatchingBracketColor ).blendAlpha( mAlpha ) );
		auto drawBracket = [&]( const TextPosition& pos ) {
			primitive.drawRectangle( Rectf( Vector2f( startScroll.x + getXOffsetCol( pos ),
													  startScroll.y + pos.line() * lineHeight ),
											Sizef( getGlyphWidth(), lineHeight ) ) );
		};
		drawBracket( mMatchingBrackets.start() );
		drawBracket( mMatchingBrackets.end() );
		primitive.setForceDraw( true );
	}
}

void UICodeEditor::drawSelectionMatch( const std::pair<int, int>& lineRange,
									   const Vector2f& startScroll, const Float& lineHeight ) {
	if ( !mDoc.hasSelection() )
		return;
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( mSelectionMatchColor ).blendAlpha( mAlpha ) );

	TextRange selection = mDoc.getSelection( true );
	const String& selectionLine = mDoc.line( selection.start().line() ).getText();
	String text( selectionLine.substr( selection.start().column(),
									   selection.end().column() - selection.start().column() ) );

	for ( auto ln = lineRange.first; ln <= lineRange.second; ln++ ) {
		const String& line = mDoc.line( ln ).getText();
		size_t pos = 0;
		// Skip ridicously long lines.
		if ( line.size() > 300 )
			continue;
		do {
			pos = line.find( text, pos );
			if ( pos != String::InvalidPos ) {
				Rectf selRect;
				Int64 startCol = pos;
				Int64 endCol = pos + text.size();
				selRect.Top = startScroll.y + ln * lineHeight;
				selRect.Bottom = selRect.Top + lineHeight;
				selRect.Left = startScroll.x + getXOffsetCol( {ln, startCol} );
				selRect.Right = startScroll.x + getXOffsetCol( {ln, endCol} );
				primitives.drawRectangle( selRect );
				pos = endCol;
			} else {
				break;
			}
		} while ( true );
	}
	primitives.setForceDraw( true );
}

void UICodeEditor::drawLineText( const Int64& index, Vector2f position, const Float& fontSize ) {
	auto& tokens = mHighlighter.getLine( index );
	for ( auto& token : tokens ) {
		Float textWidth = getTextWidth( token.text );
		if ( position.x + textWidth >= mScreenPos.x &&
			 position.x <= mScreenPos.x + mSize.getWidth() ) {
			Text line( "", mFont, fontSize );
			Color color( mColorScheme.getSyntaxColor( token.type ), mAlpha );
			line.setStyleConfig( mFontStyleConfig );
			line.setString( token.text );
			line.setColor( color );
			line.draw( position.x, position.y );
		} else if ( position.x > mScreenPos.x + mSize.getWidth() ) {
			break;
		}
		position.x += textWidth;
	}
}

void UICodeEditor::drawSelection( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								  const Float& lineHeight ) {
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor(
		Color( mFontStyleConfig.getFontSelectionBackColor() ).blendAlpha( mAlpha ) );

	TextRange selection = mDoc.getSelection( true );

	int startLine = eemax<int>( lineRange.first, selection.start().line() );
	int endLine = eemin<int>( lineRange.second, selection.end().line() );

	for ( auto ln = startLine; ln <= endLine; ln++ ) {
		const String& line = mDoc.line( ln ).getText();
		Rectf selRect;
		selRect.Top = startScroll.y + ln * lineHeight;
		selRect.Bottom = selRect.Top + lineHeight;
		if ( selection.start().line() == ln ) {
			selRect.Left = startScroll.x + getXOffsetCol( {ln, selection.start().column()} );
			if ( selection.end().line() == ln ) {
				selRect.Right = startScroll.x + getXOffsetCol( {ln, selection.end().column()} );
			} else {
				selRect.Right =
					startScroll.x + getXOffsetCol( {ln, static_cast<Int64>( line.length() )} );
			}
		} else if ( selection.end().line() == ln ) {
			selRect.Left = startScroll.x + getXOffsetCol( {ln, 0} );
			selRect.Right = startScroll.x + getXOffsetCol( {ln, selection.end().column()} );
		} else {
			selRect.Left = startScroll.x + getXOffsetCol( {ln, 0} );
			selRect.Right =
				startScroll.x + getXOffsetCol( {ln, static_cast<Int64>( line.length() )} );
		}

		primitives.drawRectangle( selRect );
	}
	primitives.setForceDraw( true );
}

void UICodeEditor::drawLineNumbers( const std::pair<int, int>& lineRange,
									const Vector2f& startScroll, const Vector2f& screenStart,
									const Float& lineHeight, const Float& lineNumberWidth,
									const int& lineNumberDigits, const Float& fontSize ) {
	Primitives primitives;
	primitives.setColor( Color( mLineNumberBackgroundColor ).blendAlpha( mAlpha ) );
	primitives.drawRectangle( Rectf( screenStart, Sizef( lineNumberWidth, mSize.getHeight() ) ) );
	TextRange selection = mDoc.getSelection( true );
	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		Text line( String( String::toStr( i + 1 ) ).padLeft( lineNumberDigits, ' ' ), mFont,
				   fontSize );
		line.setStyleConfig( mFontStyleConfig );
		line.setColor( ( i >= selection.start().line() && i <= selection.end().line() )
						   ? mLineNumberActiveFontColor
						   : mLineNumberFontColor );
		line.draw( screenStart.x + mLineNumberPaddingLeft, startScroll.y + lineHeight * i );
	}
}

void UICodeEditor::drawIndentationGuide( const std::pair<int, int>& lineRange,
										 const Vector2f& startScroll, const Float& lineHeight ) {
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( mIndentationGuideColor ).blendAlpha( mAlpha ) );
	primitives.setLineWidth( 1 );
	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		Float charWidth = getGlyphWidth();
		Float tabWidth = getTextWidth( "\t" );
		Vector2f curPos( startScroll.x, startScroll.y + lineHeight * i );
		auto& tokens = mHighlighter.getLine( i );
		if ( !tokens.empty() ) {
			size_t c = 0;
			String::StringBaseType curChar;
			while ( c < tokens[0].text.size() &&
					( tokens[0].text[c] == '\t' || tokens[0].text[c] == ' ' ) ) {
				curChar = tokens[0].text[c];
				if ( curChar == '\t' ) {
					primitives.drawLine( {{eefloor( curPos.x + tabWidth * 0.25f ),
										   eeceil( curPos.y + lineHeight * 0.5f )},
										  {eefloor( curPos.x + tabWidth * 0.75f ),
										   eeceil( curPos.y + lineHeight * 0.5f )}} );
				} else {
					primitives.drawLine(
						{{eefloor( curPos.x + charWidth * 0.5f - PixelDensity::dpToPx( 1 ) ),
						  eeceil( curPos.y + lineHeight * 0.5f )},
						 {eefloor( curPos.x + charWidth * 0.5f + PixelDensity::dpToPx( 1 ) ),
						  eeceil( curPos.y + lineHeight * 0.5f )}} );
				}
				c++;
				curPos.x += curChar == ' ' ? charWidth : tabWidth;
			}
		}
	}
	primitives.setForceDraw( true );
}

void UICodeEditor::registerCommands() {
	mDoc.setCommand( "move-to-previous-line", [&] { moveToPreviousLine(); } );
	mDoc.setCommand( "move-to-next-line", [&] { moveToNextLine(); } );
	mDoc.setCommand( "select-to-previous-line", [&] { selectToPreviousLine(); } );
	mDoc.setCommand( "select-to-next-line", [&] { selectToNextLine(); } );
	mDoc.setCommand( "move-scroll-up", [&] { moveScrollUp(); } );
	mDoc.setCommand( "move-scroll-down", [&] { moveScrollDown(); } );
	mDoc.setCommand( "indent", [&] { indent(); } );
	mDoc.setCommand( "unindent", [&] { unindent(); } );
	mDoc.setCommand( "copy", [&] { copy(); } );
	mDoc.setCommand( "cut", [&] { cut(); } );
	mDoc.setCommand( "paste", [&] { paste(); } );
	mDoc.setCommand( "font-size-grow", [&] { fontSizeGrow(); } );
	mDoc.setCommand( "font-size-shrink", [&] { fontSizeShrink(); } );
	mDoc.setCommand( "font-size-reset", [&] { fontSizeReset(); } );
	mDoc.setCommand( "lock", [&] { setLocked( true ); } );
	mDoc.setCommand( "unlock", [&] { setLocked( false ); } );
	mDoc.setCommand( "lock-toggle", [&] { setLocked( !isLocked() ); } );
	mUnlockedCmd.insert( {"copy", "select-all"} );
}

void UICodeEditor::registerKeybindings() {
	mKeyBindings.addKeybinds( {
		{{KEY_BACKSPACE, KEYMOD_CTRL}, "delete-to-previous-word"},
		{{KEY_BACKSPACE, KEYMOD_SHIFT}, "delete-to-previous-char"},
		{{KEY_BACKSPACE, 0}, "delete-to-previous-char"},
		{{KEY_DELETE, KEYMOD_CTRL}, "delete-to-next-word"},
		{{KEY_DELETE, 0}, "delete-to-next-char"},
		{{KEY_KP_ENTER, KEYMOD_CTRL | KEYMOD_SHIFT}, "new-line-above"},
		{{KEY_RETURN, KEYMOD_CTRL | KEYMOD_SHIFT}, "new-line-above"},
		{{KEY_KP_ENTER, 0}, "new-line"},
		{{KEY_RETURN, 0}, "new-line"},
		{{KEY_UP, KEYMOD_CTRL | KEYMOD_SHIFT}, "move-lines-up"},
		{{KEY_UP, KEYMOD_CTRL}, "move-scroll-up"},
		{{KEY_UP, KEYMOD_SHIFT}, "select-to-previous-line"},
		{{KEY_UP, 0}, "move-to-previous-line"},
		{{KEY_DOWN, KEYMOD_CTRL | KEYMOD_SHIFT}, "move-lines-down"},
		{{KEY_DOWN, KEYMOD_CTRL}, "move-scroll-down"},
		{{KEY_DOWN, KEYMOD_SHIFT}, "select-to-next-line"},
		{{KEY_DOWN, 0}, "move-to-next-line"},
		{{KEY_LEFT, KEYMOD_CTRL | KEYMOD_SHIFT}, "select-to-previous-word"},
		{{KEY_LEFT, KEYMOD_CTRL}, "move-to-previous-word"},
		{{KEY_LEFT, KEYMOD_SHIFT}, "select-to-previous-char"},
		{{KEY_LEFT, 0}, "move-to-previous-char"},
		{{KEY_RIGHT, KEYMOD_CTRL | KEYMOD_SHIFT}, "select-to-next-word"},
		{{KEY_RIGHT, KEYMOD_CTRL}, "move-to-next-word"},
		{{KEY_RIGHT, KEYMOD_SHIFT}, "select-to-next-char"},
		{{KEY_RIGHT, 0}, "move-to-next-char"},
		{{KEY_Z, KEYMOD_CTRL | KEYMOD_SHIFT}, "redo"},
		{{KEY_HOME, KEYMOD_CTRL | KEYMOD_SHIFT}, "select-to-start-of-doc"},
		{{KEY_HOME, KEYMOD_SHIFT}, "select-to-start-of-content"},
		{{KEY_HOME, KEYMOD_CTRL}, "move-to-start-of-doc"},
		{{KEY_HOME, 0}, "move-to-start-of-content"},
		{{KEY_END, KEYMOD_CTRL | KEYMOD_SHIFT}, "select-to-end-of-doc"},
		{{KEY_END, KEYMOD_SHIFT}, "select-to-end-of-line"},
		{{KEY_END, KEYMOD_CTRL}, "move-to-end-of-doc"},
		{{KEY_END, 0}, "move-to-end-of-line"},
		{{KEY_PAGEUP, KEYMOD_SHIFT}, "select-to-previous-page"},
		{{KEY_PAGEUP, 0}, "move-to-previous-page"},
		{{KEY_PAGEDOWN, KEYMOD_SHIFT}, "select-to-next-page"},
		{{KEY_PAGEDOWN, 0}, "move-to-next-page"},
		{{KEY_Y, KEYMOD_CTRL}, "redo"},
		{{KEY_Z, KEYMOD_CTRL}, "undo"},
		{{KEY_TAB, KEYMOD_SHIFT}, "unindent"},
		{{KEY_TAB, 0}, "indent"},
		{{KEY_C, KEYMOD_CTRL}, "copy"},
		{{KEY_X, KEYMOD_CTRL}, "cut"},
		{{KEY_V, KEYMOD_CTRL}, "paste"},
		{{KEY_A, KEYMOD_CTRL}, "select-all"},
		{{KEY_PLUS, KEYMOD_CTRL}, "font-size-grow"},
		{{KEY_KP_PLUS, KEYMOD_CTRL}, "font-size-grow"},
		{{KEY_MINUS, KEYMOD_CTRL}, "font-size-shrink"},
		{{KEY_KP_MINUS, KEYMOD_CTRL}, "font-size-shrink"},
		{{KEY_0, KEYMOD_CTRL}, "font-size-reset"},
	} );
}

}} // namespace EE::UI
