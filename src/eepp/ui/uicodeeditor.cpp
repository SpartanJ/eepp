#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
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
	mTabWidth( 4 ),
	mLastColOffset( 0 ),
	mMouseWheelScroll( 50 ) {
	clipEnable();
	if ( NULL == mFont ) {
		mFont = FontTrueType::New( "monospace", "assets/fonts/DejaVuSansMono.ttf" );
	}
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
	setThemeSkin( Theme, "textedit" );
}

void UICodeEditor::draw() {
	UIWidget::draw();

	if ( mDirtyEditor ) {
		updateEditor();
	}

	if ( mFont == NULL )
		return;

	std::pair<int, int> lineRange = getVisibleLineRange();
	Float charSize = PixelDensity::pxToDp( getCharacterSize() );
	Float lineHeight = getLineHeight();
	Vector2f start( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top );
	Vector2f startScroll( start - mScroll );
	Primitives primitives;
	TextPosition cursor( mDoc.getSelection().start() );

	primitives.setColor( Color( 255, 255, 255, 20 ) );
	primitives.drawRectangle(
		Rectf( Vector2f( startScroll.x, startScroll.y + cursor.line() * lineHeight ),
			   Sizef( mSize.getWidth(), lineHeight ) ) );

	if ( mDoc.hasSelection() ) {
		primitives.setColor( Color( 255, 255, 255, 50 ) );

		TextRange selection = mDoc.getSelection( true );

		int startLine = eemax<int>( lineRange.first, selection.start().line() );
		int endLine = eemin<int>( lineRange.second, selection.end().line() );

		for ( auto ln = startLine; ln <= endLine; ln++ ) {
			const String& line = mDoc.line( ln );
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
		Text line( mDoc.line( i ), mFont, charSize );
		line.setStyleConfig( mFontStyleConfig );
		line.draw( startScroll.x, startScroll.y + lineHeight * i );
	}

	if ( mCursorVisible ) {
		Vector2f cursorPos( startScroll.x + getXOffsetCol( cursor ),
							startScroll.y + cursor.line() * lineHeight );

		primitives.setColor( Color::White );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 2 ), lineHeight ) ) );
	}
}

void UICodeEditor::scheduledUpdate( const Time& ) {
	if ( hasFocus() ) {
		if ( mBlinkTimer.getElapsedTime().asSeconds() > 0.5f ) {
			mCursorVisible = !mCursorVisible;
			mBlinkTimer.restart();
			invalidateDraw();
		}
	}

	if ( mMouseDown &&
		 !( getUISceneNode()->getWindow()->getInput()->getPressTrigger() & EE_BUTTON_LMASK ) ) {
		mMouseDown = false;
	}
}

void UICodeEditor::reset() {}

void UICodeEditor::loadFromFile( const std::string& path ) {
	mDoc.loadFromPath( path );
	invalidateEditor();
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

void UICodeEditor::invalidateEditor() {
	mDirtyEditor = true;
}

Uint32 UICodeEditor::onFocusLoss() {
	mMouseDown = false;
	mCursorVisible = false;
	return UIWidget::onFocusLoss();
}

Uint32 UICodeEditor::onTextInput( const TextInputEvent& event ) {
	mDoc.textInput( event.getText() );
	return 1;
}

Uint32 UICodeEditor::onKeyDown( const KeyEvent& event ) {
	switch ( event.getKeyCode() ) {
		case KEY_BACKSPACE: {
			mDoc.deleteToPreviousChar();
			updateLastColumnOffset();
			break;
		}
		case KEY_DELETE: {
			mDoc.deleteToNextChar();
			break;
		}
		case KEY_KP_ENTER:
		case KEY_RETURN: {
			mDoc.newLine();
			updateLastColumnOffset();
			break;
		}
		case KEY_UP: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToPreviousLine( mLastColOffset );
			} else {
				mDoc.moveToPreviousLine( mLastColOffset );
			}
			break;
		}
		case KEY_DOWN: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToNextLine( mLastColOffset );
			} else {
				mDoc.moveToNextLine( mLastColOffset );
			}
			break;
		}
		case KEY_LEFT: {
			if ( ( event.getMod() & KEYMOD_LSHIFT ) && ( event.getMod() & KEYMOD_LCTRL ) ) {
				mDoc.selectToPreviousWord();
			} else if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToPreviousChar();
			} else if ( event.getMod() & KEYMOD_LCTRL ) {
				mDoc.moveToPreviousWord();
			} else {
				mDoc.moveToPreviousChar();
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_RIGHT: {
			if ( ( event.getMod() & KEYMOD_LSHIFT ) && ( event.getMod() & KEYMOD_LCTRL ) ) {
				mDoc.selectToNextWord();
			} else if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToNextChar();
			} else if ( event.getMod() & KEYMOD_LCTRL ) {
				mDoc.moveToNextWord();
			} else {
				mDoc.moveToNextChar();
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_HOME: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToStartOfLine();
				updateLastColumnOffset();
			} else if ( event.getMod() & KEYMOD_LCTRL ) {
				mScroll.y = 0;
				mDoc.setSelection( {0, 0} );
				invalidateDraw();
			} else {
				mDoc.setSelection( mDoc.startOfLine( mDoc.getSelection().start() ) );
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_END: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToEndOfLine();
				updateLastColumnOffset();
			} else if ( event.getMod() & KEYMOD_LCTRL ) {
				mScroll.y = getMaxScroll().y;
				mDoc.setSelection(
					{static_cast<Int64>( mDoc.lineCount() - 1 ),
					 static_cast<Int64>( mDoc.line( mDoc.lineCount() - 1 ).length() )} );
				invalidateDraw();
			} else {
				mDoc.setSelection( mDoc.endOfLine( mDoc.getSelection().start() ) );
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_PAGEUP: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToPreviousPage( getVisibleLinesCount() );
				updateLastColumnOffset();
			} else {
				mDoc.moveToPreviousPage( getVisibleLinesCount() );
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_PAGEDOWN: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.selectToNextPage( getVisibleLinesCount() );
				updateLastColumnOffset();
			} else {
				mDoc.moveToNextPage( getVisibleLinesCount() );
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_TAB: {
			if ( event.getMod() & KEYMOD_LSHIFT ) {
				mDoc.unindent();
				updateLastColumnOffset();
			} else if ( !event.getMod() ) {
				mDoc.indent();
				updateLastColumnOffset();
			}
			break;
		}
		case KEY_V: {
			if ( event.getMod() & KEYMOD_LCTRL ) {
				mDoc.textInput( getUISceneNode()->getWindow()->getClipboard()->getText() );
			}
			break;
		}
		case KEY_C: {
			if ( event.getMod() & KEYMOD_LCTRL ) {
				getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
			}
			break;
		}
		case KEY_X: {
			if ( event.getMod() & KEYMOD_LCTRL ) {
				getUISceneNode()->getWindow()->getClipboard()->setText( mDoc.getSelectedText() );
				mDoc.deleteSelection();
			}
			break;
		}
		case KEY_A: {
			if ( event.getMod() & KEYMOD_LCTRL ) {
				mDoc.selectAll();
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
	localPos.x -= mRealPadding.Left;
	localPos.y -= mRealPadding.Top;
	Int64 line =
		eeclamp<Int64>( (Int64)eefloor( localPos.y / getLineHeight() ), 0, mDoc.lineCount() - 1 );
	return TextPosition( line, getColFromXOffset( line, localPos.x ) );
}

Vector2f UICodeEditor::getViewPortLineCount() const {
	return Vector2f( eefloor( mSize.getWidth() / getGlyphWidth() ),
					 eefloor( mSize.getHeight() / getLineHeight() ) );
}

Sizef UICodeEditor::getMaxScroll() const {
	Vector2f vplc( getViewPortLineCount() );
	return Sizef(
		eefloor( ( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right ) / getGlyphWidth() ),
		vplc.y > mDoc.lineCount() - 1
			? 0.f
			: ( mDoc.lineCount() - getViewPortLineCount().y ) * getLineHeight() );
}

Uint32 UICodeEditor::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( !mMouseDown && ( flags & EE_BUTTON_LMASK ) ) {
		mMouseDown = true;
		mDoc.setSelection( resolveScreenPosition( position.asFloat() ) );
	}
	return UIWidget::onMouseDown( position, flags );
}

Uint32 UICodeEditor::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	if ( mMouseDown && ( flags & EE_BUTTON_LMASK ) ) {
		TextRange selection = mDoc.getSelection();
		selection.setStart( resolveScreenPosition( position.asFloat() ) );
		mDoc.setSelection( selection );
	}
	return UIWidget::onMouseMove( position, flags );
}

Uint32 UICodeEditor::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK ) {
		mMouseDown = false;
	} else if ( flags & EE_BUTTON_WDMASK ) {
		mScroll.y += PixelDensity::dpToPx( mMouseWheelScroll );
		mScroll.y = eemin( mScroll.y, getMaxScroll().y );
		invalidateDraw();
	} else if ( flags & EE_BUTTON_WUMASK ) {
		mScroll.y -= PixelDensity::dpToPx( mMouseWheelScroll );
		mScroll.y = eemax( mScroll.y, 0.f );
		invalidateDraw();
	}
	return UIWidget::onMouseUp( position, flags );
}

Uint32 UICodeEditor::onMouseDoubleClick( const Vector2i&, const Uint32& flags ) {
	if ( flags & EE_BUTTON_LMASK ) {
		mDoc.selectWord();
	}
	return 1;
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

void UICodeEditor::updateEditor() {
	scrollToMakeVisible( mDoc.getSelection().start() );
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

std::pair<int, int> UICodeEditor::getVisibleLineRange() {
	Float lineHeight = getLineHeight();
	Float minLine = eemax( 0.f, eefloor( mScroll.y / lineHeight ) );
	Float maxLine = eemin( mDoc.lineCount() - 1.f,
						   eefloor( ( mSize.getHeight() + mScroll.y ) / lineHeight ) + 1 );
	return std::make_pair<int, int>( (int)minLine, (int)maxLine );
}

int UICodeEditor::getVisibleLinesCount() {
	auto lines = getVisibleLineRange();
	return lines.second - lines.first;
}

void UICodeEditor::scrollToMakeVisible( const TextPosition& position ) {
	Float lineHeight = getLineHeight();
	Float min = lineHeight * ( eemax<Float>( 0, position.line() - 1 ) );
	Float max = lineHeight * ( position.line() + 2 ) - mSize.getHeight();
	mScroll.y = eemin( mScroll.y, min );
	mScroll.y = eemax( mScroll.y, max );
	invalidateDraw();
}

Float UICodeEditor::getXOffsetCol( const TextPosition& position ) const {
	const String& line = mDoc.line( position.line() );
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

Int64 UICodeEditor::getColFromXOffset( Int64 _line, const Float& offset ) const {
	if ( offset <= 0 )
		return 0;
	TextPosition pos = mDoc.sanitizePosition( TextPosition( _line, 0 ) );
	const String& line = mDoc.line( pos.line() );
	Float glyphWidth = getGlyphWidth();
	Float x = 0;
	for ( size_t i = 0; i < line.size(); i++ ) {
		if ( line[i] == '\t' ) {
			x += glyphWidth * mTabWidth;
		} else if ( line[i] != '\n' && line[i] != '\r' ) {
			x += glyphWidth;
		}
		if ( x >= offset ) {
			return i;
		}
	}
	return line.size() - 1;
}

Float UICodeEditor::getLineHeight() const {
	return mFont->getLineSpacing( getCharacterSize() );
}

Float UICodeEditor::getCharacterSize() const {
	return PixelDensity::dpToPx( mFontStyleConfig.getFontCharacterSize() );
}

Float UICodeEditor::getGlyphWidth() const {
	return mFont->getGlyph( KEY_SPACE, getCharacterSize(), false ).advance;
}

void UICodeEditor::updateLastColumnOffset() {
	mLastColOffset = mDoc.getAbsolutePosition( mDoc.getSelection().start() ).column();
}

void UICodeEditor::resetCursor() {
	if ( hasFocus() ) {
		mCursorVisible = true;
		mBlinkTimer.restart();
	}
}

}} // namespace EE::UI
