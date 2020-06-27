#include <algorithm>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/tools/uicolorpicker.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

using namespace EE::UI::Tools;

namespace EE { namespace UI {

UICodeEditor* UICodeEditor::New() {
	return eeNew( UICodeEditor, ( true, true ) );
}

UICodeEditor* UICodeEditor::NewOpt( const bool& autoRegisterBaseCommands,
									const bool& autoRegisterBaseKeybindings ) {
	return eeNew( UICodeEditor, ( autoRegisterBaseCommands, autoRegisterBaseKeybindings ) );
}

const std::map<KeyBindings::Shortcut, std::string> UICodeEditor::getDefaultKeybindings() {
	return {
		{{KEY_BACKSPACE, KEYMOD_CTRL}, "delete-to-previous-word"},
		{{KEY_BACKSPACE, KEYMOD_SHIFT}, "delete-to-previous-char"},
		{{KEY_BACKSPACE, 0}, "delete-to-previous-char"},
		{{KEY_DELETE, KEYMOD_CTRL}, "delete-to-next-word"},
		{{KEY_DELETE, 0}, "delete-to-next-char"},
		{{KEY_KP_ENTER, KEYMOD_CTRL | KEYMOD_SHIFT}, "new-line-above"},
		{{KEY_RETURN, KEYMOD_CTRL | KEYMOD_SHIFT}, "new-line-above"},
		{{KEY_KP_ENTER, KEYMOD_CTRL}, "new-line"},
		{{KEY_RETURN, KEYMOD_CTRL}, "new-line"},
		{{KEY_KP_ENTER, KEYMOD_SHIFT}, "new-line"},
		{{KEY_RETURN, KEYMOD_SHIFT}, "new-line"},
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
		{{KEY_PAGEUP, KEYMOD_CTRL}, "move-to-previous-page"},
		{{KEY_PAGEUP, KEYMOD_SHIFT}, "select-to-previous-page"},
		{{KEY_PAGEUP, 0}, "move-to-previous-page"},
		{{KEY_PAGEDOWN, KEYMOD_CTRL}, "move-to-next-page"},
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
		{{KEY_KP_DIVIDE, KEYMOD_CTRL}, "toggle-line-comments"},
	};
}

UICodeEditor::UICodeEditor( const std::string& elementTag, const bool& autoRegisterBaseCommands,
							const bool& autoRegisterBaseKeybindings ) :
	UIWidget( elementTag ),
	mFont( FontManager::instance()->getByName( "monospace" ) ),
	mDoc( std::make_shared<TextDocument>() ),
	mDirtyEditor( false ),
	mCursorVisible( false ),
	mMouseDown( false ),
	mShowLineNumber( true ),
	mShowWhitespaces( true ),
	mLocked( false ),
	mHighlightCurrentLine( true ),
	mHighlightMatchingBracket( true ),
	mHighlightSelectionMatch( true ),
	mEnableColorPickerOnSelection( false ),
	mHorizontalScrollBarEnabled( false ),
	mLongestLineWidthDirty( true ),
	mTabWidth( 4 ),
	mMouseWheelScroll( 50 ),
	mFontSize( mFontStyleConfig.getFontCharacterSize() ),
	mLineNumberPaddingLeft( 8 ),
	mLineNumberPaddingRight( 8 ),
	mHighlighter( mDoc.get() ),
	mKeyBindings( getUISceneNode()->getWindow()->getInput() ),
	mFindLongestLineWidthUpdateFrequency( Seconds( 1 ) ) {
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

	mHScrollBar = UIScrollBar::NewHorizontal();
	mHScrollBar->setParent( this );
	mHScrollBar->addEventListener( Event::OnSizeChange,
								   [&]( const Event* ) { updateScrollBar(); } );
	mHScrollBar->addEventListener( Event::OnValueChange, [&]( const Event* ) {
		setScrollX( mHScrollBar->getValue() * getMaxScroll().x, false );
	} );

	if ( NULL == mFont && elementTag == "codeeditor" )
		eePRINTL( "A monospace font must be loaded to be able to use the code editor.\nTry loading "
				  "a font with the name \"monospace\"" );

	mFontStyleConfig.Font = mFont;

	setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );

	clipEnable();
	mDoc->registerClient( this );
	subscribeScheduledUpdate();

	if ( autoRegisterBaseCommands )
		registerCommands();
	if ( autoRegisterBaseKeybindings )
		registerKeybindings();
}

UICodeEditor::UICodeEditor( const bool& autoRegisterBaseCommands,
							const bool& autoRegisterBaseKeybindings ) :
	UICodeEditor( "codeeditor", autoRegisterBaseCommands, autoRegisterBaseKeybindings ) {}

UICodeEditor::~UICodeEditor() {
	mDoc->unregisterClient( this );
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
	TextPosition cursor( mDoc->getSelection().start() );

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

	if ( mDoc->hasSelection() ) {
		drawTextRange( mDoc->getSelection( true ), lineRange, startScroll, lineHeight,
					   mFontStyleConfig.getFontSelectionBackColor() );
	}

	if ( mHighlightTextRange.isValid() && mHighlightTextRange.hasSelection() ) {
		drawTextRange( mHighlightTextRange, lineRange, startScroll, lineHeight,
					   mFontStyleConfig.getFontSelectionBackColor() );
	}

	if ( mHighlightSelectionMatch && mDoc->hasSelection() && mDoc->getSelection().inSameLine() ) {
		drawSelectionMatch( lineRange, startScroll, lineHeight );
	}

	if ( !mHighlightWord.empty() ) {
		drawWordMatch( mHighlightWord, lineRange, startScroll, lineHeight );
	}

	// Draw tab marker
	if ( mShowWhitespaces ) {
		drawWhitespaces( lineRange, startScroll, lineHeight );
	}

	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		drawLineText( i, {startScroll.x, startScroll.y + lineHeight * i}, charSize, lineHeight );
	}

	drawCursor( startScroll, lineHeight, cursor );

	if ( mShowLineNumber ) {
		drawLineNumbers( lineRange, startScroll, screenStart, lineHeight, lineNumberWidth,
						 lineNumberDigits, charSize );
	}
}

void UICodeEditor::scheduledUpdate( const Time& ) {
	if ( hasFocus() && getUISceneNode()->getWindow()->hasFocus() ) {
		if ( mBlinkTimer.getElapsedTime().asSeconds() > 0.5f ) {
			mCursorVisible = !mCursorVisible;
			mBlinkTimer.restart();
			invalidateDraw();
		}
	}

	if ( mMouseDown ) {
		if ( !( getUISceneNode()->getWindow()->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
			mMouseDown = false;
			getUISceneNode()->getWindow()->getInput()->captureMouse( false );
		} else if ( !isMouseOverMeOrChilds() ) {
			onMouseMove( getUISceneNode()->getEventDispatcher()->getMousePos(),
						 getUISceneNode()->getEventDispatcher()->getPressTrigger() );
		}
	}

	if ( mHighlighter.updateDirty( getVisibleLinesCount() ) ) {
		invalidateDraw();
	}

	if ( mHorizontalScrollBarEnabled && hasFocus() && mLongestLineWidthDirty &&
		 mLongestLineWidthLastUpdate.getElapsedTime() > mFindLongestLineWidthUpdateFrequency ) {
		updateLongestLineWidth();
	}
}

void UICodeEditor::updateLongestLineWidth() {
	if ( mHorizontalScrollBarEnabled ) {
		Float maxWidth = mLongestLineWidth;
		findLongestLine();
		if ( maxWidth != mLongestLineWidth )
			updateScrollBar();
		mLongestLineWidthLastUpdate.restart();
		mLongestLineWidthDirty = false;
	}
}

void UICodeEditor::reset() {
	mDoc->reset();
	mHighlighter.reset();
	invalidateDraw();
}

bool UICodeEditor::loadFromFile( const std::string& path ) {
	bool ret = mDoc->loadFromFile( path );
	invalidateEditor();
	updateLongestLineWidth();
	mHighlighter.changeDoc( mDoc.get() );
	invalidateDraw();
	return ret;
}

bool UICodeEditor::save() {
	return mDoc->save();
}

bool UICodeEditor::save( const std::string& path ) {
	return mDoc->save( path );
}

bool UICodeEditor::save( IOStreamFile& stream ) {
	return mDoc->save( stream );
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
		mFontStyleConfig.Font = mFont;
		invalidateDraw();
		invalidateEditor();
		onFontChanged();
	}
	return this;
}

void UICodeEditor::onFontChanged() {}

void UICodeEditor::onDocumentChanged() {
	sendCommonEvent( Event::OnDocumentChanged );
}

Uint32 UICodeEditor::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::MouseDown )
		return 1;
	return UIWidget::onMessage( msg );
}

void UICodeEditor::disableEditorFeatures() {
	mShowLineNumber = false;
	mShowWhitespaces = false;
	mHighlightCurrentLine = false;
	mHighlightMatchingBracket = false;
	mHighlightSelectionMatch = false;
	mEnableColorPickerOnSelection = false;
	mLineBreakingColumn = 0;
}

Float UICodeEditor::getViewportWidth( const bool& forceVScroll ) const {
	Float vScrollWidth =
		mVScrollBar->isVisible() || forceVScroll ? mVScrollBar->getPixelsSize().getWidth() : 0.f;
	Float viewWidth = eefloor( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right -
							   getLineNumberWidth() - vScrollWidth );
	return viewWidth;
}

UICodeEditor* UICodeEditor::setFontSize( const Float& dpSize ) {
	if ( mFontStyleConfig.CharacterSize != dpSize ) {
		mFontStyleConfig.CharacterSize =
			eeabs( dpSize - (int)dpSize ) == 0.5f || (int)dpSize == dpSize ? dpSize
																		   : eefloor( dpSize );
		mFontSize = mFontStyleConfig.CharacterSize;
		invalidateDraw();
		onFontChanged();
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
	return eemax<size_t>( 2UL, Math::countDigits( mDoc->linesCount() ) );
}

Float UICodeEditor::getLineNumberWidth() const {
	return mShowLineNumber ? eeceil( getLineNumberDigits() * getGlyphWidth() +
									 getLineNumberPaddingLeft() + getLineNumberPaddingRight() )
						   : 0.f;
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

const Color& UICodeEditor::getWhitespaceColor() const {
	return mWhitespaceColor;
}

void UICodeEditor::setWhitespaceColor( const Color& color ) {
	if ( mWhitespaceColor != color ) {
		mWhitespaceColor = color;
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
	mWhitespaceColor = mColorScheme.getEditorColor( "guide" );
	mLineBreakColumnColor = mColorScheme.getEditorColor( "line_break_column" );
	mMatchingBracketColor = mColorScheme.getEditorColor( "matching_bracket" );
	mSelectionMatchColor = mColorScheme.getEditorColor( "matching_selection" );
}

void UICodeEditor::setColorScheme( const SyntaxColorScheme& colorScheme ) {
	mColorScheme = colorScheme;
	updateColorScheme();
	invalidateDraw();
}

std::shared_ptr<Doc::TextDocument> UICodeEditor::getDocumentRef() const {
	return mDoc;
}

const TextDocument& UICodeEditor::getDocument() const {
	return *mDoc.get();
}

TextDocument& UICodeEditor::getDocument() {
	return *mDoc.get();
}

void UICodeEditor::setDocument( std::shared_ptr<TextDocument> doc ) {
	if ( mDoc.get() != doc.get() ) {
		mDoc->unregisterClient( this );
		mDoc = doc;
		mDoc->registerClient( this );
		mHighlighter.changeDoc( mDoc.get() );
		invalidateEditor();
		invalidateDraw();
		onDocumentChanged();
	}
}

bool UICodeEditor::isDirty() const {
	return mDoc->isDirty();
}

void UICodeEditor::invalidateEditor() {
	mDirtyEditor = true;
}

void UICodeEditor::invalidateLongestLineWidth() {
	mLongestLineWidthDirty = true;
	mLongestLineWidthLastUpdate.restart();
}

Uint32 UICodeEditor::onFocus() {
	if ( !mLocked ) {
		getUISceneNode()->getWindow()->startTextInput();
		resetCursor();
		mDoc->setActiveClient( this );
	}
	return UIWidget::onFocus();
}

Uint32 UICodeEditor::onFocusLoss() {
	mMouseDown = false;
	mCursorVisible = false;
	getSceneNode()->getWindow()->stopTextInput();
	if ( mDoc->getActiveClient() == this )
		mDoc->setActiveClient( nullptr );
	return UIWidget::onFocusLoss();
}

Uint32 UICodeEditor::onTextInput( const TextInputEvent& event ) {
	if ( mLocked || NULL == mFont )
		return 1;
	Input* input = getUISceneNode()->getWindow()->getInput();

	if ( ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' ) ||
		 ( input->isLeftAltPressed() && input->isShiftPressed() ) || input->isControlPressed() )
		return 1;

	mDoc->textInput( event.getText() );
	return 1;
}

Uint32 UICodeEditor::onKeyDown( const KeyEvent& event ) {
	if ( NULL == mFont )
		return 1;
	std::string cmd = mKeyBindings.getCommandFromKeyBind( {event.getKeyCode(), event.getMod()} );
	if ( !cmd.empty() ) {
		// Allow copy selection on locked mode
		if ( !mLocked || mUnlockedCmd.find( cmd ) != mUnlockedCmd.end() ) {
			mDoc->execute( cmd );
			return 0;
		}
	}
	return 1;
}

TextPosition UICodeEditor::resolveScreenPosition( const Vector2f& position ) const {
	Vector2f localPos( position );
	worldToNode( localPos );
	localPos = PixelDensity::dpToPx( localPos );
	localPos += mScroll;
	localPos.x -= mRealPadding.Left + ( mShowLineNumber ? getLineNumberWidth() : 0.f );
	localPos.y -= mRealPadding.Top;
	Int64 line = eeclamp<Int64>( (Int64)eefloor( localPos.y / getLineHeight() ), 0,
								 ( Int64 )( mDoc->linesCount() - 1 ) );
	return TextPosition( line, getColFromXOffset( line, localPos.x ) );
}

Vector2f UICodeEditor::getViewPortLineCount() const {
	return Vector2f(
		eefloor( getViewportWidth() / getGlyphWidth() ),
		eefloor( ( mSize.getHeight() - mRealPadding.Top -
				   ( mHScrollBar->isVisible() ? mHScrollBar->getPixelsSize().getHeight() : 0.f ) ) /
				 getLineHeight() ) );
}

Sizef UICodeEditor::getMaxScroll() const {
	Vector2f vplc( getViewPortLineCount() );
	return Sizef( eemax( 0.f, mLongestLineWidth - getViewportWidth() ),
				  vplc.y > mDoc->linesCount() - 1
					  ? 0.f
					  : eefloor( mDoc->linesCount() - getViewPortLineCount().y ) *
							getLineHeight() );
}

Uint32 UICodeEditor::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( isTextSelectionEnabled() && !getEventDispatcher()->isNodeDragging() && NULL != mFont &&
		 !mMouseDown && getEventDispatcher()->getMouseDownNode() == this &&
		 ( flags & EE_BUTTON_LMASK ) ) {
		mMouseDown = true;
		Input* input = getUISceneNode()->getWindow()->getInput();
		input->captureMouse( true );
		setFocus();
		if ( input->isShiftPressed() ) {
			mDoc->selectTo( resolveScreenPosition( position.asFloat() ) );
		} else {
			mDoc->setSelection( resolveScreenPosition( position.asFloat() ) );
		}
	}
	return UIWidget::onMouseDown( position, flags );
}

Uint32 UICodeEditor::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	if ( isTextSelectionEnabled() && !getUISceneNode()->getEventDispatcher()->isNodeDragging() &&
		 NULL != mFont && mMouseDown && ( flags & EE_BUTTON_LMASK ) ) {
		TextRange selection = mDoc->getSelection();
		selection.setStart( resolveScreenPosition( position.asFloat() ) );
		mDoc->setSelection( selection );
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
			mDoc->execute( "font-size-shrink" );
		} else {
			setScrollY( mScroll.y + PixelDensity::dpToPx( mMouseWheelScroll ) );
		}
		invalidateDraw();
	} else if ( flags & EE_BUTTON_WUMASK ) {
		if ( getUISceneNode()->getWindow()->getInput()->isControlPressed() ) {
			mDoc->execute( "font-size-grow" );
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
		mDoc->selectLine();
	}
	return 1;
}

Uint32 UICodeEditor::onMouseDoubleClick( const Vector2i&, const Uint32& flags ) {
	if ( mLocked || NULL == mFont )
		return 1;

	if ( flags & EE_BUTTON_LMASK ) {
		mDoc->selectWord();
		mLastDoubleClick.restart();
		checkColorPickerAction();
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

void UICodeEditor::checkColorPickerAction() {
	if ( !mEnableColorPickerOnSelection )
		return;
	String text( mDoc->getSelectedText() );
	TextRange range( mDoc->getSelection( true ) );
	if ( range.start().line() != range.end().line() )
		return;
	const String& line = mDoc->line( range.end().line() ).getText();
	bool isHash = range.start().column() > 0 &&
				  mDoc->line( range.start().line() ).getText()[range.start().column() - 1] == '#' &&
				  ( text.size() == 6 || text.size() == 8 ) && String::isHexNotation( text );
	bool isRgba = !isHash && text == "rgba" && range.end().column() < (Int64)line.size() - 1 &&
				  line[range.end().column()] == '(';
	bool isRgb = !isHash && !isRgba && text == "rgb" &&
				 range.end().column() < (Int64)line.size() - 1 && line[range.end().column()] == '(';
	if ( isHash || isRgb || isRgba ) {
		UIColorPicker* colorPicker = NULL;
		if ( isHash ) {
			colorPicker = UIColorPicker::NewModal( this, [&]( Color color ) {
				mDoc->replaceSelection( color.toHexString( false ) );
			} );
			colorPicker->setColor( Color( '#' + text ) );
		} else if ( isRgba || isRgb ) {
			TextPosition position = mDoc->findCloseBracket(
				{range.start().line(), static_cast<Int64>( range.end().column() )}, '(', ')' );
			if ( position.isValid() ) {
				mDoc->setSelection( {position.line(), position.column() + 1}, range.start() );
				colorPicker = UIColorPicker::NewModal( this, [&, isRgba]( Color color ) {
					mDoc->replaceSelection( isRgba || color.a != 255 ? color.toRgbaString()
																	 : color.toRgbString() );
				} );
				colorPicker->setColor( Color::fromString( mDoc->getSelectedText() ) );
			}
		}
		if ( colorPicker )
			colorPicker->getUIWindow()->addEventListener(
				Event::OnWindowClose, [&]( const Event* ) {
					if ( !SceneManager::instance()->isShootingDown() )
						setFocus();
				} );
	}
}

void UICodeEditor::drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							   const TextPosition& cursor ) {
	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight );
		Primitives primitives;
		primitives.setColor( Color( mCaretColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), lineHeight ) ) );
	}
}

void UICodeEditor::onSizeChange() {
	UIWidget::onSizeChange();
	invalidateEditor();
}

void UICodeEditor::onPaddingChange() {
	UIWidget::onPaddingChange();
	invalidateEditor();
}

void UICodeEditor::findLongestLine() {
	if ( mHorizontalScrollBarEnabled ) {
		mLongestLineWidth = 0;
		for ( size_t lineIndex = 0; lineIndex < mDoc->linesCount(); lineIndex++ ) {
			mLongestLineWidth = eemax( mLongestLineWidth, getLineWidth( lineIndex ) );
		}
	}
}

Float UICodeEditor::getLineWidth( const Int64& lineIndex ) {
	return getTextWidth( mDoc->line( lineIndex ).getText() );
}

void UICodeEditor::updateScrollBar() {
	int notVisibleLineCount = (int)mDoc->linesCount() - (int)getViewPortLineCount().y;

	if ( mLongestLineWidthDirty ) {
		updateLongestLineWidth();
	}

	mHScrollBar->setEnabled( false );
	mHScrollBar->setVisible( false );

	mVScrollBar->setPixelsSize( mVScrollBar->getPixelsSize().getWidth(), mSize.getHeight() );

	if ( mHorizontalScrollBarEnabled ) {
		mHScrollBar->setPixelsPosition( 0, mSize.getHeight() -
											   mHScrollBar->getPixelsSize().getHeight() );
		mHScrollBar->setPixelsSize(
			mSize.getWidth() -
				( notVisibleLineCount > 0 ? mVScrollBar->getPixelsSize().getWidth() : 0 ),
			mHScrollBar->getPixelsSize().getHeight() );
		Float viewPortWidth = getViewportWidth();
		mHScrollBar->setPageStep( viewPortWidth / mLongestLineWidth );
		mHScrollBar->setClickStep( 0.2f );
		mHScrollBar->setEnabled( mLongestLineWidth > viewPortWidth );
		mHScrollBar->setVisible( mLongestLineWidth > viewPortWidth );
	}

	mVScrollBar->setPixelsPosition( mSize.getWidth() - mVScrollBar->getPixelsSize().getWidth(), 0 );
	mVScrollBar->setPageStep( getViewPortLineCount().y / (float)mDoc->linesCount() );
	mVScrollBar->setClickStep( 0.2f );
	mVScrollBar->setEnabled( notVisibleLineCount > 0 );
	mVScrollBar->setVisible( notVisibleLineCount > 0 );
	setScrollY( mScroll.y );
}

void UICodeEditor::updateEditor() {
	mDoc->setPageSize( getVisibleLinesCount() );
	if ( mDoc->getActiveClient() == this )
		scrollToMakeVisible( mDoc->getSelection().start() );
	updateScrollBar();
	mDirtyEditor = false;
}

void UICodeEditor::onDocumentTextChanged() {
	invalidateDraw();
	checkMatchingBrackets();
	sendCommonEvent( Event::OnTextChanged );
	invalidateLongestLineWidth();
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

void UICodeEditor::onDocumentUndoRedo( const TextDocument::UndoRedo& ) {
	onDocumentSelectionChange( {} );
}

void UICodeEditor::onDocumentSaved() {
	sendCommonEvent( Event::OnSave );
}

std::pair<int, int> UICodeEditor::getVisibleLineRange() {
	Float lineHeight = getLineHeight();
	Float minLine = eemax( 0.f, eefloor( mScroll.y / lineHeight ) );
	Float maxLine = eemin( mDoc->linesCount() - 1.f,
						   eefloor( ( mSize.getHeight() + mScroll.y ) / lineHeight ) + 1 );
	return std::make_pair<int, int>( (int)minLine, (int)maxLine );
}

int UICodeEditor::getVisibleLinesCount() {
	auto lines = getVisibleLineRange();
	return lines.second - lines.first;
}

void UICodeEditor::scrollToMakeVisible( const TextPosition& position ) {
	auto lineRange = getVisibleLineRange();

	Int64 minDistance = mHScrollBar->isVisible() ? 3 : 2;

	if ( position.line() <= lineRange.first || position.line() >= lineRange.second - minDistance ) {
		// Vertical Scroll
		Float lineHeight = getLineHeight();
		Float min = eefloor( lineHeight * ( eemax<Float>( 0, position.line() - 1 ) ) );
		Float max = eefloor( lineHeight * ( position.line() + minDistance ) - mSize.getHeight() );
		if ( min < mScroll.y )
			setScrollY( min );
		else if ( max > mScroll.y )
			setScrollY( max );
	}

	// Horizontal Scroll
	Float offsetX = getXOffsetCol( position );
	Float glyphSize = getGlyphWidth();
	Float minVisibility = glyphSize;
	Float viewPortWidth = getViewportWidth();
	if ( offsetX + minVisibility > mScroll.x + viewPortWidth ) {
		setScrollX( eefloor( eemax( 0.f, offsetX + minVisibility - viewPortWidth ) ) );
	} else if ( offsetX < mScroll.x ) {
		setScrollX( eefloor( eemax( 0.f, offsetX - minVisibility ) ) );
	}
}

void UICodeEditor::setScrollX( const Float& val, bool emmitEvent ) {
	Float oldVal = mScroll.x;
	mScroll.x = eefloor( eemax( val, 0.f ) );
	if ( oldVal != mScroll.x ) {
		invalidateDraw();
		if ( mHorizontalScrollBarEnabled && emmitEvent )
			mHScrollBar->setValue( mScroll.x / getMaxScroll().x, false );
	}
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

Float UICodeEditor::getXOffsetCol( const TextPosition& position ) {
	const String& line = mDoc->line( position.line() ).getText();
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
	position.setLine( eeclamp<Int64>( position.line(), 0L, mDoc->linesCount() - 1 ) );
	// This is different from sanitizePosition, sinze allows the last character.
	position.setColumn( eeclamp<Int64>( position.column(), 0L,
										eemax<Int64>( 0, mDoc->line( position.line() ).size() ) ) );
	return getTextWidth( mDoc->line( position.line() ).substr( 0, position.column() ) );
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

void UICodeEditor::addKeyBindsString( const std::map<std::string, std::string>& binds,
									  const bool& allowLocked ) {
	mKeyBindings.addKeybindsString( binds );
	for ( auto bind : binds ) {
		if ( allowLocked ) {
			mUnlockedCmd.insert( bind.second );
		}
	}
}

void UICodeEditor::addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds,
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

void UICodeEditor::addUnlockedCommands( const std::vector<std::string>& commands ) {
	mUnlockedCmd.insert( commands.begin(), commands.end() );
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
			return String::toString( PixelDensity::dpToPx( getOutlineThickness() ) );
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

const bool& UICodeEditor::getEnableColorPickerOnSelection() const {
	return mEnableColorPickerOnSelection;
}

void UICodeEditor::setEnableColorPickerOnSelection( const bool& enableColorPickerOnSelection ) {
	mEnableColorPickerOnSelection = enableColorPickerOnSelection;
}

void UICodeEditor::setSyntaxDefinition( const SyntaxDefinition& definition ) {
	mDoc->setSyntaxDefinition( definition );
	mHighlighter.reset();
	invalidateDraw();
}

const SyntaxDefinition& UICodeEditor::getSyntaxDefinition() const {
	return mDoc->getSyntaxDefinition();
}

void UICodeEditor::checkMatchingBrackets() {
	if ( mHighlightMatchingBracket ) {
		const std::vector<String::StringBaseType> open{'{', '(', '['};
		const std::vector<String::StringBaseType> close{'}', ')', ']'};
		mMatchingBrackets = TextRange();
		TextPosition pos = mDoc->getSelection().start();
		TextDocumentLine& line = mDoc->line( pos.line() );
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
			TextPosition closePosition = mDoc->findCloseBracket( pos, openBracket, closeBracket );
			mMatchingBrackets = {pos, closePosition};
		} else if ( isCloseIt != close.end() ) {
			size_t index = std::distance( close.begin(), isCloseIt );
			String::StringBaseType openBracket = open[index];
			String::StringBaseType closeBracket = close[index];
			TextPosition closePosition = mDoc->findOpenBracket( pos, openBracket, closeBracket );
			mMatchingBrackets = {pos, closePosition};
		}
	}
}

Int64 UICodeEditor::getColFromXOffset( Int64 lineNumber, const Float& x ) const {
	if ( x <= 0 )
		return 0;
	TextPosition pos = mDoc->sanitizePosition( TextPosition( lineNumber, 0 ) );
	const String& line = mDoc->line( pos.line() ).getText();
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
	TextPosition position = mDoc->getSelection().start();
	if ( position.line() == 0 )
		return mDoc->moveToStartOfDoc();
	mDoc->moveTo( moveToLineOffset( position, -1 ) );
}

void UICodeEditor::moveToNextLine() {
	TextPosition position = mDoc->getSelection().start();
	if ( position.line() == (Int64)mDoc->linesCount() - 1 )
		return mDoc->moveToEndOfDoc();
	mDoc->moveTo( moveToLineOffset( position, 1 ) );
}

void UICodeEditor::selectToPreviousLine() {
	TextPosition position = mDoc->getSelection().start();
	if ( position.line() == 0 )
		return mDoc->moveToStartOfDoc();
	mDoc->selectTo( moveToLineOffset( position, -1 ) );
}

void UICodeEditor::selectToNextLine() {
	TextPosition position = mDoc->getSelection().start();
	if ( position.line() == (Int64)mDoc->linesCount() - 1 )
		return mDoc->moveToEndOfDoc();
	mDoc->selectTo( moveToLineOffset( position, 1 ) );
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
		mDoc->indent();
	}
}

void UICodeEditor::unindent() {
	UIEventDispatcher* eventDispatcher =
		static_cast<UIEventDispatcher*>( getUISceneNode()->getEventDispatcher() );
	if ( !eventDispatcher->justGainedFocus() ) {
		mDoc->unindent();
	}
}

void UICodeEditor::copy() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc->getSelectedText() );
}

void UICodeEditor::cut() {
	getUISceneNode()->getWindow()->getClipboard()->setText( mDoc->getSelectedText() );
	mDoc->deleteSelection();
}

void UICodeEditor::paste() {
	mDoc->textInput( getUISceneNode()->getWindow()->getClipboard()->getText() );
}

void UICodeEditor::fontSizeGrow() {
	mFontStyleConfig.CharacterSize = eemin<Float>( 96, mFontStyleConfig.CharacterSize + 1 );
	onFontChanged();
	invalidateDraw();
}

void UICodeEditor::fontSizeShrink() {
	mFontStyleConfig.CharacterSize = eemax<Float>( 4, mFontStyleConfig.CharacterSize - 1 );
	onFontChanged();
	invalidateDraw();
}

void UICodeEditor::fontSizeReset() {
	setFontSize( mFontSize );
}

const bool& UICodeEditor::getShowWhitespaces() const {
	return mShowWhitespaces;
}

void UICodeEditor::setShowWhitespaces( const bool& showWhitespaces ) {
	if ( mShowWhitespaces != showWhitespaces ) {
		mShowWhitespaces = showWhitespaces;
		invalidateDraw();
	}
}

const String& UICodeEditor::getHighlightWord() const {
	return mHighlightWord;
}

void UICodeEditor::setHighlightWord( const String& highlightWord ) {
	if ( mHighlightWord != highlightWord ) {
		mHighlightWord = highlightWord;
		invalidateDraw();
	}
}

const TextRange& UICodeEditor::getHighlightTextRange() const {
	return mHighlightTextRange;
}

void UICodeEditor::setHighlightTextRange( const TextRange& highlightSelection ) {
	if ( highlightSelection != mHighlightTextRange ) {
		mHighlightTextRange = highlightSelection;
		invalidateDraw();
	}
}

const Time& UICodeEditor::getFindLongestLineWidthUpdateFrequency() const {
	return mFindLongestLineWidthUpdateFrequency;
}

void UICodeEditor::setFindLongestLineWidthUpdateFrequency(
	const Time& findLongestLineWidthUpdateFrequency ) {
	mFindLongestLineWidthUpdateFrequency = findLongestLineWidthUpdateFrequency;
}

const bool& UICodeEditor::getHorizontalScrollBarEnabled() const {
	return mHorizontalScrollBarEnabled;
}

void UICodeEditor::setHorizontalScrollBarEnabled( const bool& horizontalScrollBarEnabled ) {
	if ( horizontalScrollBarEnabled != mHorizontalScrollBarEnabled ) {
		mHorizontalScrollBarEnabled = horizontalScrollBarEnabled;
		invalidateLongestLineWidth();
	}
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
	if ( !mDoc->hasSelection() )
		return;
	TextRange selection = mDoc->getSelection( true );
	const String& selectionLine = mDoc->line( selection.start().line() ).getText();
	String text( selectionLine.substr( selection.start().column(),
									   selection.end().column() - selection.start().column() ) );
	drawWordMatch( text, lineRange, startScroll, lineHeight );
}

void UICodeEditor::drawWordMatch( const String& text, const std::pair<int, int>& lineRange,
								  const Vector2f& startScroll, const Float& lineHeight ) {
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( mSelectionMatchColor ).blendAlpha( mAlpha ) );

	for ( auto ln = lineRange.first; ln <= lineRange.second; ln++ ) {
		const String& line = mDoc->line( ln ).getText();
		size_t pos = 0;
		// Skip ridiculously long lines.
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

void UICodeEditor::drawLineText( const Int64& index, Vector2f position, const Float& fontSize,
								 const Float& lineHeight ) {
	auto& tokens = mHighlighter.getLine( index );
	Primitives primitives;
	for ( auto& token : tokens ) {
		Float textWidth = getTextWidth( token.text );
		if ( position.x + textWidth >= mScreenPos.x &&
			 position.x <= mScreenPos.x + mSize.getWidth() ) {
			Text line( "", mFont, fontSize );
			const SyntaxColorScheme::Style& style = mColorScheme.getSyntaxStyle( token.type );
			line.setStyleConfig( mFontStyleConfig );
			if ( style.style )
				line.setStyle( style.style );
			if ( style.background != Color::Transparent ) {
				primitives.setColor( Color( style.background ).blendAlpha( mAlpha ) );
				primitives.drawRectangle( Rectf( position, Sizef( textWidth, lineHeight ) ) );
			}
			line.setColor( Color( style.color ).blendAlpha( mAlpha ) );
			line.setString( token.text );
			line.draw( position.x, position.y );
		} else if ( position.x > mScreenPos.x + mSize.getWidth() ) {
			break;
		}
		position.x += textWidth;
	}
}

void UICodeEditor::drawTextRange( const TextRange& range, const std::pair<int, int>& lineRange,
								  const Vector2f& startScroll, const Float& lineHeight,
								  const Color& backgrundColor ) {
	Primitives primitives;
	primitives.setForceDraw( false );
	primitives.setColor( Color( backgrundColor ).blendAlpha( mAlpha ) );

	int startLine = eemax<int>( lineRange.first, range.start().line() );
	int endLine = eemin<int>( lineRange.second, range.end().line() );

	for ( auto ln = startLine; ln <= endLine; ln++ ) {
		const String& line = mDoc->line( ln ).getText();
		Rectf selRect;
		selRect.Top = startScroll.y + ln * lineHeight;
		selRect.Bottom = selRect.Top + lineHeight;
		if ( range.start().line() == ln ) {
			selRect.Left = startScroll.x + getXOffsetCol( {ln, range.start().column()} );
			if ( range.end().line() == ln ) {
				selRect.Right = startScroll.x + getXOffsetCol( {ln, range.end().column()} );
			} else {
				selRect.Right =
					startScroll.x + getXOffsetCol( {ln, static_cast<Int64>( line.length() )} );
			}
		} else if ( range.end().line() == ln ) {
			selRect.Left = startScroll.x + getXOffsetCol( {ln, 0} );
			selRect.Right = startScroll.x + getXOffsetCol( {ln, range.end().column()} );
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
	TextRange selection = mDoc->getSelection( true );
	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		Text line( String( String::toString( i + 1 ) ).padLeft( lineNumberDigits, ' ' ), mFont,
				   fontSize );
		line.setStyleConfig( mFontStyleConfig );
		line.setColor( ( i >= selection.start().line() && i <= selection.end().line() )
						   ? mLineNumberActiveFontColor
						   : mLineNumberFontColor );
		line.draw( screenStart.x + mLineNumberPaddingLeft, startScroll.y + lineHeight * i );
	}
}

void UICodeEditor::drawWhitespaces( const std::pair<int, int>& lineRange,
									const Vector2f& startScroll, const Float& lineHeight ) {
	Float tabWidth = getTextWidth( "\t" );
	Float glyphW = getGlyphWidth();
	Color color( Color( mWhitespaceColor ).blendAlpha( mAlpha ) );
	unsigned int fontSize = getCharacterSize();
	// We use the GlyphDrawable since can batch the draw calls instead of Text.
	GlyphDrawable* adv = mFont->getGlyphDrawable( 187 /*'»'*/, fontSize );
	GlyphDrawable* cpoint = mFont->getGlyphDrawable( 183 /*'·'*/, fontSize );
	Float tabCenter = ( tabWidth - adv->getPxSize().getWidth() ) * 0.5f;
	adv->setDrawMode( GlyphDrawable::DrawMode::Text );
	cpoint->setDrawMode( GlyphDrawable::DrawMode::Text );
	adv->setColor( color );
	cpoint->setColor( color );
	for ( int index = lineRange.first; index <= lineRange.second; index++ ) {
		Vector2f position( {startScroll.x, startScroll.y + lineHeight * index} );
		auto& tokens = mHighlighter.getLine( index );
		for ( auto& token : tokens ) {
			Float textWidth = getTextWidth( token.text );
			if ( position.x + textWidth >= mScreenPos.x &&
				 position.x <= mScreenPos.x + mSize.getWidth() ) {
				for ( size_t i = 0; i < token.text.size(); i++ ) {
					if ( ' ' == token.text[i] ) {
						cpoint->draw( Vector2f( position.x, position.y ) );
						position.x += glyphW;
					} else if ( '\t' == token.text[i] ) {
						adv->draw( Vector2f( position.x + tabCenter, position.y ) );
						position.x += tabWidth;
					} else {
						position.x += glyphW;
					}
				}
			} else if ( position.x > mScreenPos.x + mSize.getWidth() ) {
				break;
			} else {
				position.x += textWidth;
			}
		}
	}
}

void UICodeEditor::registerCommands() {
	mDoc->setCommand( "move-to-previous-line", [&] { moveToPreviousLine(); } );
	mDoc->setCommand( "move-to-next-line", [&] { moveToNextLine(); } );
	mDoc->setCommand( "select-to-previous-line", [&] { selectToPreviousLine(); } );
	mDoc->setCommand( "select-to-next-line", [&] { selectToNextLine(); } );
	mDoc->setCommand( "move-scroll-up", [&] { moveScrollUp(); } );
	mDoc->setCommand( "move-scroll-down", [&] { moveScrollDown(); } );
	mDoc->setCommand( "indent", [&] { indent(); } );
	mDoc->setCommand( "unindent", [&] { unindent(); } );
	mDoc->setCommand( "copy", [&] { copy(); } );
	mDoc->setCommand( "cut", [&] { cut(); } );
	mDoc->setCommand( "paste", [&] { paste(); } );
	mDoc->setCommand( "font-size-grow", [&] { fontSizeGrow(); } );
	mDoc->setCommand( "font-size-shrink", [&] { fontSizeShrink(); } );
	mDoc->setCommand( "font-size-reset", [&] { fontSizeReset(); } );
	mDoc->setCommand( "lock", [&] { setLocked( true ); } );
	mDoc->setCommand( "unlock", [&] { setLocked( false ); } );
	mDoc->setCommand( "lock-toggle", [&] { setLocked( !isLocked() ); } );
	mUnlockedCmd.insert( {"copy", "select-all"} );
}

void UICodeEditor::registerKeybindings() {
	mKeyBindings.addKeybinds( getDefaultKeybindings() );
}

}} // namespace EE::UI
