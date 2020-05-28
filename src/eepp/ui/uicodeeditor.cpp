#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
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
	mLocked( false ),
	mTabWidth( 4 ),
	mLastColOffset( 0 ),
	mMouseWheelScroll( 50 ),
	mFontSize( mFontStyleConfig.getFontCharacterSize() ),
	mLineNumberPaddingLeft( 8 ),
	mLineNumberPaddingRight( 8 ),
	mHighlighter( &mDoc ) {
	mFlags |= UI_TAB_STOP;
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

	clipEnable();
	mDoc.registerClient( *this );
	subscribeScheduledUpdate();
}

UICodeEditor::~UICodeEditor() {
	mDoc.unregisterClient( *this );
}

Uint32 UICodeEditor::getType() const {
	return UI_TYPE_CODEEDIT;
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

	std::pair<int, int> lineRange = getVisibleLineRange();
	Float charSize = PixelDensity::pxToDp( getCharacterSize() );
	Float lineHeight = getLineHeight();
	int lineNumberDigits = getLineNumberDigits();
	Float lineNumberWidth = mShowLineNumber ? getLineNumberWidth() : 0.f;
	Vector2f screenStart( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top );
	Vector2f start( screenStart.x + lineNumberWidth, screenStart.y );
	Vector2f startScroll( start - mScroll );
	Primitives primitives;
	TextPosition cursor( mDoc.getSelection().start() );

	if ( !mLocked ) {
		primitives.setColor( mCurrentLineBackgroundColor );
		primitives.drawRectangle( Rectf(
			Vector2f( startScroll.x + mScroll.x, startScroll.y + cursor.line() * lineHeight ),
			Sizef( mSize.getWidth(), lineHeight ) ) );
	}

	if ( mDoc.hasSelection() ) {
		primitives.setColor( mFontStyleConfig.getFontSelectionBackColor() );

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
	}

	for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
		Vector2f curPos( startScroll.x, startScroll.y + lineHeight * i );
		auto& tokens = mHighlighter.getLine( i );
		for ( auto& token : tokens ) {
			Float textWidth = getTextWidth( token.text );
			if ( curPos.x + textWidth >= mScreenPos.x &&
				 curPos.x <= mScreenPos.x + mSize.getWidth() ) {
				Text line( "", mFont, charSize );
				line.setStyleConfig( mFontStyleConfig );
				line.setString( token.text );
				line.setColor( mColorScheme.getSyntaxColor( token.type ) );
				line.draw( curPos.x, curPos.y );
			} else if ( curPos.x > mScreenPos.x + mSize.getWidth() ) {
				break;
			}
			curPos.x += textWidth;
		}
	}

	if ( mCursorVisible && !mLocked ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight );

		primitives.setColor( mCaretColor );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), lineHeight ) ) );
	}

	if ( mShowLineNumber ) {
		primitives.setColor( mLineNumberBackgroundColor );
		primitives.drawRectangle(
			Rectf( screenStart, Sizef( lineNumberWidth, mSize.getHeight() ) ) );
		TextRange selection = mDoc.getSelection( true );
		for ( int i = lineRange.first; i <= lineRange.second; i++ ) {
			Text line( String( String::toStr( i + 1 ) ).padLeft( lineNumberDigits, ' ' ), mFont,
					   charSize );
			line.setStyleConfig( mFontStyleConfig );
			line.setColor( ( i >= selection.start().line() && i <= selection.end().line() )
							   ? mLineNumberActiveFontColor
							   : mLineNumberFontColor );
			line.draw( screenStart.x + mLineNumberPaddingLeft, startScroll.y + lineHeight * i );
		}
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
	mDoc.loadFromPath( path );
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

UICodeEditor* UICodeEditor::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.getFontColor() != color ) {
		mFontStyleConfig.FontColor = color;
		invalidateDraw();
	}
	return this;
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
	return getLineNumberDigits() * getGlyphWidth() + getLineNumberPaddingLeft() +
		   getLineNumberPaddingRight();
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

	// Allow copy selection on locked mode
	if ( mLocked ) {
		if ( event.getKeyCode() == KEY_C && ( event.getMod() & KEYMOD_CTRL ) ) {
			getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
		}
		return 1;
	}

	switch ( event.getKeyCode() ) {
		case KEY_BACKSPACE: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.deleteToPreviousWord();
			} else if ( event.getMod() == 0 || ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.deleteToPreviousChar();
			}
			break;
		}
		case KEY_DELETE: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.deleteToNextWord();
			} else if ( event.getMod() == 0 || ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.deleteToNextChar();
			}
			break;
		}
		case KEY_KP_ENTER:
		case KEY_RETURN: {
			if ( ( event.getMod() & KEYMOD_CTRL ) && ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.newLineAbove();
			} else if ( !( event.getMod() & KEYMOD_ALT ) ) {
				mDoc.newLine();
			}
			break;
		}
		case KEY_UP: {
			if ( ( event.getMod() & KEYMOD_CTRL ) && ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.moveLinesUp();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				setScrollY( mScroll.y - getLineHeight() );
			} else if ( event.getMod() & KEYMOD_SHIFT ) {
				selectToPreviousLine();
			} else if ( event.getMod() == 0 ) {
				moveToPreviousLine();
			}
			break;
		}
		case KEY_DOWN: {
			if ( ( event.getMod() & KEYMOD_CTRL ) && ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.moveLinesDown();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				setScrollY( mScroll.y + getLineHeight() );
			} else if ( event.getMod() & KEYMOD_SHIFT ) {
				selectToNextLine();
			} else {
				moveToNextLine();
			}
			break;
		}
		case KEY_LEFT: {
			if ( ( event.getMod() & KEYMOD_SHIFT ) && ( event.getMod() & KEYMOD_CTRL ) ) {
				mDoc.selectToPreviousWord();
			} else if ( event.getMod() & KEYMOD_SHIFT ) {
				mDoc.selectToPreviousChar();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.moveToPreviousWord();
			} else {
				mDoc.moveToPreviousChar();
			}
			break;
		}
		case KEY_RIGHT: {
			if ( ( event.getMod() & KEYMOD_SHIFT ) && ( event.getMod() & KEYMOD_CTRL ) ) {
				mDoc.selectToNextWord();
			} else if ( event.getMod() & KEYMOD_SHIFT ) {
				mDoc.selectToNextChar();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.moveToNextWord();
			} else {
				mDoc.moveToNextChar();
			}
			break;
		}
		case KEY_HOME: {
			if ( ( event.getMod() & KEYMOD_CTRL ) && ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.selectToStartOfDoc();
			} else if ( event.getMod() & KEYMOD_SHIFT ) {
				mDoc.selectToStartOfContent();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.moveToStartOfDoc();
			} else if ( event.getMod() == 0 ) {
				mDoc.moveToStartOfContent();
			}
			break;
		}
		case KEY_END: {
			if ( ( event.getMod() & KEYMOD_CTRL ) && ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.selectToEndOfDoc();
			} else if ( event.getMod() & KEYMOD_SHIFT ) {
				mDoc.selectToEndOfLine();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.moveToEndOfDoc();
			} else if ( event.getMod() == 0 ) {
				mDoc.moveToEndOfLine();
			}
			break;
		}
		case KEY_PAGEUP: {
			if ( event.getMod() & KEYMOD_SHIFT ) {
				mDoc.selectToPreviousPage( getVisibleLinesCount() );
			} else if ( event.getMod() == 0 ) {
				mDoc.moveToPreviousPage( getVisibleLinesCount() );
			}
			break;
		}
		case KEY_PAGEDOWN: {
			if ( event.getMod() & KEYMOD_SHIFT ) {
				mDoc.selectToNextPage( getVisibleLinesCount() );
			} else if ( event.getMod() == 0 ) {
				mDoc.moveToNextPage( getVisibleLinesCount() );
			}
			break;
		}
		case KEY_TAB: {
			UIEventDispatcher* eventDispatcher =
				static_cast<UIEventDispatcher*>( getUISceneNode()->getEventDispatcher() );
			if ( !eventDispatcher->justGainedFocus() ) {
				if ( event.getMod() & KEYMOD_SHIFT ) {
					mDoc.unindent();
				} else if ( event.getMod() == 0 ) {
					mDoc.indent();
				}
			}
			break;
		}
		case KEY_V: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.textInput( getUISceneNode()->getWindow()->getClipboard()->getText() );
			}
			break;
		}
		case KEY_C: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
			}
			break;
		}
		case KEY_X: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
				mDoc.deleteSelection();
			}
			break;
		}
		case KEY_A: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.selectAll();
			}
			break;
		}
		case KEY_Z: {
			if ( ( event.getMod() & KEYMOD_CTRL ) && ( event.getMod() & KEYMOD_SHIFT ) ) {
				mDoc.redo();
			} else if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.undo();
			}
			break;
		}
		case KEY_Y: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mDoc.redo();
			}
			break;
		}
		case KEY_PLUS:
		case KEY_KP_PLUS: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mFontStyleConfig.CharacterSize =
					eemin<Float>( 96, mFontStyleConfig.CharacterSize + 1 );
				invalidateDraw();
			}
			break;
		}
		case KEY_MINUS:
		case KEY_KP_MINUS: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				mFontStyleConfig.CharacterSize =
					eemax<Float>( 4, mFontStyleConfig.CharacterSize - 1 );
				invalidateDraw();
			}
			break;
		}
		case KEY_0:
		case KEY_KP0: {
			if ( event.getMod() & KEYMOD_CTRL ) {
				setFontSize( mFontSize );
			}
			break;
		}
		default:
			break;
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
	if ( !getUISceneNode()->getEventDispatcher()->isNodeDragging() && NULL != mFont &&
		 !mMouseDown && ( flags & EE_BUTTON_LMASK ) ) {
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
	if ( !getUISceneNode()->getEventDispatcher()->isNodeDragging() && NULL != mFont && mMouseDown &&
		 ( flags & EE_BUTTON_LMASK ) ) {
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
		mMouseDown = false;
		getUISceneNode()->getWindow()->getInput()->captureMouse( false );
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

Uint32 UICodeEditor::onMouseDoubleClick( const Vector2i&, const Uint32& flags ) {
	if ( mLocked || NULL == mFont )
		return 1;

	if ( flags & EE_BUTTON_LMASK ) {
		mDoc.selectWord();
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
	mScroll = {0, 0};
	invalidateEditor();
}

void UICodeEditor::onPaddingChange() {
	UIWidget::onPaddingChange();
	invalidateEditor();
}

void UICodeEditor::updateScrollBar() {
	mVScrollBar->setPixelsSize( 0, mSize.getHeight() );
	mVScrollBar->setPixelsPosition( mSize.getWidth() - mVScrollBar->getPixelsSize().getWidth(), 0 );
	int notVisibleLineCount = (int)mDoc.linesCount() - (int)getViewPortLineCount().y;
	mVScrollBar->setPageStep( getViewPortLineCount().y / (float)mDoc.linesCount() );
	mVScrollBar->setClickStep( 0.2f );
	mVScrollBar->setEnabled( notVisibleLineCount > 0 );
	mVScrollBar->setVisible( notVisibleLineCount > 0 );
	setScrollY( mScroll.y );
}

void UICodeEditor::updateEditor() {
	scrollToMakeVisible( mDoc.getSelection().start() );
	updateScrollBar();
	mDirtyEditor = false;
}

void UICodeEditor::onDocumentTextChanged() {
	invalidateDraw();
}

void UICodeEditor::onDocumentCursorChange( const Doc::TextPosition& ) {
	resetCursor();
	invalidateEditor();
	invalidateDraw();
}

void UICodeEditor::onDocumentSelectionChange( const Doc::TextRange& ) {
	resetCursor();
	invalidateDraw();
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
	// Vertical Scroll
	Float lineHeight = getLineHeight();
	Float min = eefloor( lineHeight * ( eemax<Float>( 0, position.line() - 1 ) ) );
	Float max = eefloor( lineHeight * ( position.line() + 2 ) - mSize.getHeight() );
	if ( min < mScroll.y )
		setScrollY( min );
	else if ( max > mScroll.y )
		setScrollY( max );

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

Int64 UICodeEditor::getColFromXOffset( Int64 lineNumber, const Float& x ) const {
	if ( x <= 0 )
		return 0;
	TextPosition pos = mDoc.sanitizePosition( TextPosition( lineNumber, 0 ) );
	const String& line = mDoc.line( pos.line() ).getText();
	Int64 len = line.length();
	Float glyphWidth = getGlyphWidth();
	Float xOffset = 0;
	for ( int i = 0; i < len; i++ ) {
		if ( xOffset >= x )
			return xOffset - x > glyphWidth * 0.5f ? eemax<Int64>( 0, i - 1 ) : i;
		xOffset += ( line[i] == '\t' ) ? glyphWidth * mTabWidth : glyphWidth;
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

}} // namespace EE::UI
